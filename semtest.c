#include "../umix.h"
#include "../mykernel3.h"
#include "../aux.h"
#include "../sys.h"
#include <stdio.h>

#define CALLS 10
#define BLOCK 0
#define UNBLOCK 1

/* Used to keep track of the previous CALLS Unblock/Block calls */
struct call {
  int valid;
  int calledFunction;
  int arg;
} callArr[CALLS];
int failures = 0;
int testFailures = 0;
int tests = 0;

/* Blank out everything regarding the tester */
void initTester() {
  for (int i = 0; i < CALLS; i++) {
    callArr[i].valid = 0;
    callArr[i].calledFunction = -1;
  }
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
  for (int i = 0; i < CALLS; i++) {
    if (!callArr[i].valid) {
      callArr[i].valid = 1;
      callArr[i].calledFunction = UNBLOCK;
      callArr[i].arg = p;
      return 0;
    }
  }
}

int Block(int p) {
  for (int i = 0; i < CALLS; i++) {
    if (!callArr[i].valid) {
      callArr[i].valid = 1;
      callArr[i].calledFunction = BLOCK;
      callArr[i].arg = p;
      return 0;
    }
  }
}


/* Tester functions to assert Block/Unblock calls */
void assert_block_called_with(int param) {
  int called = 0;
  for (int i = CALLS-1; i >= 0; i--) {
    if (callArr[i].valid && callArr[i].calledFunction == BLOCK) {
      called = 1;
      if (callArr[i].arg != param) {
        printf(">ERR: Expected Block Call with %d but called with %d\n",param,
            callArr[i].arg);
        failures++;
      }
    }
  }
  if (!called) {
    printf(">ERR: Expected Block call with %d but not called\n",param);
    failures++;
  }
}

void assert_unblock_called_with(int param) {
  int called = 0;
  for (int i = CALLS-1; i >= 0; i--) {
    if (callArr[i].valid && callArr[i].calledFunction == UNBLOCK) {
      called = 1;
      if (callArr[i].arg != param) {
        printf(">ERR: Expected UnBlock Call with %d but called with %d\n",param,
            callArr[i].arg);
        failures++;
      }
    }
  }
  if (!called) {
    printf(">ERR: Expected Unblock call with %d but not called\n",param);
    failures++;
  }
}


void assert_block_not_called_with() {
  for (int i = CALLS-1; i >= 0; i--) {
    if (callArr[i].valid && callArr[i].calledFunction == BLOCK)  {
      printf(">ERR: NonExpected Block Call called with %d\n", callArr[i].arg);
      failures++;
    }
  }
}


void assert_unblock_non_called_with() {
  for (int i = CALLS-1; i >= 0; i--) {
    if (callArr[i].valid && callArr[i].calledFunction == UNBLOCK) {
      printf(">ERR: NonExpected Unblock Call called with %d\n", callArr[i].arg);
      failures++;
    }
  }
}

/* Test function that outputs small summary */
void test(void (*testfunction)(), char* testName) {
  invalidate();
  tests++;
  printf("%s: ", failures == 0 ? "PASS" : "FAIL");
  printf("%d %s test failures\n",failures,testName);
  if (failures > 0)
    testFailures++;
}

/* TESTS GO HERE ------------------- */
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

/* TESTS ARE CALLED HERE ----------- */
void main() {
  InitSem();
  test(nonlocked_sem_test, "Simple non-wait");
  test(locked_sem_test_all_wait, "Zero-val semaphore 5 procs");
  test(locked_sem_test, "2 proc must wait");

  /* final output */
  printf("%d/%d TESTS PASSED\n", tests-testFailures, tests);
}
