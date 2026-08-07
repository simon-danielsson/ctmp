#include "../src/main.h"

static void test1(void) { printf("test 1"); }
static void test2(void) { printf("test 2"); }
static void test3(void) { printf("test 3"); }

typedef void (*test_fn)(void);

static const test_fn tests[] = {
    test1,
    test2,
    test3,
    NULL,
};

void _run_tests(void) {
    uint i = 0;
    while (tests[i]) {
        tests[i]();
        i++;
    }
    exit(0);
}
