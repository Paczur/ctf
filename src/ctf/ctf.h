#ifndef CTF_H
#define CTF_H

#ifndef CTF_ALIASES
#define CTF_ALIASES CTF_ON
#endif

#ifdef NDEBUG
#define prereq_assert(expr, msg)
#else
#define prereq_assert(expr, msg) \
  do {                           \
    if(!(expr)) assert(!msg);    \
  } while(0)
#endif

include(`base.m4')
#include <assert.h>
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
  struct ctf__state *states;
  uintmax_t states_size;
  uintmax_t states_capacity;
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
  const struct ctf__test test_name = {                         \
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
void ctf__assert_fold(uintmax_t count, const char *msg, int line,
                      const char *file);
uintmax_t ctf__pass(const char *, int, const char *,...);
uintmax_t ctf__fail(const char *, int, const char *,...);
uintmax_t ctf__assert_msg(int, const char *, int, const char *,...);

void ctf_main(int argc, char *argv[]);
void ctf_group_run(struct ctf__group group);
#define ctf_groups_run(...)                                          \
  ctf__groups_run(sizeof((const struct ctf__group[]){__VA_ARGS__}) / \
                    sizeof(const struct ctf__group),                 \
                  __VA_ARGS__)

void ctf_sigsegv_handler(int unused);

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

void ctf_assert_barrier(void);
void ctf_assert_hide(uintmax_t count);
#define ctf_assert_fold(count, msg) \
  ctf__assert_fold(count, msg, __LINE__, __FILE__)
#define ctf_pass(m, ...) ctf__pass(m, __LINE__, __FILE__, ##__VA_ARGS__)
#define ctf_fail(m, ...) ctf__fail(m, __LINE__, __FILE__, ##__VA_ARGS__)
#define ctf_assert_true(a) ctf_assert_int_neq(a, 0);
#define ctf_assert_false(a) ctf_assert_int_eq(a, 0);
#define ctf_expect_true(a) ctf_expect_int_neq(a, 0);
#define ctf_expect_false(a) ctf_expect_int_eq(a, 0);
#define ctf_assert_non_null(a) ctf_assert_ptr_neq(a, NULL);
#define ctf_assert_null(a) ctf_assert_ptr_eq(a, NULL);
#define ctf_expect_non_null(a) ctf_expect_ptr_neq(a, NULL);
#define ctf_expect_null(a) ctf_expect_ptr_eq(a, NULL);
#define ctf_assert_msg(cmp, m, ...) \
  ctf__assert_msg(, cmp, m, __LINE__, __FILE__, ##__VA_ARGS__)
#define ctf_assert(cmp)        \
  do {                         \
    ctf_assert_msg(cmp, #cmp); \
    ctf_assert_hide(1);        \
  } while(0)

// clang-format off
/*
define(`EA_TEMPLATE', `#define $4$3_$1_$2(a, b) ctf__$3_$1_$2(a, b, #a, #b, __LINE__, __FILE__)')dnl
define(`EA_MEMORY_TEMPLATE',
`format(`#define $4$3_memory_$1_$2(a, b, length) ctf__$3_memory_$1_$2((const void *)a, (const void *)b, length,  sizeof(*(a)), %d, #a, #b, __LINE__, __FILE__)', ifelse(`$1', `int', `1', `0'))')dnl
define(`EA_ARRAY_TEMPLATE',
`format(`#define $4$3_array_$1_$2(a, b) ctf__$3_array_$1_$2((const void *const *)a, (const void *const *)b, sizeof(a)/sizeof(*(a)), sizeof(b)/sizeof(*(b)),  sizeof(*(a)), %d, #a, #b, __LINE__, __FILE__)', ifelse(`$1', `int', `1', `0'))')dnl
define(`EA_FUNCTION',
`format(`int ctf__$3_$1_$2(%s, %s, const char *, const char *, int, const char *);',TYPE(`$1'),TYPE(`$1'))')dnl
define(`EA_MEMORY_PRIMITIVE_FUNCTION',
`int ctf__$3_memory_$1_$2(const void *, const void *, uintmax_t, uintmax_t, int, const char *, const char *, int, const char *);')dnl
define(`EA_ARRAY_PRIMITIVE_FUNCTION',
`int $4_$3_array_$1_$2(const void *, const void *, uintmax_t, uintmax_t, uintmax_t, int, const char *, const char *, int, const char*);')dnl
define(`ASSERT_WRAP',
`indir(`$1', `$2', `$3', `assert', `$4')
indir(`$1', `$2', `$3', `expect', `$4')
')dnl
define(`EA_FACTORY', `foreach(`type', `$1', `foreach(`comp', `$2', `indir(`ASSERT_WRAP', `$3', type, comp, `ctf_')')')')dnl
*/
EA_FACTORY(`(PRIMITIVE_TYPES, str)', `(CMPS)', `EA_FUNCTION')
EA_FACTORY(`(PRIMITIVE_TYPES)', `(CMPS)', `EA_MEMORY_PRIMITIVE_FUNCTION')
EA_FACTORY(`(PRIMITIVE_TYPES)', `(CMPS)', `EA_ARRAY_PRIMITIVE_FUNCTION')
EA_FACTORY(`(PRIMITIVE_TYPES, str)', `(CMPS)', `EA_TEMPLATE')
EA_FACTORY(`(PRIMITIVE_TYPES)', `(CMPS)', `EA_MEMORY_TEMPLATE')
EA_FACTORY(`(PRIMITIVE_TYPES)', `(CMPS)', `EA_ARRAY_TEMPLATE')
#if CTF_ALIASES == CTF_ON
/*
define(`EA_ALIAS_FACTORY', `foreach(`type', `$1', `foreach(`comp', `$2', `indir(`ASSERT_WRAP', `$3', type, comp, `')')')')dnl
define(`ALIAS', `#define $1 ctf_$1
')dnl
*/
COMB(`ALIAS',
     `(assert_barrier(), assert_fold(count, msg),
       assert_true(a), assert_false(a), expect_true(a), expect_false(a),
       assert_null(a), assert_non_null(a), expect_null(a), expect_non_null(a))')
EA_ALIAS_FACTORY(`(PRIMITIVE_TYPES, str)', `(CMPS)', `EA_TEMPLATE')
EA_ALIAS_FACTORY(`(PRIMITIVE_TYPES)', `(CMPS)', `EA_MEMORY_TEMPLATE')
EA_ALIAS_FACTORY(`(PRIMITIVE_TYPES)', `(CMPS)', `EA_ARRAY_TEMPLATE')
// clang-format on

#endif

include(`mocks.h')
#endif
