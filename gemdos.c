#include "common.h"
#include "cpu.h"
#include "mmu.h"

static void gemdos_pexec(struct cpu *cpu)
{
  int i;
  WORD mode;
  LONG name_addr;
  LONG cmdline_addr;
  LONG env_addr;
  char name[50] = "";
  char cmdline[1024] = "";

  mode = mmu_read_word_print(cpu->a[7]+2);
  name_addr = mmu_read_long_print(cpu->a[7]+4);
  cmdline_addr = mmu_read_long_print(cpu->a[7]+8); /* Also basepage */
  env_addr = mmu_read_long_print(cpu->a[7]+12);

  if(mode != 4 && mode != 5) {
    for(i=0;i<50;i++) {
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

  stack = mmu_read_long_print(cpu->a[7]+2);
  printf("Super($%06x)\n", stack);
}

static void gemdos_mshrink(struct cpu *cpu)
{
  LONG block;
  LONG newsiz;

  block = mmu_read_long_print(cpu->a[7]+4);
  newsiz = mmu_read_long_print(cpu->a[7]+8);
  printf("Mshrink($%06x, %d)\n", block, newsiz);
}

static void gemdos_malloc(struct cpu *cpu)
{
  LONG size;

  size = mmu_read_long_print(cpu->a[7]+2);
  printf("Malloc(%d)\n", size);
}

void gemdos_print(struct cpu *cpu)
{
  int cmd;

  cmd = mmu_read_word_print(cpu->a[7]);
  switch(cmd) {
  case 0x20:
    gemdos_super(cpu);
    break;
  case 0x48:
    gemdos_malloc(cpu);
    break;
  case 0x4a:
    gemdos_mshrink(cpu);
    break;
  case 0x4b:
    gemdos_pexec(cpu);
    break;
  default:
    printf("CMD: %d [%04x]\n", cmd, cmd);
  }
}
