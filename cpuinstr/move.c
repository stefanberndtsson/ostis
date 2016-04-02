#include "common.h"
#include "cpu.h"
#include "cpuinstr.h"
#include "cprint.h"
#include "mmu.h"
#include "ea.h"

#define MOVE_SIZE(op) (((op>>12) > 1) ? (((op>>12) == 3) ? 2 : 4) : 1)
#define MOVE_SIZE_B(op) (MOVE_SIZE(op) == 1)
#define MOVE_SIZE_W(op) (MOVE_SIZE(op) == 2)
#define MOVE_SIZE_L(op) (MOVE_SIZE(op) == 4)
#define MOVE_EA_FETCH 1
#define MOVE_PARSE    2

static void parse(struct cpu *cpu, WORD op)
{
  int ea_dst = EA_HIGH(op);
  int srcr;
  int dstr;

  /* Special case, register to register only, no need to create
   * memory access structures
   */
  if((EA_MODE_DREG(op) || EA_MODE_AREG(op)) && EA_MODE_DREG(ea_dst)) {
    if(EA_MODE_DREG(op)) {
      srcr = cpu->d[EA_MODE_REG(op)];
    } else if(EA_MODE_AREG(op)) {
      srcr = cpu->a[EA_MODE_REG(op)];
    }
    dstr = EA_MODE_REG(ea_dst);
    if(MOVE_SIZE_B(op)) {
      cpu->d[dstr] = (cpu->d[dstr]&0xffffff00)|(srcr&0xff);
    } else if(MOVE_SIZE_W(op)) {
      cpu->d[dstr] = (cpu->d[dstr]&0xffff0000)|(srcr&0xffff);
    } else if(MOVE_SIZE_L(op)) {
      cpu->d[dstr] = srcr;
    }
    set_state(cpu, INSTR_STATE_FINISHED);
    return;
  }

  /* Destination to data register unimplemented for anything other than
   * reg->reg for the moment
   */
  if(EA_MODE_DREG(ea_dst)) return;
  
  /* Destination is going on the bus, prepare the write queue with the
   * appropriate data
   */
  if(EA_MODE_AOFF(ea_dst)) {
    cpu->instr_data_ea_reg = EA_MODE_REG(ea_dst);
    cpu->instr_data_ea_addr = ea_addr(cpu, MOVE_SIZE(op), op, cpu->instr_data_fetch);
  }

  /* MOVE.x Dx,MEM */
  if(EA_MODE_DREG(op)) {
    if(MOVE_SIZE_B(op) || MOVE_SIZE_W(op)) {
      cpu->instr_data_word_ptr[cpu->instr_data_word_pos] = DREG_LWORD(cpu, EA_MODE_REG(op));
    }
    if(MOVE_SIZE_L(op)) {
      if(EA_MODE_ADEC(ea_dst)) {
        cpu->instr_data_word_ptr[cpu->instr_data_word_pos] = DREG_LWORD(cpu, EA_MODE_REG(op));
      }
    }
  }
}

static void move(struct cpu *cpu, WORD op)
{
  switch(cpu->instr_state) {

    /* Initial state
     * Calculate number of words of EA to fetch
     * If no EA is to be fetched, run parse immediately.
     * Wait 4 cycles
     */
  case INSTR_STATE_NONE:
    cpu->instr_data_word_count = EA_FETCH_COUNT(op);
    cpu->instr_data_word_pos = 0;
    /* EA_FETCH if there are words to fetch, otherwise,
     * parse instruction, and start executing.
     */
    if(cpu->instr_data_word_count > 0) {
      set_state(cpu, MOVE_EA_FETCH);
    } else {
      parse(cpu, op);
      /* Next state is set by parse */
    }
    ADD_CYCLE(4);
    break;
  case MOVE_EA_FETCH:
    printf("DEBUG: MOVE_EA_FETCH unimplemented\n");
    set_state(cpu, INSTR_STATE_FINISHED);
    break;
  }
}

static struct cprint *move_print(LONG addr, WORD op)
{
  int src,dst,s;
  struct cprint *ret;

  ret = cprint_alloc(addr);

  src = op&0x3f;
  dst = MODESWAP((op&0xfc0)>>6);
  s = (op&0x3000)>>12;
  if(s == 1) s = 0;
  if(s == 3) s = 1;

  switch(s) {
  case 0:
    strcpy(ret->instr, "MOVE.B");
    break;
  case 1:
    strcpy(ret->instr, "MOVE.W");
    break;
  case 2:
    strcpy(ret->instr, "MOVE.L");
    break;
  }
  ea_print(ret, src, s);
  strcat(ret->data, ",");
  ea_print(ret, dst, s);
  
  return ret;
}

void move_instr_init(void *instr[], void *print[])
{
  int i;

  for(i=0x1000;i<0x4000;i++) {
    if(ea_valid(i&0x3f, EA_INVALID_NONE) && 
       ea_valid(MODESWAP((i&0xfc0)>>6), EA_INVALID_A|EA_INVALID_DST)) {
      instr[i] = move;
      print[i] = move_print;
    }
  }
}
