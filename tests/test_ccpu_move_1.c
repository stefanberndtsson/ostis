#include "test_main.h"
#include "test_helpers.h"
#include "mmu.h"
#include "cpu.h"

#define TEST_HOOK_VERIFY_TEST 1
#define TEST_HOOK_SETUP_TEST  6

static uint64_t tmp_cycles;
static int test_num = 0;

static void test_ccpu_move_1_hook_init(struct cpu *cpu)
{
  TEST_CASE_START("MOVE", "Clocked CPU - MOVE instruction - Register to EA");
}

static void test_ccpu_move_1_setup_test1(struct cpu *cpu)
{
  cpu->d[0] = 0x55aa;
  cpu->d[1] = 0x12340000;
  cpu->a[0] = 0xaa55;
  cpu->d[2] = 0x87650000;
}

static void test_ccpu_move_1_setup_test2(struct cpu *cpu)
{
  cpu->d[0] = 0xffffff80;
  cpu->a[0] = 0x10000;
}

static void test_ccpu_move_1_setup_test3(struct cpu *cpu)
{
  cpu->d[0] = 0xffffff80;
  cpu->a[0] = 0x10000;
}

static void test_ccpu_move_1_setup_test4(struct cpu *cpu)
{
  cpu->d[0] = 0xffffff80;
  cpu->a[0] = 0x10010;
}

static void test_ccpu_move_1_setup_test5(struct cpu *cpu)
{
  cpu->d[0] = 0xffffff80;
  cpu->a[0] = 0x10010;
}

static void test_ccpu_move_1_setup_test6(struct cpu *cpu)
{
  cpu->d[0] = 0xffffff80;
  cpu->a[0] = 0x10010;
  cpu->d[1] = 0xfffffffe;
}

static void test_ccpu_move_1_setup_test7(struct cpu *cpu)
{
  cpu->d[0] = 0xffffff80;
}

static void test_ccpu_move_1_setup_test8(struct cpu *cpu)
{
  cpu->d[0] = 0xffffff80;
}

static void test_ccpu_move_1_verify_test1(struct cpu *cpu)
{
  uint64_t cycles;

  cycles = cpu->cycle - tmp_cycles;

  TEST_START(1, "MOVE.W Rx,Dy");
  EXPECT_EQ(LONG, cpu->d[1], 0x123455aa, "D1");
  EXPECT_EQ(LONG, cpu->d[2], 0x8765aa55, "D2");
  EXPECT_EQ(INT64, cycles, 8L, "cycle count");
  TEST_END_OK;
}

static void test_ccpu_move_1_verify_test2(struct cpu *cpu)
{
  WORD v1;
  uint64_t cycles;

  cycles = cpu->cycle - tmp_cycles;

  v1 = bus_read_word_print(0x10000);

  TEST_START(2, "MOVE.W D0,(A0)");
  EXPECT_EQ(WORD, v1, 0xff80, "[0x10000]");
  EXPECT_EQ(ADDR, cpu->a[0], 0x10000, "A0");
  EXPECT_EQ(INT64, cycles, 8L, "cycle count");
  TEST_END_OK;
}

static void test_ccpu_move_1_verify_test3(struct cpu *cpu)
{
  WORD v1;
  uint64_t cycles;

  cycles = cpu->cycle - tmp_cycles;

  v1 = bus_read_word_print(0x10000);

  TEST_START(3, "MOVE.W D0,(A0)+");
  EXPECT_EQ(WORD, v1, 0xff80, "[0x10000]");
  EXPECT_EQ(ADDR, cpu->a[0], 0x10002, "A0");
  EXPECT_EQ(INT64, cycles, 8L, "cycle count");
  TEST_END_OK;
}

static void test_ccpu_move_1_verify_test4(struct cpu *cpu)
{
  WORD v1;
  uint64_t cycles;

  cycles = cpu->cycle - tmp_cycles;

  v1 = bus_read_word_print(0x1000e);

  TEST_START(4, "MOVE.W D0,-(A0)");
  EXPECT_EQ(WORD, v1, 0xff80, "[0x1000e]");
  EXPECT_EQ(ADDR, cpu->a[0], 0x1000e, "A0");
  EXPECT_EQ(INT64, cycles, 8L, "cycle count");
  TEST_END_OK;
}

