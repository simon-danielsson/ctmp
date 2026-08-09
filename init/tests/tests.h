#ifndef TESTS_H
#define TESTS_H

void _run_tests(void);

#define test_assert(cond)                                                      \
  do {                                                                         \
    if (!(cond)) {                                                             \
      fprintf(stderr, "\033[4;31mFAILURE :: %s:%d (%s)\033[0m\n", __func__,    \
              __LINE__, #cond);                                                \
    } else {                                                                   \
      fprintf(stderr, "SUCCESS :: %s:%d (%s)\n", __func__, __LINE__, #cond);   \
    }                                                                          \
  } while (0)

#endif
