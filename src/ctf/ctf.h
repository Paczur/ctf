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
#define CTF__ASSERT_PRINT_TYPE_int 0
#define CTF__ASSERT_PRINT_TYPE_uint 1
#define CTF__ASSERT_PRINT_TYPE_ptr 2
#define CTF__ASSERT_PRINT_TYPE_char 3
#define CTF__ASSERT_PRINT_TYPE_float 4

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

// clang-format off
/*
define(`EA_TEMPLATE', `#define $4$3_$1_$2(a, b) ctf__$3_$1_$2(a, b, #a, #b, __LINE__, __FILE__)')dnl
define(`EA_MEM_TEMPLATE',
`format(`#define $4$3_mem_$1_$2(a, b, length) ctf__$3_mem_$2((const void *)a, (const void *)b, length,  sizeof(*(a)), %d, CTF__ASSERT_PRINT_TYPE_$1, #a, #b, __LINE__, __FILE__)', ifelse(`$1', `int', `1', `$1', `float', `2', `0'))')dnl
define(`EA_ARR_TEMPLATE',
`format(`#define $4$3_arr_$1_$2(a, b) ctf__$3_arr_$2((const void *const *)a, (const void *const *)b, sizeof(a)/sizeof(*(a)), sizeof(b)/sizeof(*(b)),  sizeof(*(a)), %d, CTF__ASSERT_PRINT_TYPE_$1, #a, #b, __LINE__, __FILE__)', ifelse(`$1', `int', `1', `$1', `float', `2', `0'))')dnl
define(`EA_FUNCTION',
`format(`int ctf__$3_$1_$2(%s, %s, const char *, const char *, int, const char *);',TYPE(`$1'),TYPE(`$1'))')dnl
define(`EA_MEM_FUNCTION',
`int ctf__$3_mem_$2(const void *, const void *, uintmax_t, uintmax_t, int, int, const char *, const char *, int, const char *);')dnl
define(`EA_ARR_FUNCTION',
`int ctf__$3_arr_$2(const void *, const void *, uintmax_t, uintmax_t, uintmax_t, int, int, const char *, const char *, int, const char*);')dnl
define(`EA_FACTORY', `foreach(`type', `$1', `foreach(`comp', `$2',
`
indir(`$3', type, comp, `assert', `ctf_')
indir(`$3', type, comp, `expect', `ctf_')
')')')dnl
define(`EA_COMP_TEMPLATE', `#define $3$2_$1(a, cmp, b) ctf__$2_$1(a, #cmp, b, #a, #b, __LINE__, __FILE__)')dnl
define(`EA_COMP_MEM_TEMPLATE',
`format(`#define $3$2_mem_$1(a, cmp, b, length) ctf__$2_mem((const void *)a, #cmp, (const void *)b, length,  sizeof(*(a)), %d, CTF__ASSERT_PRINT_TYPE_$1, #a, #b, __LINE__, __FILE__)', ifelse(`$1', `int', `1', `$1', `float', `2', `0'))')dnl
define(`EA_COMP_ARR_TEMPLATE',
`format(`#define $3$2_arr_$1(a, cmp, b) ctf__$2_arr((const void *const *)a, #cmp, (const void *const *)b, sizeof(a)/sizeof(*(a)), sizeof(b)/sizeof(*(b)),  sizeof(*(a)), %d, CTF__ASSERT_PRINT_TYPE_$1, #a, #b, __LINE__, __FILE__)', ifelse(`$1', `int', `1', `$1', `float', `2', `0'))')dnl
define(`EA_COMP_FUNCTION', `format(`int ctf__$2_$1(%s, const char *, %s, const char *, const char *, int, const char*);',TYPE(`$1'),TYPE(`$1'))')
define(`EA_COMP_MEM_FUNCTION',
`int ctf__$2_mem(const void *, const char *, const void *, uintmax_t, uintmax_t, int, int, const char *, const char *, int, const char *);')dnl
define(`EA_COMP_ARR_FUNCTION',
`int ctf__$2_arr(const void *, const char *, const void *, uintmax_t, uintmax_t, uintmax_t, int, int, const char *, const char *, int, const char*);')dnl
define(`EA_COMP_FACTORY', `foreach(`type', `$1',
`
indir(`$2', type, `assert', `ctf_')
indir(`$2', type, `expect', `ctf_')
')')')dnl
*/
EA_FACTORY(`(PRIMITIVE_TYPES, str)', `(CMPS)', `EA_FUNCTION')
EA_FACTORY(`(PRIMITIVE_TYPES)', `(CMPS)', `EA_MEM_FUNCTION')
EA_FACTORY(`(PRIMITIVE_TYPES)', `(CMPS)', `EA_ARR_FUNCTION')
EA_COMP_FACTORY(`(PRIMITIVE_TYPES, str)', `EA_COMP_TEMPLATE')
EA_COMP_FACTORY(`(PRIMITIVE_TYPES)', `EA_COMP_MEM_TEMPLATE')
EA_COMP_FACTORY(`(PRIMITIVE_TYPES)', `EA_COMP_ARR_TEMPLATE')
EA_COMP_FACTORY(`(PRIMITIVE_TYPES, str)', `EA_COMP_FUNCTION')
EA_COMP_FACTORY(`(PRIMITIVE_TYPES)', `EA_COMP_MEM_FUNCTION')
EA_COMP_FACTORY(`(PRIMITIVE_TYPES)', `EA_COMP_ARR_FUNCTION')
#if __STDC_VERSION__ >= 201112L
#define ctf_expect(a, cmp, b) \
  _Generic((b), \
           char       : ctf__expect_char,\
           int8_t     : ctf__expect_int,\
           int16_t    : ctf__expect_int,\
           int32_t    : ctf__expect_int,\
           int64_t    : ctf__expect_int,\
           uint8_t    : ctf__expect_uint,\
           uint16_t   : ctf__expect_uint,\
           uint32_t   : ctf__expect_uint,\
           uint64_t   : ctf__expect_uint,\
           float      : ctf__expect_float,\
           double     : ctf__expect_float,\
           long double: ctf__expect_float,\
           default    : ctf__expect_ptr)\
((a), #cmp, (b), #a, #b, __LINE__, __FILE__)
#define ctf_assert(a, cmp, b) \
  _Generic((b), \
           char    : ctf__assert_char,\
           int8_t  : ctf__assert_int,\
           int16_t : ctf__assert_int,\
           int32_t : ctf__assert_int,\
           int64_t : ctf__assert_int,\
           uint8_t : ctf__assert_uint,\
           uint16_t: ctf__assert_uint,\
           uint32_t: ctf__assert_uint,\
           uint64_t: ctf__assert_uint,\
           float:    ctf__assert_float,\
           double:    ctf__assert_float,\
           long double:    ctf__assert_float,\
           default    : ctf__assert_ptr)\
((a), #cmp, (b), #a, #b, __LINE__, __FILE__)
#define ctf_expect_mem(a, cmp, b, l) \
  ctf__expect_mem((a), #cmp, (b), l, \
                  sizeof(*a),\
                  _Generic((*(b)),\
                           int8_t  : 1,\
                           int16_t : 1,\
                           int32_t : 1,\
                           int64_t : 1,\
                           float   : 2,\
                           double   : 2,\
                           long double   : 2,\
                           default : 0), \
                           _Generic((*(b)),\
           char     : CTF__ASSERT_PRINT_TYPE_char,\
           int8_t   : CTF__ASSERT_PRINT_TYPE_int,\
           int16_t  : CTF__ASSERT_PRINT_TYPE_int,\
           int32_t  : CTF__ASSERT_PRINT_TYPE_int,\
           int64_t  : CTF__ASSERT_PRINT_TYPE_int,\
           uint8_t  : CTF__ASSERT_PRINT_TYPE_uint,\
           uint16_t : CTF__ASSERT_PRINT_TYPE_uint,\
           uint32_t : CTF__ASSERT_PRINT_TYPE_uint,\
           uint64_t : CTF__ASSERT_PRINT_TYPE_uint,\
           float: CTF__ASSERT_PRINT_TYPE_float,\
           double: CTF__ASSERT_PRINT_TYPE_float,\
           long double: CTF__ASSERT_PRINT_TYPE_float,\
           default    : CTF__ASSERT_PRINT_TYPE_ptr),\
#a, #b, __LINE__, __FILE__)
#define ctf_assert_mem(a, cmp, b, l) \
  ctf__assert_mem((a), #cmp, (b), l, \
                  sizeof(*a),\
                  _Generic((*(b)),\
                           int8_t  : 1,\
                           int16_t : 1,\
                           int32_t : 1,\
                           int64_t : 1,\
                           float   : 2,\
                           double   : 2,\
                           long double   : 2,\
                           default : 0), \
                           _Generic((*(b)),\
           char     : CTF__ASSERT_PRINT_TYPE_char,\
           int8_t   : CTF__ASSERT_PRINT_TYPE_int,\
           int16_t  : CTF__ASSERT_PRINT_TYPE_int,\
           int32_t  : CTF__ASSERT_PRINT_TYPE_int,\
           int64_t  : CTF__ASSERT_PRINT_TYPE_int,\
           uint8_t  : CTF__ASSERT_PRINT_TYPE_uint,\
           uint16_t : CTF__ASSERT_PRINT_TYPE_uint,\
           uint32_t : CTF__ASSERT_PRINT_TYPE_uint,\
           uint64_t : CTF__ASSERT_PRINT_TYPE_uint,\
           float: CTF__ASSERT_PRINT_TYPE_float,\
           double: CTF__ASSERT_PRINT_TYPE_float,\
           long double: CTF__ASSERT_PRINT_TYPE_float,\
           default    : CTF__ASSERT_PRINT_TYPE_ptr),\
#a, #b, __LINE__, __FILE__)
#define ctf_expect_arr(a, cmp, b) \
  ctf__expect_arr((a), #cmp, (b), sizeof(a)/sizeof(*(a)), \
                  sizeof(b)/sizeof(*(b)), sizeof(*a),\
                  _Generic((*(b)),\
                           int8_t  : 1,\
                           int16_t : 1,\
                           int32_t : 1,\
                           int64_t : 1,\
                           float   : 2,\
                           double   : 2,\
                           long double   : 2,\
                           default : 0), \
                           _Generic((*(b)),\
           char     : CTF__ASSERT_PRINT_TYPE_char,\
           int8_t   : CTF__ASSERT_PRINT_TYPE_int,\
           int16_t  : CTF__ASSERT_PRINT_TYPE_int,\
           int32_t  : CTF__ASSERT_PRINT_TYPE_int,\
           int64_t  : CTF__ASSERT_PRINT_TYPE_int,\
           uint8_t  : CTF__ASSERT_PRINT_TYPE_uint,\
           uint16_t : CTF__ASSERT_PRINT_TYPE_uint,\
           uint32_t : CTF__ASSERT_PRINT_TYPE_uint,\
           uint64_t : CTF__ASSERT_PRINT_TYPE_uint,\
           float: CTF__ASSERT_PRINT_TYPE_float,\
           double: CTF__ASSERT_PRINT_TYPE_float,\
           long double: CTF__ASSERT_PRINT_TYPE_float,\
           default    : CTF__ASSERT_PRINT_TYPE_ptr),\
#a, #b, __LINE__, __FILE__)
#define ctf_assert_arr(a, cmp, b) \
  ctf__assert_arr((a), #cmp, (b), sizeof(a)/sizeof(*(a)), \
                  sizeof(b)/sizeof(*(b)), sizeof(*a),\
                  _Generic((*(b)),\
                           int8_t  : 1,\
                           int16_t : 1,\
                           int32_t : 1,\
                           int64_t : 1,\
                           float   : 2,\
                           double   : 2,\
                           long double   : 2,\
                           default : 0), \
                           _Generic((*(b)),\
           char     : CTF__ASSERT_PRINT_TYPE_char,\
           int8_t   : CTF__ASSERT_PRINT_TYPE_int,\
           int16_t  : CTF__ASSERT_PRINT_TYPE_int,\
           int32_t  : CTF__ASSERT_PRINT_TYPE_int,\
           int64_t  : CTF__ASSERT_PRINT_TYPE_int,\
           uint8_t  : CTF__ASSERT_PRINT_TYPE_uint,\
           uint16_t : CTF__ASSERT_PRINT_TYPE_uint,\
           uint32_t : CTF__ASSERT_PRINT_TYPE_uint,\
           uint64_t : CTF__ASSERT_PRINT_TYPE_uint,\
           float: CTF__ASSERT_PRINT_TYPE_float,\
           double: CTF__ASSERT_PRINT_TYPE_float,\
           long double: CTF__ASSERT_PRINT_TYPE_float,\
           default    : CTF__ASSERT_PRINT_TYPE_ptr),\
#a, #b, __LINE__, __FILE__)
#endif
#if CTF_ALIASES == CTF_ON
/*
define(`EA_ALIAS_FACTORY', `foreach(`type', `$1', `foreach(`comp', `$2',
`
indir(`$3', type, comp, `assert', `')
indir(`$3', type, comp, `expect', `')
'
)')')dnl
define(`EA_COMP_ALIAS_FACTORY', `foreach(`type', `$1',
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
           char       : ctf__expect_char,\
           int8_t     : ctf__expect_int,\
           int16_t    : ctf__expect_int,\
           int32_t    : ctf__expect_int,\
           int64_t    : ctf__expect_int,\
           uint8_t    : ctf__expect_uint,\
           uint16_t   : ctf__expect_uint,\
           uint32_t   : ctf__expect_uint,\
           uint64_t   : ctf__expect_uint,\
           float      : ctf__expect_float,\
           double     : ctf__expect_float,\
           long double: ctf__expect_float,\
           default    : ctf__expect_ptr)\
((a), #cmp, (b), #a, #b, __LINE__, __FILE__)
#define assert(a, cmp, b) \
  _Generic((b), \
           char    : ctf__assert_char,\
           int8_t  : ctf__assert_int,\
           int16_t : ctf__assert_int,\
           int32_t : ctf__assert_int,\
           int64_t : ctf__assert_int,\
           uint8_t : ctf__assert_uint,\
           uint16_t: ctf__assert_uint,\
           uint32_t: ctf__assert_uint,\
           uint64_t: ctf__assert_uint,\
           float:    ctf__assert_float,\
           double:    ctf__assert_float,\
           long double:    ctf__assert_float,\
           default    : ctf__assert_ptr)\
((a), #cmp, (b), #a, #b, __LINE__, __FILE__)
#define expect_mem(a, cmp, b, l) \
  ctf__expect_mem((a), #cmp, (b), l, \
                  sizeof(*a),\
                  _Generic((*(b)),\
                           int8_t  : 1,\
                           int16_t : 1,\
                           int32_t : 1,\
                           int64_t : 1,\
                           float   : 2,\
                           double   : 2,\
                           long double   : 2,\
                           default : 0), \
                           _Generic((*(b)),\
           char     : CTF__ASSERT_PRINT_TYPE_char,\
           int8_t   : CTF__ASSERT_PRINT_TYPE_int,\
           int16_t  : CTF__ASSERT_PRINT_TYPE_int,\
           int32_t  : CTF__ASSERT_PRINT_TYPE_int,\
           int64_t  : CTF__ASSERT_PRINT_TYPE_int,\
           uint8_t  : CTF__ASSERT_PRINT_TYPE_uint,\
           uint16_t : CTF__ASSERT_PRINT_TYPE_uint,\
           uint32_t : CTF__ASSERT_PRINT_TYPE_uint,\
           uint64_t : CTF__ASSERT_PRINT_TYPE_uint,\
           float: CTF__ASSERT_PRINT_TYPE_float,\
           double: CTF__ASSERT_PRINT_TYPE_float,\
           long double: CTF__ASSERT_PRINT_TYPE_float,\
           default    : CTF__ASSERT_PRINT_TYPE_ptr),\
#a, #b, __LINE__, __FILE__)
#define assert_mem(a, cmp, b, l) \
  ctf__assert_mem((a), #cmp, (b), l, \
                  sizeof(*a),\
                  _Generic((*(b)),\
                           int8_t  : 1,\
                           int16_t : 1,\
                           int32_t : 1,\
                           int64_t : 1,\
                           float   : 2,\
                           double   : 2,\
                           long double   : 2,\
                           default : 0), \
                           _Generic((*(b)),\
           char     : CTF__ASSERT_PRINT_TYPE_char,\
           int8_t   : CTF__ASSERT_PRINT_TYPE_int,\
           int16_t  : CTF__ASSERT_PRINT_TYPE_int,\
           int32_t  : CTF__ASSERT_PRINT_TYPE_int,\
           int64_t  : CTF__ASSERT_PRINT_TYPE_int,\
           uint8_t  : CTF__ASSERT_PRINT_TYPE_uint,\
           uint16_t : CTF__ASSERT_PRINT_TYPE_uint,\
           uint32_t : CTF__ASSERT_PRINT_TYPE_uint,\
           uint64_t : CTF__ASSERT_PRINT_TYPE_uint,\
           float: CTF__ASSERT_PRINT_TYPE_float,\
           double: CTF__ASSERT_PRINT_TYPE_float,\
           long double: CTF__ASSERT_PRINT_TYPE_float,\
           default    : CTF__ASSERT_PRINT_TYPE_ptr),\
#a, #b, __LINE__, __FILE__)
#define expect_arr(a, cmp, b) \
  ctf__expect_arr((a), #cmp, (b), sizeof(a)/sizeof(*(a)), \
                  sizeof(b)/sizeof(*(b)), sizeof(*a),\
                  _Generic((*(b)),\
                           int8_t  : 1,\
                           int16_t : 1,\
                           int32_t : 1,\
                           int64_t : 1,\
                           float   : 2,\
                           double   : 2,\
                           long double   : 2,\
                           default : 0), \
                           _Generic((*(b)),\
           char     : CTF__ASSERT_PRINT_TYPE_char,\
           int8_t   : CTF__ASSERT_PRINT_TYPE_int,\
           int16_t  : CTF__ASSERT_PRINT_TYPE_int,\
           int32_t  : CTF__ASSERT_PRINT_TYPE_int,\
           int64_t  : CTF__ASSERT_PRINT_TYPE_int,\
           uint8_t  : CTF__ASSERT_PRINT_TYPE_uint,\
           uint16_t : CTF__ASSERT_PRINT_TYPE_uint,\
           uint32_t : CTF__ASSERT_PRINT_TYPE_uint,\
           uint64_t : CTF__ASSERT_PRINT_TYPE_uint,\
           float: CTF__ASSERT_PRINT_TYPE_float,\
           double: CTF__ASSERT_PRINT_TYPE_float,\
           long double: CTF__ASSERT_PRINT_TYPE_float,\
           default    : CTF__ASSERT_PRINT_TYPE_ptr),\
#a, #b, __LINE__, __FILE__)
#define assert_arr(a, cmp, b) \
  ctf__assert_arr((a), #cmp, (b), sizeof(a)/sizeof(*(a)), \
                  sizeof(b)/sizeof(*(b)), sizeof(*a),\
                  _Generic((*(b)),\
                           int8_t  : 1,\
                           int16_t : 1,\
                           int32_t : 1,\
                           int64_t : 1,\
                           float   : 2,\
                           double   : 2,\
                           long double   : 2,\
                           default : 0), \
                           _Generic((*(b)),\
           char     : CTF__ASSERT_PRINT_TYPE_char,\
           int8_t   : CTF__ASSERT_PRINT_TYPE_int,\
           int16_t  : CTF__ASSERT_PRINT_TYPE_int,\
           int32_t  : CTF__ASSERT_PRINT_TYPE_int,\
           int64_t  : CTF__ASSERT_PRINT_TYPE_int,\
           uint8_t  : CTF__ASSERT_PRINT_TYPE_uint,\
           uint16_t : CTF__ASSERT_PRINT_TYPE_uint,\
           uint32_t : CTF__ASSERT_PRINT_TYPE_uint,\
           uint64_t : CTF__ASSERT_PRINT_TYPE_uint,\
           float: CTF__ASSERT_PRINT_TYPE_float,\
           double: CTF__ASSERT_PRINT_TYPE_float,\
           long double: CTF__ASSERT_PRINT_TYPE_float,\
           default    : CTF__ASSERT_PRINT_TYPE_ptr),\
#a, #b, __LINE__, __FILE__)
#endif
COMB(`ALIAS',
     `(assert_barrier(), assert_fold(count, msg), subtest(name),
       assert_true(a), assert_false(a), expect_true(a), expect_false(a),
       assert_null(a), assert_non_null(a), expect_null(a), expect_non_null(a))')
EA_ALIAS_FACTORY(`(PRIMITIVE_TYPES, str)', `(CMPS)', `EA_TEMPLATE')
EA_ALIAS_FACTORY(`(PRIMITIVE_TYPES)', `(CMPS)', `EA_MEM_TEMPLATE')
EA_ALIAS_FACTORY(`(PRIMITIVE_TYPES)', `(CMPS)', `EA_ARR_TEMPLATE')
EA_COMP_ALIAS_FACTORY(`(PRIMITIVE_TYPES, str)', `EA_COMP_TEMPLATE')
EA_COMP_ALIAS_FACTORY(`(PRIMITIVE_TYPES)', `EA_COMP_MEM_TEMPLATE')
EA_COMP_ALIAS_FACTORY(`(PRIMITIVE_TYPES)', `EA_COMP_ARR_TEMPLATE')
EA_COMP_ALIAS_FACTORY(`(PRIMITIVE_TYPES, str)', `EA_COMP_FUNCTION')
// clang-format on

#endif

include(`mocks.h')
#endif