static void test_ccpu_move_1_verify_test5(struct cpu *cpu)
{
  WORD v1;
  uint64_t cycles;

  cycles = cpu->cycle - tmp_cycles;

  v1 = bus_read_word_print(0x10014);

  TEST_START(5, "MOVE.W D0,4(A0)");
  EXPECT_EQ(WORD, v1, 0xff80, "[0x10014]");
  EXPECT_EQ(ADDR, cpu->a[0], 0x10010, "A0");
  EXPECT_EQ(INT64, cycles, 12L, "cycle count");
  TEST_END_OK;
}

static void test_ccpu_move_1_verify_test6(struct cpu *cpu)
{
  WORD v1;
  uint64_t cycles;

  cycles = cpu->cycle - tmp_cycles;

  v1 = bus_read_word_print(0x10012);

  TEST_START(6, "MOVE.W D0,4(A0,D1.W)");
  EXPECT_EQ(WORD, v1, 0xff80, "[0x10012]");
  EXPECT_EQ(ADDR, cpu->a[0], 0x10010, "A0");
  EXPECT_EQ(INT64, cycles, 16L, "cycle count");
  TEST_END_OK;
}

static void test_ccpu_move_1_verify_test7(struct cpu *cpu)
{
  WORD v1;
  uint64_t cycles;

  cycles = cpu->cycle - tmp_cycles;

  v1 = bus_read_word_print(0x4000);

  TEST_START(7, "MOVE.W D0,$4000.W");
  EXPECT_EQ(WORD, v1, 0xff80, "[0x4000]");
  EXPECT_EQ(INT64, cycles, 12L, "cycle count");
  TEST_END_OK;
}

static void test_ccpu_move_1_verify_test8(struct cpu *cpu)
{
  WORD v1;
  uint64_t cycles;

  cycles = cpu->cycle - tmp_cycles;

  v1 = bus_read_word_print(0x14000);

  TEST_START(8, "MOVE.W D0,$14000.W");
  EXPECT_EQ(WORD, v1, 0xff80, "[0x14000]");
  EXPECT_EQ(INT64, cycles, 16L, "cycle count");
  TEST_END_OK;
}

static void test_ccpu_move_1_hook_exit(struct cpu *cpu)
{
  TEST_CASE_END;
  exit(0);
}

static void test_ccpu_move_1_hook_reset_cycles(struct cpu *cpu)
{
  tmp_cycles = cpu->cycle;
}

static void test_ccpu_move_1_verify_test_dispatch(struct cpu *cpu)
{
  switch(test_num) {
  case 1:
    test_ccpu_move_1_verify_test1(cpu);
    break;
  case 2:
    test_ccpu_move_1_verify_test2(cpu);
    break;
  case 3:
    test_ccpu_move_1_verify_test3(cpu);
    break;
  case 4:
    test_ccpu_move_1_verify_test4(cpu);
    break;
  case 5:
    test_ccpu_move_1_verify_test5(cpu);
    break;
  case 6:
    test_ccpu_move_1_verify_test6(cpu);
    break;
  case 7:
    test_ccpu_move_1_verify_test7(cpu);
    break;
  case 8:
    test_ccpu_move_1_verify_test8(cpu);
    break;
  }
}

static void test_ccpu_move_1_setup_test_dispatch(struct cpu *cpu)
{
  test_num++;
  switch(test_num) {
  case 1:
    test_ccpu_move_1_setup_test1(cpu);
    break;
  case 2:
    test_ccpu_move_1_setup_test2(cpu);
    break;
  case 3:
    test_ccpu_move_1_setup_test3(cpu);
    break;
  case 4:
    test_ccpu_move_1_setup_test4(cpu);
    break;
  case 5:
    test_ccpu_move_1_setup_test5(cpu);
    break;
  case 6:
    test_ccpu_move_1_setup_test6(cpu);
    break;
  case 7:
    test_ccpu_move_1_setup_test7(cpu);
    break;
  case 8:
    test_ccpu_move_1_setup_test8(cpu);
    break;
  }

  tmp_cycles = cpu->cycle;
}

void test_ccpu_move_1_init()
{
  struct test_case *test_case;
  
  test_case = test_case_alloc("ccpu_move_1");

  test_case->hooks[TEST_HOOK_INIT] = test_ccpu_move_1_hook_init;
  test_case->hooks[TEST_HOOK_VERIFY_TEST] = test_ccpu_move_1_verify_test_dispatch;
  test_case->hooks[TEST_HOOK_SETUP_TEST] = test_ccpu_move_1_setup_test_dispatch;
  test_case->hooks[TEST_HOOK_EXIT] = test_ccpu_move_1_hook_exit;
  
  test_case_register(test_case);
}
