#include <glob.h>
#include <unistd.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include "common.h"
#include "cpu.h"
#include "gemdos.h"
#include "mmu.h"

#define SP(x) cpu->a[7]+x
#define SPWORD(x) mmu_read_word_print(SP(x))
#define SPLONG(x) mmu_read_long_print(SP(x))

#define GEMDOS_BASEDIR "/var/tmp/HD/"
#define GEMDOS_NO_FILE 0
#define GEMDOS_FILE_OK 1

#define DRIVE_C 2
#define DRIVE_N 14

#define GEMDOS_DSETDRV  0x0e
#define GEMDOS_DGETDRV  0x19
#define GEMDOS_FSETDTA  0x1a
#define GEMDOS_DSETPATH 0x3b
#define GEMDOS_FOPEN    0x3d
#define GEMDOS_FCLOSE   0x3e
#define GEMDOS_FREAD    0x3f
#define GEMDOS_FSFIRST  0x4e
#define GEMDOS_FSNEXT   0x4f

#define GEMDOS_E_OK      0
#define GEMDOS_EFILNF  -33
#define GEMDOS_EPTHNF  -34
#define GEMDOS_ENHNDL  -35
#define GEMDOS_EIHNDL  -37
#define GEMDOS_ENMFIL  -49

static int drive_selected = 0;
static char current_path[1024];
static FILE *handles[1024] = {};
static LONG dta = 0;
static glob_t globbuf;
static int globpos = 0;

struct dta {
  BYTE d_reserved[21];
  BYTE d_attrib;
  WORD d_time;
  WORD d_date;
  LONG d_length;
  BYTE d_fname[14];
};

int gemdos_hd_drive()
{
  return DRIVE_C;
}

static char *host_path(char *basedir, char *tos_path)
{
  int i;
  int content_offset = 0;
  static char new_path[2048] = "";
  
  if(!tos_path || strlen(tos_path) == 0) {
    return basedir;
  }

  if(strlen(tos_path) == 2 || strlen(tos_path) == 3) {
    if(tos_path[1] == ':' && (tos_path[2] == '\0' || tos_path[2] == '\\')) {
      return basedir;
    }
  }

  if(strlen(tos_path) > 3 && tos_path[1] == ':') {
    tos_path += 3;
  }
  
  strcat(new_path, basedir);
  content_offset = strlen(basedir);

  for(i=0;i<1024-strlen(new_path);i++) {
    char tmp;

    tmp = tos_path[i];
    if(tmp == '\\') {
      tmp = '/';
    }

    /* Truncate "*.*" to just "*" */
    if(tmp == '*') {
      if(new_path[i+content_offset-1] == '.' && new_path[i+content_offset-2] == '*') {
        content_offset -= 2;
        continue;
      }
    }

    new_path[i+content_offset] = tmp;
    if(!new_path[i+content_offset]) break;
  }

  if(new_path[strlen(new_path)-1] == '/' && new_path[strlen(new_path)-2] == '/') {
    new_path[strlen(new_path)-1] = '\0';
  }
  return new_path;
}

static void set_return_word(struct cpu *cpu, WORD value)
{
  cpu->d[0] = (cpu->d[0]&0xffff0000)|value;
}

static void set_return_long(struct cpu *cpu, LONG value)
{
  cpu->d[0] = value;
}

static int gemdos_dsetdrv(struct cpu *cpu)
{
  int drivenum;

  drivenum = SPWORD(2);
  if(drivenum == gemdos_hd_drive()) {
    drive_selected = 1;
    set_return_long(cpu, mmu_read_long_print(0x4c2) | (1<<drivenum));
    return GEMDOS_ABORT_CALL;
  } else {
    drive_selected = 0;
    return GEMDOS_RESUME_CALL;
  }
}

static int gemdos_dgetdrv(struct cpu *cpu)
{
  if(!drive_selected) {
    return GEMDOS_RESUME_CALL;
  }

  set_return_word(cpu, gemdos_hd_drive());
  return GEMDOS_ABORT_CALL;
}

