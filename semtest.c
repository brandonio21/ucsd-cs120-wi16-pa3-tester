#include "../umix.h"
#include "../mykernel3.h"
#include "../aux.h"
#include "../sys.h"
#include <stdio.h>
#include <string.h>

#define BLOCK 0
#define UNBLOCK 1

/* Used to keep track of the previous CALLS Unblock/Block calls */
struct call {
  int valid;
  int calledFunction;
  int arg;
} lastCall;

/* Keep track of failures and tests run */
int failures = 0;
int testFailures = 0;
int tests = 0;

/* debug output */
int debug = 0;

/* Blank out everything regarding the tester */
void initTester() {
  lastCall.valid = 0;
  lastCall.calledFunction = -1;
  failures = 0;
}

/* Alias of above */
void invalidate() {
  initTester();
}

/* Override UMIX functions so the tester can extract info */
void Printf(char *fmt, ...) {
  return;
}


int Unblock(int p) {
  if (debug)
    printf("[DEBUG] [USER] Unblock called with %d\n",p);

  lastCall.valid = 1;
  lastCall.calledFunction = UNBLOCK;
  lastCall.arg = p;
}

int Block(int p) {
  if (debug)
    printf("[DEBUG] [USER] Block called with %d\n",p);

  lastCall.valid = 1;
  lastCall.calledFunction = BLOCK;
  lastCall.arg = p;
}


/* Tester functions to assert Block/Unblock calls */
void assert_block_called_with(int param) {
  if (debug)
    printf("[DEBUG] Expecting last call to be Block with %d\n", param);
  if (lastCall.valid && lastCall.calledFunction == BLOCK && lastCall.arg) {
    if (lastCall.arg != param) {
      printf(">ERR: Expected Block call with %d but called with %d\n",param,
          lastCall.arg);
      failures++;
    }
  } else {
    printf(">ERR: Expected Block call with %d but never called\n", param);
    failures++;
  }
  lastCall.valid = 0;
}

void assert_unblock_called_with(int param) {
  if (debug)
    printf("[DEBUG] Expecting last call to be Unblock with %d\n", param);
  if (lastCall.valid && lastCall.calledFunction == UNBLOCK && lastCall.arg) {
    if (lastCall.arg != param) {
      printf(">ERR: Expected Unblock call with %d but called with %d\n",param,
          lastCall.arg);
      failures++;
    }
  } else {
    printf(">ERR: Expected Unblock call with %d but never called\n", param);
    failures++;
  }
  lastCall.valid = 0;
}


void assert_block_not_called_with() {
  if (debug)
    printf("[DEBUG] Expecting last call to NOT BE Block\n");

  if (lastCall.valid && lastCall.calledFunction == BLOCK) {
    printf(">ERR: Did not expect Block call, but called with %d\n", 
        lastCall.arg);
    failures++;
  }
}


void assert_unblock_not_called_with() {
  if (debug)
    printf("[DEBUG] Expecting last call to NOT BE Unblock\n");

  if (lastCall.valid && lastCall.calledFunction == UNBLOCK) {
    printf(">ERR: Did not expect Unblock call, but called with %d\n", 
        lastCall.arg);
    failures++;
  }
}

/* Test function that outputs small summary */
void test(void (*testfunction)(), char* testName) {
  if (debug) {
    printf("---------------------------------------------\n");
    printf("STARTING %s TEST\n",testName);
  }
  invalidate();
  tests++;
  testfunction();
  printf("%s: ", failures == 0 ? "PASS" : "FAIL");
  printf("%d %s test failures\n",failures,testName);
  if (failures > 0)
    testFailures++;
  if (debug)
    printf("---------------------------------------------\n");
}

/* TESTS GO HERE ------------------- */
void test_fifo_impl() {
  int s = MySeminit(0,0);
  MyWait(1,s);
  assert_block_called_with(1);
  MyWait(2,s);
  assert_block_called_with(2);
  MyWait(3,s);
  assert_block_called_with(3);
  MyWait(4,s);
  assert_block_called_with(4);
  MyWait(5,s);
  assert_block_called_with(5);
  MyWait(6,s);
  assert_block_called_with(6);
  MySignal(0,s);
  assert_unblock_called_with(1);
  MySignal(0,s);
  assert_unblock_called_with(2);
  MySignal(0,s);
  assert_unblock_called_with(3);
  MySignal(0,s);
  assert_unblock_called_with(4);
  MySignal(0,s);
  assert_unblock_called_with(5);
  MySignal(0,s);
  assert_unblock_called_with(6);
  MySignal(0,s);
  assert_unblock_not_called_with();
}
void high_value_sem() {
  int s = MySeminit(0,5);
  MyWait(1,s);
  assert_block_not_called_with();
  MyWait(2,s);
  assert_block_not_called_with();
  MyWait(3,s);
  assert_block_not_called_with();
  MyWait(4,s);
  assert_block_not_called_with();
  MyWait(5,s);
  assert_block_not_called_with();
  MyWait(6,s);
  assert_block_called_with(6);
  MySignal(0,s);
  assert_unblock_called_with(6);
}
void nonlocked_sem_test() {
  int s = MySeminit(1,1);
  MyWait(1,s);
  assert_block_not_called_with();
}

void locked_sem_test_all_wait() {
  int s = MySeminit(0,0);
  MyWait(1,s);
  assert_block_called_with(1);
  MyWait(2,s);
  assert_block_called_with(2);
  MyWait(3,s);
  assert_block_called_with(3);
  MyWait(4,s);
  assert_block_called_with(4);
  MyWait(5,s);
  assert_block_called_with(5);
}

void locked_sem_test() {
  int s = MySeminit(1,1);
  MyWait(1,s);
  assert_block_not_called_with();
  MyWait(2,s);
  assert_block_called_with(2);
  MySignal(1,s);
  assert_unblock_called_with(2);
}

/*
 * tests whether your semaphore wait list can handle every process waiting on it
 * technically if this happens there won't be any processes alive for there to be
 * a deadlock
 */
void test_max_wait_list_fifo() {
    int sem = MySeminit(1, 0);

    // add every process to the wait list
    for (int pid = 1; pid <= MAXPROCS; pid++) {
        MyWait(pid, sem);
        assert_block_called_with(pid);
    }

    for (int pid = 1; pid <= MAXPROCS; pid++) {
        // technically 0 here is invalid as a pid, but it probably doesn't matter?
        MySignal(0, sem);
        assert_unblock_called_with(pid);
    }
}

/* TESTS ARE CALLED HERE ----------- */
void main(int argc, char** argv) {
  if (argc > 1 && strcmp(argv[1], "-v") == 0)
    debug = 1;
  InitSem();
  test(nonlocked_sem_test, "Simple non-wait");
  test(locked_sem_test_all_wait, "Zero-val semaphore 5 procs");
  test(locked_sem_test, "2 proc must wait");
  test(high_value_sem, "High value semaphore");
  test(test_fifo_impl, "FIFO Test");
  test(test_max_wait_list_fifo, "Maxing out process wait list");

  /* final output */
  printf("%d/%d TESTS PASSED\n", tests-testFailures, tests);
}
