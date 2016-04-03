#include "test_main.h"
#include "test_helpers.h"
#include "mmu.h"
#include "cpu.h"

#define TEST_HOOK_VERIFY_TEST 1
#define TEST_HOOK_SETUP_TEST  6

static uint64_t tmp_cycles;
static int test_num = 0;

static void test_ccpu_move_2_hook_init(struct cpu *cpu)
{
  TEST_CASE_START("MOVE", "Clocked CPU - MOVE instruction - EA to register");
}

static void test_ccpu_move_2_setup_test1(struct cpu *cpu)
{
  bus_write_word(0x10000, 0x55aa);
  cpu->a[0] = 0x10000;
  cpu->d[0] = 0;
}

static void test_ccpu_move_2_setup_test2(struct cpu *cpu)
{
  bus_write_word(0x10000, 0xaa55);
  cpu->a[0] = 0x10000;
  cpu->d[0] = 0;
}

static void test_ccpu_move_2_setup_test3(struct cpu *cpu)
{
  bus_write_word(0x10010, 0x1234);
  cpu->a[0] = 0x10010;
  cpu->d[0] = 0;
}

static void test_ccpu_move_2_setup_test4(struct cpu *cpu)
{
  bus_write_word(0x10004, 0x1234);
  cpu->a[0] = 0x10000;
  cpu->d[0] = 0;
}

static void test_ccpu_move_2_setup_test5(struct cpu *cpu)
{
  bus_write_word(0x10022, 0x4321);
  cpu->a[0] = 0x10020;
  cpu->d[0] = 0;
  cpu->d[1] = 0xfffffffe;
}

static void test_ccpu_move_2_setup_test6(struct cpu *cpu)
{
  bus_write_word(0x4000, 0x9988);
  cpu->d[0] = 0;
}

static void test_ccpu_move_2_setup_test7(struct cpu *cpu)
{
  bus_write_word(0x14000, 0x8899);
  cpu->d[0] = 0;
}

static void test_ccpu_move_2_setup_test8(struct cpu *cpu)
{
  cpu->d[0] = 0;
}

static void test_ccpu_move_2_setup_test9(struct cpu *cpu)
{
  cpu->d[0] = 0;
  cpu->d[1] = 0xfffffffe;
}

static void test_ccpu_move_2_setup_test10(struct cpu *cpu)
{
  cpu->d[0] = 0;
}

static void test_ccpu_move_2_verify_test1(struct cpu *cpu)
{
  uint64_t cycles;

  cycles = cpu->cycle - tmp_cycles;

  TEST_START(1, "MOVE.W (A0),D0");
  EXPECT_EQ(LONG, cpu->d[0], 0x000055aa, "D0");
  EXPECT_EQ(LONG, cpu->a[0], 0x00010000, "A0");
  EXPECT_EQ(INT64, cycles, 8L, "cycle count");
  TEST_END_OK;
}

static void test_ccpu_move_2_verify_test2(struct cpu *cpu)
{
  uint64_t cycles;

  cycles = cpu->cycle - tmp_cycles;

  TEST_START(1, "MOVE.W (A0)+,D0");
  EXPECT_EQ(LONG, cpu->d[0], 0x0000aa55, "D0");
  EXPECT_EQ(LONG, cpu->a[0], 0x00010002, "A0");
  EXPECT_EQ(INT64, cycles, 8L, "cycle count");
  TEST_END_OK;
}

static void test_ccpu_move_2_verify_test3(struct cpu *cpu)
{
  uint64_t cycles;

  cycles = cpu->cycle - tmp_cycles;

  TEST_START(1, "MOVE.W -(A0),D0");
  EXPECT_EQ(LONG, cpu->d[0], 0x00001234, "D0");
  EXPECT_EQ(LONG, cpu->a[0], 0x0001000e, "A0");
  EXPECT_EQ(INT64, cycles, 8L, "cycle count");
  TEST_END_OK;
}

static void test_ccpu_move_2_verify_test4(struct cpu *cpu)
{
  uint64_t cycles;

  cycles = cpu->cycle - tmp_cycles;

  TEST_START(1, "MOVE.W 4(A0),D0");
  EXPECT_EQ(LONG, cpu->d[0], 0x00001234, "D0");
  EXPECT_EQ(LONG, cpu->a[0], 0x00010000, "A0");
  EXPECT_EQ(INT64, cycles, 8L, "cycle count");
  TEST_END_OK;
}

static void test_ccpu_move_2_verify_test5(struct cpu *cpu)
{
  uint64_t cycles;

  cycles = cpu->cycle - tmp_cycles;

  TEST_START(1, "MOVE.W 4(A0,D1.W),D0");
  EXPECT_EQ(LONG, cpu->d[0], 0x00004321, "D0");
  EXPECT_EQ(LONG, cpu->a[0], 0x00010020, "A0");
  EXPECT_EQ(INT64, cycles, 8L, "cycle count");
  TEST_END_OK;
}