static int gemdos_dsetpath(struct cpu *cpu)
{
  int i;
  LONG path_addr;

  if(!drive_selected) {
    return GEMDOS_RESUME_CALL;
  }
  
  path_addr = SPLONG(2);
  
  for(i=0;i<1024;i++) {
    current_path[i] = mmu_read_byte_print(path_addr+i);
    if(!current_path[i]) break;
  }

  set_return_word(cpu, GEMDOS_E_OK);
  return GEMDOS_ABORT_CALL;
}

static int find_free_handle()
{
  int i;
  for(i=2;i<1024;i++) {
    if(!handles[i]) return i;
  }
  return 0;
}

static int gemdos_fsetdta(struct cpu *cpu)
{
  if(!drive_selected) {
    return GEMDOS_RESUME_CALL;
  }

  dta = SPLONG(2);

  return GEMDOS_ABORT_CALL;
}

static WORD dta_time(time_t mtime)
{
  struct tm *tm;
  WORD dtime;
  tm = localtime(&mtime);

  dtime = tm->tm_hour << 11;
  dtime |= tm->tm_min << 5;
  dtime |= tm->tm_sec >> 1;

  return dtime;
}

static WORD dta_date(time_t mtime)
{
  struct tm *tm;
  WORD ddate;
  tm = localtime(&mtime);

  ddate = (tm->tm_year + 1900 - 1980)<<9;
  ddate |= (tm->tm_mon+1) << 5;
  ddate |= tm->tm_mday;

  return ddate;
}

static int dta_write()
{
  int i,j,skip;
  char *filename;
  char *dirname;
  struct stat buf;
  BYTE attrib = 0;
  int size;

  dirname = host_path(GEMDOS_BASEDIR, current_path);

  for(i=globpos;i<globbuf.gl_pathc;i++) {
    skip = 0;
    filename = globbuf.gl_pathv[i];
    filename += strlen(dirname);
    if(strlen(filename) > 12) {
      printf("DEBUG: Skipping %s due to length\n", filename);
      skip = 1;
      continue;
    }
    for(j=0;j<strlen(filename);j++) {
      if(!(((filename[j] >= '0') && (filename[j] <= '9')) ||
           ((filename[j] >= 'A') && (filename[j] <= 'Z')) ||
           (filename[j] == '_') || (filename[j] == '-') ||
           (filename[j] == '.'))) {
        printf("DEBUG: Skipping %s due to bad characters\n", filename);
        skip = 1;
        break;
      }
    }
    if(skip) continue;

    stat(globbuf.gl_pathv[i], &buf);
    if(!S_ISREG(buf.st_mode) && !S_ISDIR(buf.st_mode)) {
      skip = 1;
      printf("DEBUG: Skipping %s due to not being file or directory: %08x\n", filename, buf.st_mode);
      continue;
    }

    if(buf.st_mode & S_IFDIR) {
      attrib |= 0x10;
    }
    size = buf.st_size;
    mmu_write_byte(dta + 21, attrib);
    mmu_write_word(dta + 22, dta_time(buf.st_mtime));
    mmu_write_word(dta + 24, dta_date(buf.st_mtime));
    mmu_write_long(dta + 26, size);
    for(j=0;j<strlen(filename);j++) {
      mmu_write_byte(dta + 30 + j, filename[j]);
    }
    mmu_write_byte(dta + 30 + j, 0);
    globpos = i+1;
    break;
  }

  if(skip) {
    return GEMDOS_NO_FILE;
  } else {
    return GEMDOS_FILE_OK;
  }
}

