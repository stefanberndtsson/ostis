#include "common.h"
#include "cpu.h"
#include "mmu.h"

#define SP(x) cpu->a[7]+x

static void gemdos_pexec(struct cpu *cpu)
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

static void gemdos_super(struct cpu *cpu)
{
  LONG stack;

  stack = mmu_read_long_print(SP(2));
  printf("Super($%06x)\n", stack);
}

static void gemdos_mshrink(struct cpu *cpu)
{
  LONG block;
  LONG newsiz;

  block = mmu_read_long_print(SP(4));
  newsiz = mmu_read_long_print(SP(8));
  printf("Mshrink($%06x, %d)\n", block, newsiz);
}

static void gemdos_malloc(struct cpu *cpu)
{
  LONG size;

  size = mmu_read_long_print(SP(2));
  printf("Malloc(%d)\n", size);
}

static void gemdos_mfree(struct cpu *cpu)
{
  LONG block;

  block = mmu_read_long_print(SP(2));
  printf("Mfree($%06x)\n", block);
}

static void gemdos_fsfirst(struct cpu *cpu)
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

static void gemdos_fsnext(struct cpu *cpu)
{
  printf("Fsnext()\n");
}

static void gemdos_dsetpath(struct cpu *cpu)
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

static void gemdos_dsetdrv(struct cpu *cpu)
{
  WORD drivenum;

  drivenum = mmu_read_word_print(SP(2));
  printf("Dsetdrv(\"%c:\")\n", drivenum + 'A');
}

static void gemdos_dgetdrv(struct cpu *cpu)
{
  printf("Dgetdrv()\n");
}

static void gemdos_fsetdta(struct cpu *cpu)
{
  LONG buf;

  buf = mmu_read_long_print(SP(2));
  printf("Fsetdta($%06x)\n", buf);
}

static void gemdos_fopen(struct cpu *cpu)
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

static void gemdos_fread(struct cpu *cpu)
{
  WORD handle;
  LONG count;
  LONG buf;

  handle = mmu_read_word_print(SP(2));
  count = mmu_read_long_print(SP(4));
  buf = mmu_read_long_print(SP(8));
  
  printf("Fread(%d, %d, $%06x)\n", handle, count, buf);
}

static void gemdos_fclose(struct cpu *cpu)
{
  WORD handle;

  handle = mmu_read_word_print(SP(2));
  printf("Fclose(%d)\n", handle);
}

void gemdos_print(struct cpu *cpu)
{
  int cmd;

  cmd = mmu_read_word_print(SP(0));
  switch(cmd) {
  case 0x0e:
    gemdos_dsetdrv(cpu);
    break;
  case 0x19:
    gemdos_dgetdrv(cpu);
    break;
  case 0x1a:
    gemdos_fsetdta(cpu);
    break;
  case 0x20:
    gemdos_super(cpu);
    break;
  case 0x3b:
    gemdos_dsetpath(cpu);
    break;
  case 0x3d:
    gemdos_fopen(cpu);
    break;
  case 0x3e:
    gemdos_fclose(cpu);
    break;
  case 0x3f:
    gemdos_fread(cpu);
    break;
  case 0x48:
    gemdos_malloc(cpu);
    break;
  case 0x49:
    gemdos_mfree(cpu);
    break;
  case 0x4a:
    gemdos_mshrink(cpu);
    break;
  case 0x4b:
    gemdos_pexec(cpu);
    break;
  case 0x4e:
    gemdos_fsfirst(cpu);
    break;
  case 0x4f:
    gemdos_fsnext(cpu);
    break;
  default:
    printf("CMD: %d [%04x]\n", cmd, cmd);
    exit(-1);
  }
}