static void test_ccpu_move_2_verify_test6(struct cpu *cpu)
{
  uint64_t cycles;

  cycles = cpu->cycle - tmp_cycles;

  TEST_START(1, "MOVE.W $4000.W,D0");
  EXPECT_EQ(LONG, cpu->d[0], 0x00009988, "D0");
  EXPECT_EQ(INT64, cycles, 8L, "cycle count");
  TEST_END_OK;
}

static void test_ccpu_move_2_verify_test7(struct cpu *cpu)
{
  uint64_t cycles;

  cycles = cpu->cycle - tmp_cycles;

  TEST_START(1, "MOVE.W $14000.L,D0");
  EXPECT_EQ(LONG, cpu->d[0], 0x00008899, "D0");
  EXPECT_EQ(INT64, cycles, 8L, "cycle count");
  TEST_END_OK;
}

static void test_ccpu_move_2_verify_test8(struct cpu *cpu)
{
  uint64_t cycles;

  cycles = cpu->cycle - tmp_cycles;

  TEST_START(1, "MOVE.W 4(PC),D0");
  EXPECT_EQ(LONG, cpu->d[0], 0x000055aa, "D0");
  EXPECT_EQ(INT64, cycles, 8L, "cycle count");
  TEST_END_OK;
}

static void test_ccpu_move_2_verify_test9(struct cpu *cpu)
{
  uint64_t cycles;

  cycles = cpu->cycle - tmp_cycles;

  TEST_START(1, "MOVE.W 4(PC,D1.W),D0");
  EXPECT_EQ(LONG, cpu->d[0], 0x000055aa, "D0");
  EXPECT_EQ(INT64, cycles, 8L, "cycle count");
  TEST_END_OK;
}

static void test_ccpu_move_2_verify_test10(struct cpu *cpu)
{
  uint64_t cycles;

  cycles = cpu->cycle - tmp_cycles;

  TEST_START(1, "MOVE.W #$11112345,D0");
  EXPECT_EQ(LONG, cpu->d[0], 0x11112345, "D0");
  EXPECT_EQ(INT64, cycles, 8L, "cycle count");
  TEST_END_OK;
}

static void test_ccpu_move_2_hook_exit(struct cpu *cpu)
{
  TEST_CASE_END;
  exit(0);
}

static void test_ccpu_move_2_hook_reset_cycles(struct cpu *cpu)
{
  tmp_cycles = cpu->cycle;
}

static void test_ccpu_move_2_verify_test_dispatch(struct cpu *cpu)
{
  switch(test_num) {
  case 1:
    test_ccpu_move_2_verify_test1(cpu);
    break;
  case 2:
    test_ccpu_move_2_verify_test2(cpu);
    break;
  case 3:
    test_ccpu_move_2_verify_test3(cpu);
    break;
  case 4:
    test_ccpu_move_2_verify_test4(cpu);
    break;
  case 5:
    test_ccpu_move_2_verify_test5(cpu);
    break;
  case 6:
    test_ccpu_move_2_verify_test6(cpu);
    break;
  case 7:
    test_ccpu_move_2_verify_test7(cpu);
    break;
  case 8:
    test_ccpu_move_2_verify_test8(cpu);
    break;
  case 9:
    test_ccpu_move_2_verify_test9(cpu);
    break;
  case 10:
    test_ccpu_move_2_verify_test10(cpu);
    break;
  }
}

static void test_ccpu_move_2_setup_test_dispatch(struct cpu *cpu)
{
  test_num++;
  switch(test_num) {
  case 1:
    test_ccpu_move_2_setup_test1(cpu);
    break;
  case 2:
    test_ccpu_move_2_setup_test2(cpu);
    break;
  case 3:
    test_ccpu_move_2_setup_test3(cpu);
    break;
  case 4:
    test_ccpu_move_2_setup_test4(cpu);
    break;
  case 5:
    test_ccpu_move_2_setup_test5(cpu);
    break;
  case 6:
    test_ccpu_move_2_setup_test6(cpu);
    break;
  case 7:
    test_ccpu_move_2_setup_test7(cpu);
    break;
  case 8:
    test_ccpu_move_2_setup_test8(cpu);
    break;
  case 9:
    test_ccpu_move_2_setup_test9(cpu);
    break;
  case 10:
    test_ccpu_move_2_setup_test10(cpu);
    break;
  }

  tmp_cycles = cpu->cycle;
}

void test_ccpu_move_2_init()
{
  struct test_case *test_case;
  
  test_case = test_case_alloc("ccpu_move_2");

  test_case->hooks[TEST_HOOK_INIT] = test_ccpu_move_2_hook_init;
  test_case->hooks[TEST_HOOK_VERIFY_TEST] = test_ccpu_move_2_verify_test_dispatch;
  test_case->hooks[TEST_HOOK_SETUP_TEST] = test_ccpu_move_2_setup_test_dispatch;
  test_case->hooks[TEST_HOOK_EXIT] = test_ccpu_move_2_hook_exit;
  
  test_case_register(test_case);
}