static int gemdos_fsfirst(struct cpu *cpu)
{
  int i;
  WORD attr;
  LONG filename_addr;
  char filename[1024];
  char *pwd;
  int content_offset = 0;
  
  if(!drive_selected) {
    return GEMDOS_RESUME_CALL;
  }

  pwd = getcwd(NULL, 1024);
  if(!pwd) {
    printf("DEBUG: getcwd fail: %s\n", strerror(errno));
  }

  if(chdir(GEMDOS_BASEDIR)) {
    printf("DEBUG: CHDIR fail\n");
  }
  
  filename_addr = SPLONG(2);
  attr = SPWORD(6);

  content_offset = 0;
  for(i=0;i<1024;i++) {
    BYTE tmp;
    tmp = mmu_read_byte_print(filename_addr+i);
#if 0
    if(i == 2 && tmp == '\\' && filename[i+content_offset-1] == ':') {
      content_offset -= 3;
      continue;
    }
    if(tmp == '\\') {
      tmp = '/';
    }
    if(tmp == '*') {
      if(filename[i+content_offset-1] == '.' && filename[i+content_offset-2] == '*') {
        content_offset -= 2;
        continue;
      }
    }
#endif
    filename[i+content_offset] = tmp;
    if(!filename[i+content_offset]) break;
  }

  globpos = 0;
  glob(host_path(GEMDOS_BASEDIR, filename), 0, NULL, &globbuf);
  if(globbuf.gl_pathc == 0) {
    set_return_long(cpu, GEMDOS_ENMFIL);
  } else {
    if(dta_write()) {
      set_return_long(cpu, GEMDOS_E_OK);
    } else {
      set_return_long(cpu, GEMDOS_ENMFIL);
    }
  }
  
  i = attr; /* Dummy */
  
  if(chdir(pwd)) {
    printf("DEBUG: CHDIR fail\n");
  }
  
  return GEMDOS_ABORT_CALL;
}

static int gemdos_fsnext(struct cpu *cpu)
{
  char *pwd;
  if(!drive_selected) {
    return GEMDOS_RESUME_CALL;
  }

  pwd = getcwd(NULL, 1024);
  if(!pwd) {
    printf("DEBUG: getcwd fail: %s\n", strerror(errno));
  }

  if(chdir(GEMDOS_BASEDIR)) {
    printf("DEBUG: CHDIR fail\n");
  }
  
  if(dta_write()) {
    set_return_long(cpu, GEMDOS_E_OK);
  } else {
    set_return_long(cpu, GEMDOS_ENMFIL);
  }

  if(chdir(pwd)) {
    printf("DEBUG: CHDIR fail\n");
  }
  
return GEMDOS_ABORT_CALL;
}

static int gemdos_fopen(struct cpu *cpu)
{
  int i;
  WORD mode;
  LONG fname_addr;
  char fname[1024];
  int handle;

  if(!drive_selected) {
    return GEMDOS_RESUME_CALL;
  }

  fname_addr = SPLONG(2);
  mode = SPWORD(6);

  if(mode != 0) {
    return GEMDOS_RESUME_CALL;
  }
  
  snprintf(fname, strlen(GEMDOS_BASEDIR)+1, GEMDOS_BASEDIR);
  for(i=0;i<1024;i++) {
    BYTE tmp;
    tmp = mmu_read_byte_print(fname_addr+i);
    if(tmp == '\\') {
      tmp = '/';
    }
    fname[i+strlen(GEMDOS_BASEDIR)] = tmp;
    if(!fname[i+strlen(GEMDOS_BASEDIR)]) break;
  }

  handle = find_free_handle();
  
  if(!handle) {
    set_return_long(cpu, GEMDOS_ENHNDL);
    return GEMDOS_ABORT_CALL;
  }
  
  handles[handle] = fopen(fname, "rb");
  printf("DEBUG: fname == %s\n", fname);
  if(handles[handle]) {
    set_return_long(cpu, handle);
  } else {
    set_return_long(cpu, GEMDOS_EFILNF);
  }
  return GEMDOS_ABORT_CALL;
}

static int gemdos_fread(struct cpu *cpu)
{
  int i;
  WORD handle;
  LONG count;
  LONG buf;
  BYTE *tmpdata;
  int bytes_read;

  if(!drive_selected) {
    return GEMDOS_RESUME_CALL;
  }
  
  handle = SPWORD(2);
  count = SPLONG(4);
  buf = SPLONG(8);

  if(!handles[handle]) {
    set_return_long(cpu, GEMDOS_EIHNDL);
    return GEMDOS_ABORT_CALL;
  }
  
  tmpdata = malloc(count);
  bytes_read = fread(tmpdata, 1, count, handles[handle]);

  printf("DEBUG: bytes_read: %d\n", bytes_read);
  cpu_enter_debugger();
  
  for(i=0;i<count;i++) {
    mmu_write_byte(buf+i, tmpdata[i]);
  }
  set_return_long(cpu, bytes_read);
  return GEMDOS_ABORT_CALL;
}

