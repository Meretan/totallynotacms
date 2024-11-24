#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

#include "tests.h"

static long totals;
static long passed;
static long failed;
static const char *currentsuite;

static void TestPassed(const char *name)
{
  printf("@@@ <passed> %s\n", name);
  totals++;
  passed++;
}

static void TestFailed(const char *name, const char *format, ...)
{
  va_list ap;
  va_start(ap, format);
  printf("@@@ <FAILED> %s [", name);
  vprintf(format, ap);
  printf("]\n");
  va_end(ap);
  totals++;
  failed++;
}

void test_suite(const char *name)
{
  totals = 0;
  passed = 0;
  failed = 0;
  currentsuite = strdup(name);
  printf("@@@ @@@ @@@ %s @@@\n", name);
}

void test_subsuite(const char *name)
{
  printf("@@@ ---- %s ----\n", name);
}

int test(const char *name, int result)
{
  if(result) {
    TestPassed(name);
    return 1;
  } else {
    TestFailed(name, "");
    return 0;
  }
}

int test_str(const char *name, const char *tst, const char *expected)
{
  if(tst == expected || (tst && expected && 0 == strcmp(tst, expected))) {
    TestPassed(name);
    return 1;
  } else {
    TestFailed(name, "result: \"%s\", expected: \"%s\"",
                     tst, expected);
    return 0;
  }
}

int test_long(const char *name, long tst, long expected)
{
  if(tst == expected) {
    TestPassed(name);
    return 1;
  } else {
    TestFailed(name, "result: %d, expected: %d",
                     tst, expected);
    return 0;
  }
}


void test_score()
{
  printf("@@@ -----------------\n");
  printf("@@@ * Suite name: %s\n", currentsuite);
  printf("@@@ * Passed: %ld\n", passed);
  printf("@@@ * Failed: %ld\n", failed);
  printf("@@@ * Totals: %ld\n", totals);
  if(failed) {
    printf("@@@ ! SOME TESTS FAILED in the suite %s\n", currentsuite);
  }
  free((void*)currentsuite);
  currentsuite = "**deleted**";
}


