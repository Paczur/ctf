#ifndef CTF_H
#define CTF_H

#ifndef CTF_ALIASES
#define CTF_ALIASES CTF_ON
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
#define CTF__EA_MEM_TYPE_int 0
#define CTF__EA_MEM_TYPE_uint 1
#define CTF__EA_MEM_TYPE_ptr 2
#define CTF__EA_MEM_TYPE_char 3
#define CTF__EA_MEM_TYPE_float 4

#define CTF__EA_TYPE_INT intmax_t
#define CTF__EA_TYPE_UINT uintmax_t
#define CTF__EA_TYPE_CHAR char
#define CTF__EA_TYPE_PTR const void *
#define CTF__EA_TYPE_STR const char *
#define CTF__EA_TYPE_FLOAT long double

#define CTF__EA_FLAG_expect 0
#define CTF__EA_FLAG_assert 1

#define CTF__EA_SIGN_uint 0
#define CTF__EA_SIGN_int 1
#define CTF__EA_SIGN_char 0
#define CTF__EA_SIGN_ptr 0
#define CTF__EA_SIGN_float 2

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
int ctf__ea_arr(const void *a, const char *cmp, const void *b,
                uintmax_t la, uintmax_t lb, const char *a_str,
                const char *b_str, int assert,
                uintmax_t step, int sign, int type, int line,
                const char *file);
void ctf__assert_fold(uintmax_t count, const char *msg, int line,
                      const char *file);
uintmax_t ctf__pass(const char *, int, const char *,...);
uintmax_t ctf__fail(const char *, int, const char *,...);
uintmax_t ctf__assert_msg(int, const char *, int, const char *,...);
void ctf__subtest_enter(struct ctf__thread_data *thread_data, const char *name);
void ctf__subtest_leave(struct ctf__thread_data *thread_data);

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
#define ctf_assert_fold(count, msg) \
  ctf__assert_fold(count, msg, __LINE__, __FILE__)
#define ctf_pass(m, ...) ctf__pass(m, __LINE__, __FILE__, ##__VA_ARGS__)
#define ctf_fail(m, ...) ctf__fail(m, __LINE__, __FILE__, ##__VA_ARGS__)
#define ctf_assert_true(a) ctf_assert_int(0, !=, a);
#define ctf_assert_false(a) ctf_assert_int(0, ==, a);
#define ctf_expect_true(a) ctf_expect_int(0, !=, a);
#define ctf_expect_false(a) ctf_expect_int(0, ==, a);
#define ctf_assert_non_null(a) ctf_assert_ptr(NULL, !=, a);
#define ctf_assert_null(a) ctf_assert_ptr(NULL, ==, a);
#define ctf_expect_non_null(a) ctf_expect_ptr(NULL, !=, a);
#define ctf_expect_null(a) ctf_expect_ptr(NULL, ==, a);
#define ctf_assert_msg(cmp, m, ...) \
  ctf__assert_msg(, cmp, m, __LINE__, __FILE__, ##__VA_ARGS__)

#define CTF__EA_FUN(type, TYPE)                                \
  int ctf__ea_##type(CTF__EA_TYPE_##TYPE a, const char *cmp,   \
                     CTF__EA_TYPE_##TYPE b, const char *a_str, \
                     const char *b_str, int assert, int line,  \
                     const char *file);