int gemdos_fclose(struct cpu *cpu)
{
  WORD handle;

  if(!drive_selected) {
    return GEMDOS_RESUME_CALL;
  }

  handle = SPWORD(2);
  if(handles[handle]) {
    handles[handle] = NULL;
    set_return_long(cpu, GEMDOS_E_OK);
  } else {
    set_return_long(cpu, GEMDOS_EIHNDL);
  }
  return GEMDOS_ABORT_CALL;
}

int gemdos_hd(struct cpu *cpu)
{
  switch(SPWORD(0)) {
  case GEMDOS_DSETDRV:
    return gemdos_dsetdrv(cpu);
  case GEMDOS_DGETDRV:
    return gemdos_dgetdrv(cpu);
  case GEMDOS_DSETPATH:
    return gemdos_dsetpath(cpu);
  case GEMDOS_FSETDTA:
    return gemdos_fsetdta(cpu);
  case GEMDOS_FSFIRST:
    return gemdos_fsfirst(cpu);
  case GEMDOS_FSNEXT:
    return gemdos_fsnext(cpu);
  case GEMDOS_FOPEN:
    return gemdos_fopen(cpu);
  case GEMDOS_FCLOSE:
    return gemdos_fclose(cpu);
  case GEMDOS_FREAD:
    return gemdos_fread(cpu);
  }
  return GEMDOS_RESUME_CALL;
}

static void gemdos_pexec_print(struct cpu *cpu)
{
  int i;
  WORD mode;
  LONG name_addr;
  LONG cmdline_addr;
  LONG env_addr;
  char name[1024] = "";
  char cmdline[1024] = "";

  mode = mmu_read_word_print(SP(2));
  name_addr = mmu_read_long_print(SP(4));
  cmdline_addr = mmu_read_long_print(SP(8)); /* Also basepage */
  env_addr = mmu_read_long_print(SP(12));

  if(mode != 4 && mode != 5) {
    for(i=0;i<1024;i++) {
      name[i] = mmu_read_byte_print(name_addr+i);
      if(!name[i]) break;
    }
  }
  if(mode != 4) {
    cmdline[0] = '"';
    for(i=0;i<1022;i++) {
      cmdline[i+1] = mmu_read_byte_print(cmdline_addr+i);
      if(!cmdline[i+1]) break;
    }
    cmdline[1023] = '\0';
    i = strlen(cmdline);
    cmdline[i] = '"';
    cmdline[i+1] = '\0';
  } else {
    snprintf(cmdline, 1023, "$%06x", cmdline_addr);
  }

  printf("Pexec(%d, \"%s\", %s, $%06x)\n", mode, name, cmdline, env_addr);
}

static void gemdos_super_print(struct cpu *cpu)
{
  LONG stack;

  stack = mmu_read_long_print(SP(2));
  printf("Super($%06x)\n", stack);
}

static void gemdos_mshrink_print(struct cpu *cpu)
{
  LONG block;
  LONG newsiz;

  block = mmu_read_long_print(SP(4));
  newsiz = mmu_read_long_print(SP(8));
  printf("Mshrink($%06x, %d)\n", block, newsiz);
}

static void gemdos_malloc_print(struct cpu *cpu)
{
  LONG size;

  size = mmu_read_long_print(SP(2));
  printf("Malloc(%d)\n", size);
}

static void gemdos_mfree_print(struct cpu *cpu)
{
  LONG block;

  block = mmu_read_long_print(SP(2));
  printf("Mfree($%06x)\n", block);
}

