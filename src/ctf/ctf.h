#ifndef CTF_H
#define CTF_H

#ifndef CTF_EA_ALIASES
#define CTF_EA_ALIASES CTF_ON
#endif

#ifndef CTF_MOCK_ALIASES
#define CTF_MOCK_ALIASES CTF_ON
#endif

#ifndef CTF_OTHER_ALIASES
#define CTF_OTHER_ALIASES CTF_ON
#endif

#ifdef assert
#undef assert
#endif

include(`base.m4')
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define CTF_CONST_GROUP_SIZE 128
#define CTF_CONST_SIGNAL_STACK_SIZE 1024

struct ctf__state {
  int status;
  int line;
  const char *file;
  char *msg;
  uintmax_t msg_size;
  uintmax_t msg_capacity;
};
struct ctf__states {
  struct ctf__state *states;
  uintmax_t size;
  uintmax_t capacity;
};
struct ctf__test_element {
  int8_t issubtest;
  union {
    struct ctf__subtest *subtest;
    struct ctf__states *states;
  } el;
};
struct ctf__subtest {
  struct ctf__test_element *elements;
  uintmax_t size;
  uintmax_t capacity;
  const char *name;
  struct ctf__subtest *parent;
  int status;
};
struct ctf__test {
  void (*const f)(void);
  const char *name;
};
struct ctf__group {
  const struct ctf__test *tests;
  uintmax_t tests_size;
  void (*setup)(void);
  void (*teardown)(void);
  void (*test_setup)(void);
  void (*test_teardown)(void);
  const char *name;
};
struct ctf__stats {
  uintmax_t groups_passed;
  uintmax_t groups_failed;
  uintmax_t tests_passed;
  uintmax_t tests_failed;
  uintmax_t asserts_passed;
  uintmax_t asserts_failed;
  uintmax_t expects_passed;
  uintmax_t expects_failed;
};
struct ctf__thread_data {
  struct ctf__test_element *test_elements;
  uintmax_t test_elements_size;
  uintmax_t test_elements_capacity;
  struct ctf__subtest *subtest_current;
  struct ctf__mock **mock_reset_stack;
  uintmax_t mock_reset_stack_size;
  uintmax_t mock_reset_stack_capacity;
  struct ctf__stats stats;
};

extern struct ctf__thread_data *restrict ctf__thread_data;
extern int ctf_exit_code;
extern pthread_key_t ctf__thread_index;
extern char ctf_signal_altstack[CTF_CONST_SIGNAL_STACK_SIZE];
extern int ctf__opt_threads;
extern int ctf__opt_cleanup;

#define CTF_TEST(test_name)                                    \
  static void ctf__test_fn_##test_name(void);                  \
  static const char ctf__test_name_##test_name[] = #test_name; \
  const struct ctf__test test_name = {                         \
    .name = ctf__test_name_##test_name,                        \
    .f = ctf__test_fn_##test_name,                             \
  };                                                           \
  void ctf__test_fn_##test_name(void)
#define CTF_TEST_STATIC(test_name)                             \
  static void ctf__test_fn_##test_name(void);                  \
  static const char ctf__test_name_##test_name[] = #test_name; \
  static const struct ctf__test test_name = {                  \
    .name = ctf__test_name_##test_name,                        \
    .f = ctf__test_fn_##test_name,                             \
  };                                                           \
  static void ctf__test_fn_##test_name(void)
#define CTF_TEST_EXTERN(name) extern const struct ctf__test name;
#define CTF_GROUP(group_name)                                              \
  static const char ctf__group_name_##group_name[] = #group_name;          \
  static const struct ctf__test                                            \
    ctf__group_arr_##group_name[CTF_CONST_GROUP_SIZE];                     \
  static void (*ctf__group_setup_p_##group_name)(void);                    \
  static void (*ctf__group_teardown_p_##group_name)(void);                 \
  static void (*ctf__group_test_setup_p_##group_name)(void);               \
  static void (*ctf__group_test_teardown_p_##group_name)(void);            \
  static void ctf__group_setup_##group_name(void) {                        \
    if(ctf__group_setup_p_##group_name) ctf__group_setup_p_##group_name(); \
  }                                                                        \
  static void ctf__group_teardown_##group_name(void) {                     \
    if(ctf__group_teardown_p_##group_name)                                 \
      ctf__group_teardown_p_##group_name();                                \
  }                                                                        \
  static void ctf__group_test_setup_##group_name(void) {                   \
    if(ctf__group_test_setup_p_##group_name)                               \
      ctf__group_test_setup_p_##group_name();                              \
  }                                                                        \
  static void ctf__group_test_teardown_##group_name(void) {                \
    if(ctf__group_test_teardown_p_##group_name)                            \
      ctf__group_test_teardown_p_##group_name();                           \
  }                                                                        \
  const struct ctf__group group_name = {                                   \
    .tests = ctf__group_arr_##group_name,                                  \
    .setup = ctf__group_setup_##group_name,                                \
    .teardown = ctf__group_teardown_##group_name,                          \
    .test_setup = ctf__group_test_setup_##group_name,                      \
    .test_teardown = ctf__group_test_teardown_##group_name,                \
    .name = ctf__group_name_##group_name,                                  \
  };                                                                       \
  static const struct ctf__test                                            \
    ctf__group_arr_##group_name[CTF_CONST_GROUP_SIZE]
#define CTF_GROUP_STATIC(group_name)                                       \
  static const char ctf__group_name_##group_name[] = #group_name;          \
  static const struct ctf__test                                            \
    ctf__group_arr_##group_name[CTF_CONST_GROUP_SIZE];                     \
  static void (*ctf__group_setup_p_##group_name)(void);                    \
  static void (*ctf__group_teardown_p_##group_name)(void);                 \
  static void (*ctf__group_test_setup_p_##group_name)(void);               \
  static void (*ctf__group_test_teardown_p_##group_name)(void);            \
  static void ctf__group_setup_##group_name(void) {                        \
    if(ctf__group_setup_p_##group_name) ctf__group_setup_p_##group_name(); \
  }                                                                        \
  static void ctf__group_teardown_##group_name(void) {                     \
    if(ctf__group_teardown_p_##group_name)                                 \
      ctf__group_teardown_p_##group_name();                                \
  }                                                                        \
  static void ctf__group_test_setup_##group_name(void) {                   \
    if(ctf__group_test_setup_p_##group_name)                               \
      ctf__group_test_setup_p_##group_name();                              \
  }                                                                        \
  static void ctf__group_test_teardown_##group_name(void) {                \
    if(ctf__group_test_teardown_p_##group_name)                            \
      ctf__group_test_teardown_p_##group_name();                           \
  }                                                                        \
  static const struct ctf__group group_name = {                            \
    .tests = ctf__group_arr_##group_name,                                  \
    .setup = ctf__group_setup_##group_name,                                \
    .teardown = ctf__group_teardown_##group_name,                          \
    .test_setup = ctf__group_test_setup_##group_name,                      \
    .test_teardown = ctf__group_test_teardown_##group_name,                \
    .name = ctf__group_name_##group_name,                                  \
  };                                                                       \
  static const struct ctf__test                                            \
    ctf__group_arr_##group_name[CTF_CONST_GROUP_SIZE]
#define CTF_GROUP_EXTERN(name) extern const struct ctf__group name;
#define CTF_GROUP_SETUP(name)                      \
  static void ctf__group_setup_def_##name(void);   \
  static void (*ctf__group_setup_p_##name)(void) = \
    ctf__group_setup_def_##name;                   \
  static void ctf__group_setup_def_##name(void)
#define CTF_GROUP_TEARDOWN(name)                      \
  static void ctf__group_teardown_def_##name(void);   \
  static void (*ctf__group_teardown_p_##name)(void) = \
    ctf__group_teardown_def_##name;                   \
  static void ctf__group_teardown_def_##name(void)
#define CTF_GROUP_TEST_SETUP(name)                      \
  static void ctf__group_test_setup_def_##name(void);   \
  static void (*ctf__group_test_setup_p_##name)(void) = \
    ctf__group_test_setup_def_##name;                   \
  static void ctf__group_test_setup_def_##name(void)
#define CTF_GROUP_TEST_TEARDOWN(name)                      \
  static void ctf__group_test_teardown_def_##name(void);   \
  static void (*ctf__group_test_teardown_p_##name)(void) = \
    ctf__group_test_teardown_def_##name;                   \
  static void ctf__group_test_teardown_def_##name(void)

void *ctf__cleanup_realloc(void *ptr, uintmax_t size, uintptr_t thread_index);
void *ctf__cleanup_malloc(uintmax_t size, uintptr_t thread_index);
void ctf__groups_run(int count, ...);
void ctf__subtest_enter(struct ctf__thread_data *thread_data, const char *name);
void ctf__subtest_leave(struct ctf__thread_data *thread_data);

void ctf_main(int argc, char *argv[]);
void ctf_group_run(struct ctf__group group);
#define ctf_groups_run(...)                                          \
  ctf__groups_run(sizeof((const struct ctf__group[]){__VA_ARGS__}) / \
                    sizeof(const struct ctf__group),                 \
                  __VA_ARGS__)

void ctf_sigsegv_handler(int unused);

#define ctf_subtest(name)                                                \
  for(uintptr_t ctf__subtest_thread_index =                              \
                  (uintptr_t)pthread_getspecific(ctf__thread_index),     \
                ctf__local_end_flag = 0;                                 \
      !ctf__local_end_flag;)                                             \
    for(ctf__subtest_enter(ctf__thread_data + ctf__subtest_thread_index, \
                           #name);                                       \
        !ctf__local_end_flag;                                            \
        ctf__local_end_flag = 1,                                         \
        ctf__subtest_leave(ctf__thread_data + ctf__subtest_thread_index))
#define ctf_barrier()      \
  do {                     \
    ctf_parallel_sync();   \
    if(ctf_exit_code) {    \
      ctf_parallel_stop(); \
      return;              \
    }                      \
  } while(0)
void ctf_parallel_start(void);
void ctf_parallel_stop(void);
void ctf_parallel_sync(void);

// define(`ALIAS', `#define $1 ctf_$1')
#if CTF_OTHER_ALIASES == CTF_ON
COMB(`ALIAS',
     `(subtest(name))')
#endif

include(`assert.h')
include(`mocks.h')
#endif