// clang-format off
/*
define(`EA_FUNCTION',
`format(`int ctf__$3_$1_$2(%s, %s, const char *, const char *, int, const char *);',TYPE(`$1'),TYPE(`$1'))')dnl
define(`EA_MEM_FUNCTION',
`int ctf__$3_mem_$2(const void *, const void *, uintmax_t, uintmax_t, int, int, const char *, const char *, int, const char *);')dnl
define(`EA_ARR_FUNCTION',
`int ctf__$3_arr_$2(const void *, const void *, uintmax_t, uintmax_t, uintmax_t, int, int, const char *, const char *, int, const char*);')dnl
define(`EA_TEMPLATE', `#define $3$2_$1(a, cmp, b) ctf__ea_$1(a, #cmp, b, #a, #b, CTF__EA_FLAG_$2, __LINE__, __FILE__)')dnl
define(`EA_MEM_TEMPLATE',
`#define $3$2_mem_$1(a, cmp, b, length) ctf__ea_arr((const void *)a, #cmp, (const void *)b, length, length, #a, #b, CTF__EA_FLAG_$2, sizeof(*(a)), CTF__EA_SIGN_$1, CTF__EA_MEM_TYPE_$1, __LINE__, __FILE__)')dnl
define(`EA_ARR_TEMPLATE',
`#define $3$2_arr_$1(a, cmp, b) ctf__ea_arr((const void *)a, #cmp, (const void *)b, sizeof(a)/sizeof(*(a)), sizeof(b)/sizeof(*(b)), #a, #b, CTF__EA_FLAG_$2, sizeof(*(a)), CTF__EA_SIGN_$1, CTF__EA_MEM_TYPE_$1, __LINE__, __FILE__)')dnl
define(`EA_FUNCTION', `format(`int ctf__$2_$1(%s, const char *, %s, const char *, const char *, int, const char*);',TYPE(`$1'),TYPE(`$1'))')
define(`EA_MEM_FUNCTION',
`int ctf__$2_mem(const void *, const char *, const void *, uintmax_t, uintmax_t, int, int, const char *, const char *, int, const char *);')dnl
define(`EA_ARR_FUNCTION',
`int ctf__$2_arr(const void *, const char *, const void *, uintmax_t, uintmax_t, uintmax_t, int, int, const char *, const char *, int, const char*);')dnl
define(`EA_FACTORY', `foreach(`type', `$1',
`
indir(`$2', type, `assert', `ctf_')
indir(`$2', type, `expect', `ctf_')
')')')dnl
*/
EA_FACTORY(`(PRIMITIVE_TYPES, str)', `EA_TEMPLATE')
EA_FACTORY(`(PRIMITIVE_TYPES)', `EA_MEM_TEMPLATE')
EA_FACTORY(`(PRIMITIVE_TYPES)', `EA_ARR_TEMPLATE')
EA_FACTORY(`(PRIMITIVE_TYPES, str)', `EA_FUNCTION')
EA_FACTORY(`(PRIMITIVE_TYPES)', `EA_MEM_FUNCTION')
EA_FACTORY(`(PRIMITIVE_TYPES)', `EA_ARR_FUNCTION')
COMB2(`RUN1', `(CTF__EA_FUN)', `(PRIMITIVE_TYPES, str)')
#if __STDC_VERSION__ >= 201112L
#define ctf_expect(a, cmp, b) \
  _Generic((b), \
           char       : ctf__ea_char,\
           int8_t     : ctf__ea_int,\
           int16_t    : ctf__ea_int,\
           int32_t    : ctf__ea_int,\
           int64_t    : ctf__ea_int,\
           uint8_t    : ctf__ea_uint,\
           uint16_t   : ctf__ea_uint,\
           uint32_t   : ctf__ea_uint,\
           uint64_t   : ctf__ea_uint,\
           float      : ctf__ea_float,\
           double     : ctf__ea_float,\
           long double: ctf__ea_float,\
           default    : ctf__ea_ptr)\
((a), #cmp, (b), #a, #b, 0, __LINE__, __FILE__)
#define ctf_assert(a, cmp, b) \
  _Generic((b), \
           char    : ctf__ea_char,\
           int8_t  : ctf__ea_int,\
           int16_t : ctf__ea_int,\
           int32_t : ctf__ea_int,\
           int64_t : ctf__ea_int,\
           uint8_t : ctf__ea_uint,\
           uint16_t: ctf__ea_uint,\
           uint32_t: ctf__ea_uint,\
           uint64_t: ctf__ea_uint,\
           float:    ctf__ea_float,\
           double:    ctf__ea_float,\
           long double:    ctf__ea_float,\
           default    : ctf__ea_ptr)\
((a), #cmp, (b), #a, #b, 1, __LINE__, __FILE__)
#define ctf_expect_mem(a, cmp, b, l) \
  ctf__ea_arr((a), #cmp, (b), l, l, #a, #b, 0,\
                  sizeof(*a),\
                  _Generic((*(b)),\
                           int8_t  : CTF__EA_SIGN_int,\
                           int16_t : CTF__EA_SIGN_int,\
                           int32_t : CTF__EA_SIGN_int,\
                           int64_t : CTF__EA_SIGN_int,\
                           float   : CTF__EA_SIGN_float,\
                           double   : CTF__EA_SIGN_float,\
                           long double   : CTF__EA_SIGN_float,\
                           default : CTF__EA_SIGN_uint), \
                           _Generic((*(b)),\
           char     : CTF__EA_MEM_TYPE_char,\
           int8_t   : CTF__EA_MEM_TYPE_int,\
           int16_t  : CTF__EA_MEM_TYPE_int,\
           int32_t  : CTF__EA_MEM_TYPE_int,\
           int64_t  : CTF__EA_MEM_TYPE_int,\
           uint8_t  : CTF__EA_MEM_TYPE_uint,\
           uint16_t : CTF__EA_MEM_TYPE_uint,\
           uint32_t : CTF__EA_MEM_TYPE_uint,\
           uint64_t : CTF__EA_MEM_TYPE_uint,\
           float: CTF__EA_MEM_TYPE_float,\
           double: CTF__EA_MEM_TYPE_float,\
           long double: CTF__EA_MEM_TYPE_float,\
           default    : CTF__EA_MEM_TYPE_ptr),\
__LINE__, __FILE__)
#define ctf_assert_mem(a, cmp, b, l) \
  ctf__ea_arr((a), #cmp, (b), l, l, #a, #b, 1,\
                  sizeof(*a),\
                  _Generic((*(b)),\
                           int8_t  : CTF__EA_SIGN_int,\
                           int16_t : CTF__EA_SIGN_int,\
                           int32_t : CTF__EA_SIGN_int,\
                           int64_t : CTF__EA_SIGN_int,\
                           float   : CTF__EA_SIGN_float,\
                           double   : CTF__EA_SIGN_float,\
                           long double   : CTF__EA_SIGN_float,\
                           default : CTF__EA_SIGN_uint), \
                           _Generic((*(b)),\
           char     : CTF__EA_MEM_TYPE_char,\
           int8_t   : CTF__EA_MEM_TYPE_int,\
           int16_t  : CTF__EA_MEM_TYPE_int,\
           int32_t  : CTF__EA_MEM_TYPE_int,\
           int64_t  : CTF__EA_MEM_TYPE_int,\
           uint8_t  : CTF__EA_MEM_TYPE_uint,\
           uint16_t : CTF__EA_MEM_TYPE_uint,\
           uint32_t : CTF__EA_MEM_TYPE_uint,\
           uint64_t : CTF__EA_MEM_TYPE_uint,\
           float: CTF__EA_MEM_TYPE_float,\
           double: CTF__EA_MEM_TYPE_float,\
           long double: CTF__EA_MEM_TYPE_float,\
           default    : CTF__EA_MEM_TYPE_ptr),\
__LINE__, __FILE__)
#define ctf_expect_arr(a, cmp, b) \
  ctf__ea_arr((a), #cmp, (b), sizeof(a)/sizeof(*(a)), sizeof(b)/sizeof(*(b)), #a, #b, 0,\
                  sizeof(*a),\
                  _Generic((*(b)),\
                           int8_t  : CTF__EA_SIGN_int,\
                           int16_t : CTF__EA_SIGN_int,\
                           int32_t : CTF__EA_SIGN_int,\
                           int64_t : CTF__EA_SIGN_int,\
                           float   : CTF__EA_SIGN_float,\
                           double   : CTF__EA_SIGN_float,\
                           long double   : CTF__EA_SIGN_float,\
                           default : CTF__EA_SIGN_uint), \
                           _Generic((*(b)),\
           char     : CTF__EA_MEM_TYPE_char,\
           int8_t   : CTF__EA_MEM_TYPE_int,\
           int16_t  : CTF__EA_MEM_TYPE_int,\
           int32_t  : CTF__EA_MEM_TYPE_int,\
           int64_t  : CTF__EA_MEM_TYPE_int,\
           uint8_t  : CTF__EA_MEM_TYPE_uint,\
           uint16_t : CTF__EA_MEM_TYPE_uint,\
           uint32_t : CTF__EA_MEM_TYPE_uint,\
           uint64_t : CTF__EA_MEM_TYPE_uint,\
           float: CTF__EA_MEM_TYPE_float,\
           double: CTF__EA_MEM_TYPE_float,\
           long double: CTF__EA_MEM_TYPE_float,\
           default    : CTF__EA_MEM_TYPE_ptr),\
__LINE__, __FILE__)
#define ctf_assert_arr(a, cmp, b) \
  ctf__ea_arr((a), #cmp, (b), sizeof(a)/sizeof(*(a)), sizeof(b)/sizeof(*(b)), #a, #b, 1,\
                  sizeof(*a),\
                  _Generic((*(b)),\
                           int8_t  : CTF__EA_SIGN_int,\
                           int16_t : CTF__EA_SIGN_int,\
                           int32_t : CTF__EA_SIGN_int,\
                           int64_t : CTF__EA_SIGN_int,\
                           float   : CTF__EA_SIGN_float,\
                           double   : CTF__EA_SIGN_float,\
                           long double   : CTF__EA_SIGN_float,\
                           default : CTF__EA_SIGN_uint), \
                           _Generic((*(b)),\
           char     : CTF__EA_MEM_TYPE_char,\
           int8_t   : CTF__EA_MEM_TYPE_int,\
           int16_t  : CTF__EA_MEM_TYPE_int,\
           int32_t  : CTF__EA_MEM_TYPE_int,\
           int64_t  : CTF__EA_MEM_TYPE_int,\
           uint8_t  : CTF__EA_MEM_TYPE_uint,\
           uint16_t : CTF__EA_MEM_TYPE_uint,\
           uint32_t : CTF__EA_MEM_TYPE_uint,\
           uint64_t : CTF__EA_MEM_TYPE_uint,\
           float: CTF__EA_MEM_TYPE_float,\
           double: CTF__EA_MEM_TYPE_float,\
           long double: CTF__EA_MEM_TYPE_float,\
           default    : CTF__EA_MEM_TYPE_ptr),\
__LINE__, __FILE__)
#endif
#if CTF_ALIASES == CTF_ON
/*
define(`EA_ALIAS_FACTORY', `foreach(`type', `$1', `foreach(`comp', `$2',
`
indir(`$3', type, comp, `assert', `')
indir(`$3', type, comp, `expect', `')
'
)')')dnl
define(`EA_ALIAS_FACTORY', `foreach(`type', `$1',
`
indir(`$2', type, `assert', `')
indir(`$2', type, `expect', `')
')')')dnl
define(`ALIAS', `#define $1 ctf_$1
')dnl
*/
#if __STDC_VERSION__ >= 201112L
#define expect(a, cmp, b) \
  _Generic((b), \
           char       : ctf__ea_char,\
           int8_t     : ctf__ea_int,\
           int16_t    : ctf__ea_int,\
           int32_t    : ctf__ea_int,\
           int64_t    : ctf__ea_int,\
           uint8_t    : ctf__ea_uint,\
           uint16_t   : ctf__ea_uint,\
           uint32_t   : ctf__ea_uint,\
           uint64_t   : ctf__ea_uint,\
           float      : ctf__ea_float,\
           double     : ctf__ea_float,\
           long double: ctf__ea_float,\
           default    : ctf__ea_ptr)\
((a), #cmp, (b), #a, #b, 0, __LINE__, __FILE__)
#define assert(a, cmp, b) \
  _Generic((b), \
           char    : ctf__ea_char,\
           int8_t  : ctf__ea_int,\
           int16_t : ctf__ea_int,\
           int32_t : ctf__ea_int,\
           int64_t : ctf__ea_int,\
           uint8_t : ctf__ea_uint,\
           uint16_t: ctf__ea_uint,\
           uint32_t: ctf__ea_uint,\
           uint64_t: ctf__ea_uint,\
           float:    ctf__ea_float,\
           double:    ctf__ea_float,\
           long double:    ctf__ea_float,\
           default    : ctf__ea_ptr)\
((a), #cmp, (b), #a, #b, 1, __LINE__, __FILE__)
#define expect_mem(a, cmp, b, l) \
  ctf__ea_arr((a), #cmp, (b), l, l, #a, #b, 0,\
                  sizeof(*a),\
                  _Generic((*(b)),\
                           int8_t  : CTF__EA_SIGN_int,\
                           int16_t : CTF__EA_SIGN_int,\
                           int32_t : CTF__EA_SIGN_int,\
                           int64_t : CTF__EA_SIGN_int,\
                           float   : CTF__EA_SIGN_float,\
                           double   : CTF__EA_SIGN_float,\
                           long double   : CTF__EA_SIGN_float,\
                           default : CTF__EA_SIGN_uint), \
                           _Generic((*(b)),\
           char     : CTF__EA_MEM_TYPE_char,\
           int8_t   : CTF__EA_MEM_TYPE_int,\
           int16_t  : CTF__EA_MEM_TYPE_int,\
           int32_t  : CTF__EA_MEM_TYPE_int,\
           int64_t  : CTF__EA_MEM_TYPE_int,\
           uint8_t  : CTF__EA_MEM_TYPE_uint,\
           uint16_t : CTF__EA_MEM_TYPE_uint,\
           uint32_t : CTF__EA_MEM_TYPE_uint,\
           uint64_t : CTF__EA_MEM_TYPE_uint,\
           float: CTF__EA_MEM_TYPE_float,\
           double: CTF__EA_MEM_TYPE_float,\
           long double: CTF__EA_MEM_TYPE_float,\
           default    : CTF__EA_MEM_TYPE_ptr),\
__LINE__, __FILE__)
#define assert_mem(a, cmp, b, l) \
  ctf__ea_arr((a), #cmp, (b), l, l, #a, #b, 1,\
                  sizeof(*a),\
                  _Generic((*(b)),\
                           int8_t  : CTF__EA_SIGN_int,\
                           int16_t : CTF__EA_SIGN_int,\
                           int32_t : CTF__EA_SIGN_int,\
                           int64_t : CTF__EA_SIGN_int,\
                           float   : CTF__EA_SIGN_float,\
                           double   : CTF__EA_SIGN_float,\
                           long double   : CTF__EA_SIGN_float,\
                           default : CTF__EA_SIGN_uint), \
                           _Generic((*(b)),\
           char     : CTF__EA_MEM_TYPE_char,\
           int8_t   : CTF__EA_MEM_TYPE_int,\
           int16_t  : CTF__EA_MEM_TYPE_int,\
           int32_t  : CTF__EA_MEM_TYPE_int,\
           int64_t  : CTF__EA_MEM_TYPE_int,\
           uint8_t  : CTF__EA_MEM_TYPE_uint,\
           uint16_t : CTF__EA_MEM_TYPE_uint,\
           uint32_t : CTF__EA_MEM_TYPE_uint,\
           uint64_t : CTF__EA_MEM_TYPE_uint,\
           float: CTF__EA_MEM_TYPE_float,\
           double: CTF__EA_MEM_TYPE_float,\
           long double: CTF__EA_MEM_TYPE_float,\
           default    : CTF__EA_MEM_TYPE_ptr),\
__LINE__, __FILE__)
#define expect_arr(a, cmp, b) \
  ctf__ea_arr((a), #cmp, (b), sizeof(a)/sizeof(*(a)), sizeof(b)/sizeof(*(b)), #a, #b, 0,\
                  sizeof(*a),\
                  _Generic((*(b)),\
                           int8_t  : CTF__EA_SIGN_int,\
                           int16_t : CTF__EA_SIGN_int,\
                           int32_t : CTF__EA_SIGN_int,\
                           int64_t : CTF__EA_SIGN_int,\
                           float   : CTF__EA_SIGN_float,\
                           double   : CTF__EA_SIGN_float,\
                           long double   : CTF__EA_SIGN_float,\
                           default : CTF__EA_SIGN_uint), \
                           _Generic((*(b)),\
           char     : CTF__EA_MEM_TYPE_char,\
           int8_t   : CTF__EA_MEM_TYPE_int,\
           int16_t  : CTF__EA_MEM_TYPE_int,\
           int32_t  : CTF__EA_MEM_TYPE_int,\
           int64_t  : CTF__EA_MEM_TYPE_int,\
           uint8_t  : CTF__EA_MEM_TYPE_uint,\
           uint16_t : CTF__EA_MEM_TYPE_uint,\
           uint32_t : CTF__EA_MEM_TYPE_uint,\
           uint64_t : CTF__EA_MEM_TYPE_uint,\
           float: CTF__EA_MEM_TYPE_float,\
           double: CTF__EA_MEM_TYPE_float,\
           long double: CTF__EA_MEM_TYPE_float,\
           default    : CTF__EA_MEM_TYPE_ptr),\
__LINE__, __FILE__)
#define assert_arr(a, cmp, b) \
  ctf__ea_arr((a), #cmp, (b), sizeof(a)/sizeof(*(a)), sizeof(b)/sizeof(*(b)), #a, #b, 1,\
                  sizeof(*a),\
                  _Generic((*(b)),\
                           int8_t  : CTF__EA_SIGN_int,\
                           int16_t : CTF__EA_SIGN_int,\
                           int32_t : CTF__EA_SIGN_int,\
                           int64_t : CTF__EA_SIGN_int,\
                           float   : CTF__EA_SIGN_float,\
                           double   : CTF__EA_SIGN_float,\
                           long double   : CTF__EA_SIGN_float,\
                           default : CTF__EA_SIGN_uint), \
                           _Generic((*(b)),\
           char     : CTF__EA_MEM_TYPE_char,\
           int8_t   : CTF__EA_MEM_TYPE_int,\
           int16_t  : CTF__EA_MEM_TYPE_int,\
           int32_t  : CTF__EA_MEM_TYPE_int,\
           int64_t  : CTF__EA_MEM_TYPE_int,\
           uint8_t  : CTF__EA_MEM_TYPE_uint,\
           uint16_t : CTF__EA_MEM_TYPE_uint,\
           uint32_t : CTF__EA_MEM_TYPE_uint,\
           uint64_t : CTF__EA_MEM_TYPE_uint,\
           float: CTF__EA_MEM_TYPE_float,\
           double: CTF__EA_MEM_TYPE_float,\
           long double: CTF__EA_MEM_TYPE_float,\
           default    : CTF__EA_MEM_TYPE_ptr),\
__LINE__, __FILE__)
#endif
COMB(`ALIAS',
     `(assert_barrier(), assert_fold(count, msg), subtest(name),
       assert_true(a), assert_false(a), expect_true(a), expect_false(a),
       assert_null(a), assert_non_null(a), expect_null(a), expect_non_null(a))')
EA_ALIAS_FACTORY(`(PRIMITIVE_TYPES, str)', `EA_TEMPLATE')
EA_ALIAS_FACTORY(`(PRIMITIVE_TYPES)', `EA_MEM_TEMPLATE')
EA_ALIAS_FACTORY(`(PRIMITIVE_TYPES)', `EA_ARR_TEMPLATE')
EA_ALIAS_FACTORY(`(PRIMITIVE_TYPES, str)', `EA_FUNCTION')
// clang-format on

#endif

include(`mocks.h')
#endif
