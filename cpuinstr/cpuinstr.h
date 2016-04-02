#ifndef CPUINSTR_H
#define CPUINSTR_H

#include "cpu.h"
#include "mmu.h"

static WORD fetch_word(struct cpu *cpu)
{
  WORD data;

  data = bus_read_word(cpu->pc);
  printf("DEBUG: fetch_word: ADDR: %06x => %04x\n", cpu->pc, data);
  cpu->pc += 2;
  return data;
}

static void set_state(struct cpu *cpu, int state)
{
  printf("DEBUG: set_state: %d => %d\n", cpu->instr_state, state);
  cpu->instr_state = state;
}

/* Write one word to memory at cpu->instr_data_ea_addr,
 * from the word pointer array at the current position
 */
static void write_word(struct cpu *cpu)
{
  LONG addr;
  WORD data;

  /* Predec */
  if(cpu->instr_data_step < 0) {
    cpu->instr_data_ea_addr += cpu->instr_data_step;
  }

  addr = cpu->instr_data_ea_addr;
  data = *((WORD *)cpu->instr_data_word_ptr[cpu->instr_data_word_pos]);

  printf("DEBUG: write_word: Addr: %06x  %04x\n", addr, data);
  
  bus_write_word(addr, data);

  /* Postinc */
  if(cpu->instr_data_step > 0) {
    cpu->instr_data_ea_addr += cpu->instr_data_step;
  }

  cpu->instr_data_word_pos++;
}

/* Read one word from memory at cpu->instr_data_ea_addr and 
 * write it to the word pointer array at the current position
 */
static void read_word(struct cpu *cpu)
{
  LONG addr;
  WORD data;

  addr = cpu->instr_data_ea_addr;
  data = bus_read_word(addr);

  *((WORD *)cpu->instr_data_word_ptr[cpu->instr_data_word_pos]) = data;

  cpu->instr_data_ea_addr += cpu->instr_data_step;
  cpu->instr_data_word_pos++;
}

#endif
