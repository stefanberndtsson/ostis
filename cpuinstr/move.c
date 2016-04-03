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
#define MOVE_WRITE    3
#define MOVE_DELAY    4
#define MOVE_DELAYED  5

#define MOVE_REGTOREG(op) ((EA_MODE_DREG(op) || EA_MODE_AREG(op)) && EA_MODE_DREG(EA_HIGH(op)))

/* Special case, register to register only, no need to create
 * memory access structures
 */
static void move_regtoreg(struct cpu *cpu, WORD op)
{
  int ea_dst = EA_HIGH(op);
  int srcr;
  int dstr;

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
}

static void prepare_src(struct cpu *cpu, WORD op)
{
  int rnum;
  int ea_dst = EA_HIGH(op);
  int word_count = 0;

  if(EA_MODE_DREG(op)) {
    rnum = EA_MODE_REG(op);
  } else if(EA_MODE_AREG(op)) {
    rnum = EA_MODE_REG(op) + 8;
  }
  
  /* MOVE.x Rx,MEM */
  if(EA_MODE_DREG(op) || EA_MODE_AREG(op)) {
    if(MOVE_SIZE_B(op) || MOVE_SIZE_W(op)) {
      cpu->instr_data_word_ptr[word_count++] = REG_LWORD(cpu, rnum);
    }
    if(MOVE_SIZE_L(op)) {
      if(EA_MODE_ADEC(ea_dst)) {
        cpu->instr_data_word_ptr[word_count++] = REG_LWORD(cpu, rnum);
        cpu->instr_data_word_ptr[word_count++] = REG_HWORD(cpu, rnum);
      } else {
        cpu->instr_data_word_ptr[word_count++] = REG_HWORD(cpu, rnum);
        cpu->instr_data_word_ptr[word_count++] = REG_LWORD(cpu, rnum);
      }
    }
    cpu->instr_data_word_count = word_count;
    cpu->instr_data_word_pos = 0;
    set_state(cpu, MOVE_WRITE);
  }
}

static void prepare_dst(struct cpu *cpu, WORD op)
{
  int ea_dst = EA_HIGH(op);

  if(EA_MODE_AMEM(ea_dst) || EA_MODE_AINC(ea_dst) || EA_MODE_ADEC(ea_dst) ||
     EA_MODE_AOFF(ea_dst) || EA_MODE_AROF(ea_dst) || EA_MODE_SHRT(ea_dst) ||
     EA_MODE_LONG(ea_dst)) {
    printf("DEBUG: AMEM|AINC|ADEC|AOFF|AROF|SHRT|LONG\n");
    cpu->instr_data_ea_reg = EA_MODE_REG(ea_dst);
    cpu->instr_data_ea_addr = ea_addr(cpu, MOVE_SIZE(op), ea_dst, cpu->instr_data_fetch);
    printf("DEBUG: Addr: %d %06x\n", cpu->instr_data_ea_reg, cpu->instr_data_ea_addr);
    cpu->instr_data_step = 2;
  }

  if(EA_MODE_AINC(ea_dst)) {
    cpu->a[EA_MODE_REG(ea_dst)] += MOVE_SIZE(op);
  }

  if(EA_MODE_ADEC(ea_dst)) {
    cpu->instr_data_step = -2;
    cpu->a[EA_MODE_REG(ea_dst)] -= MOVE_SIZE(op);
  }
}

/* Normal cases, register to memory, memory to register and memory to memory
 * These cases require setup before execution.
 */
static void prepare(struct cpu *cpu, WORD op)
{
  prepare_dst(cpu, op);
  prepare_src(cpu, op);
}

static void move(struct cpu *cpu, WORD op)
{
  int tmp;

  /* Special cases where there is a 2c internal delay before
   * fetching EA for instruction
   */
  if(cpu->instr_state == INSTR_STATE_NONE) {
    if(EA_MODE_AROF(EA_HIGH(op))) {
      set_state(cpu, MOVE_DELAY);
    }
  }
  
  switch(cpu->instr_state) {

    /* Initial state (or delayed by 2c)
     * Calculate number of words of EA to fetch
     * If no EA is to be fetched, run parse immediately.
     * Wait 4 cycles
     */
  case INSTR_STATE_NONE:
  case MOVE_DELAYED:
    printf("DEBUG: Initial: %d %d\n", EA_FETCH_COUNT(op), EA_FETCH_COUNT(EA_HIGH(op)));
    if(MOVE_REGTOREG(op)) {
      move_regtoreg(cpu, op);
    } else {
      tmp = EA_FETCH_COUNT(op);
      if(tmp < 0) tmp = 0;
      cpu->instr_data_word_count = tmp;
      tmp = EA_FETCH_COUNT(EA_HIGH(op));
      if(tmp < 0) tmp = 0;
      cpu->instr_data_word_count += tmp;
      cpu->instr_data_word_pos = 0;
      /* EA_FETCH if there are words to fetch, otherwise,
       * prepare data, and let preparation decide where to go
       */
      if(cpu->instr_data_word_count > 0) {
        set_state(cpu, MOVE_EA_FETCH);
      } else {
        prepare(cpu, op);
      }
    }
    ADD_CYCLE(4);
    break;
  case MOVE_EA_FETCH:
    cpu->instr_data_fetch[cpu->instr_data_word_pos++] = fetch_word(cpu);
    /* Still EA to fetch, keep current state, otherwise move on to preparation */
    if(cpu->instr_data_word_pos >= cpu->instr_data_word_count) {
      prepare(cpu, op);
    }
    break;
  case MOVE_WRITE:
    write_word(cpu);
    set_state(cpu, INSTR_STATE_FINISHED);
    ADD_CYCLE(4);
    break;
  case MOVE_DELAY:
    ADD_CYCLE(2);
    set_state(cpu, MOVE_DELAYED);
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