static void gemdos_fsfirst_print(struct cpu *cpu)
{
  int i;
  LONG filename_addr;
  WORD attr;
  char filename[1024] = "";

  filename_addr = mmu_read_long_print(SP(2));
  attr = mmu_read_word_print(SP(6));
  
  for(i=0;i<1024;i++) {
    filename[i] = mmu_read_byte_print(filename_addr+i);
    if(!filename[i]) break;
  }
  printf("Fsfirst(\"%s\", $%02x)\n", filename, attr);
}

static void gemdos_fsnext_print(struct cpu *cpu)
{
  printf("Fsnext()\n");
}

static void gemdos_dsetpath_print(struct cpu *cpu)
{
  int i;
  LONG path_addr;
  char path[1024] = "";

  path_addr = mmu_read_long_print(SP(2));
  
  for(i=0;i<1024;i++) {
    path[i] = mmu_read_byte_print(path_addr+i);
    if(!path[i]) break;
  }
  printf("Dsetpath(\"%s\")\n", path);
}

static void gemdos_dsetdrv_print(struct cpu *cpu)
{
  WORD drivenum;

  drivenum = mmu_read_word_print(SP(2));
  printf("Dsetdrv(\"%c:\")\n", drivenum + 'A');
}

static void gemdos_dgetdrv_print(struct cpu *cpu)
{
  printf("Dgetdrv()\n");
}

static void gemdos_fsetdta_print(struct cpu *cpu)
{
  LONG buf;

  buf = mmu_read_long_print(SP(2));
  printf("Fsetdta($%06x)\n", buf);
}

static void gemdos_fopen_print(struct cpu *cpu)
{
  int i;
  LONG fname_addr;
  WORD mode;
  char fname[1024];

  fname_addr = mmu_read_long_print(SP(2));
  mode = mmu_read_word_print(SP(6));
  
  for(i=0;i<1024;i++) {
    fname[i] = mmu_read_byte_print(fname_addr+i);
    if(!fname[i]) break;
  }
  printf("Fopen(\"%s\", $%04x)\n", fname, mode);
}

static void gemdos_fread_print(struct cpu *cpu)
{
  WORD handle;
  LONG count;
  LONG buf;

  handle = mmu_read_word_print(SP(2));
  count = mmu_read_long_print(SP(4));
  buf = mmu_read_long_print(SP(8));
  
  printf("Fread(%d, %d, $%06x)\n", handle, count, buf);
}

static void gemdos_fclose_print(struct cpu *cpu)
{
  WORD handle;

  handle = mmu_read_word_print(SP(2));
  printf("Fclose(%d)\n", handle);
}

static void gemdos_pterm0_print(struct cpu *cpu)
{
  printf("Pterm0()\n");
}

void gemdos_print(struct cpu *cpu)
{
  int cmd;

  cmd = mmu_read_word_print(SP(0));
  switch(cmd) {
  case 0x00:
    gemdos_pterm0_print(cpu);
    break;
  case 0x0e:
    gemdos_dsetdrv_print(cpu);
    break;
  case 0x19:
    gemdos_dgetdrv_print(cpu);
    break;
  case 0x1a:
    gemdos_fsetdta_print(cpu);
    break;
  case 0x20:
    gemdos_super_print(cpu);
    break;
  case 0x3b:
    gemdos_dsetpath_print(cpu);
    break;
  case 0x3d:
    gemdos_fopen_print(cpu);
    break;
  case 0x3e:
    gemdos_fclose_print(cpu);
    break;
  case 0x3f:
    gemdos_fread_print(cpu);
    break;
  case 0x48:
    gemdos_malloc_print(cpu);
    break;
  case 0x49:
    gemdos_mfree_print(cpu);
    break;
  case 0x4a:
    gemdos_mshrink_print(cpu);
    break;
  case 0x4b:
    gemdos_pexec_print(cpu);
    break;
  case 0x4e:
    gemdos_fsfirst_print(cpu);
    break;
  case 0x4f:
    gemdos_fsnext_print(cpu);
    break;
  default:
    printf("CMD: %d [%04x]\n", cmd, cmd);
    //    exit(-1);
  }
}
