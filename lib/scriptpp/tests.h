#ifndef SENTRY_TESTS_H___
#define SENTRY_TESTS_H___


#ifdef __cplusplus
extern "C" {
#endif

void test_suite(const char *name);

void test_subsuite(const char *name);

int test(const char *name, int result);
int test_str(const char *name, const char *tst, const char *expected);
int test_long(const char *name, long tst, long expected);

void test_score();

#ifdef __cplusplus
}
#endif

#endif
