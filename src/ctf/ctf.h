#ifndef CTF_H
#define CTF_H

#ifndef CTF_ALIASES
#define CTF_ALIASES CTF_ON
#endif

include(`base.m4')
#include <setjmp.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <threads.h>

#define CTF_CONST_STATE_FILE_SIZE 256
#define CTF_CONST_STATE_MSG_SIZE 4096
#define CTF_CONST_STATES_PER_THREAD 64
#define CTF_CONST_MOCKS_PER_TEST 64
#define CTF_CONST_MAX_THREADS 16
#define CTF_CONST_GROUP_SIZE 128
#define CTF_CONST_MOCK_GROUP_SIZE 64
#define CTF_CONST_MOCK_CHECKS_PER_TEST 32

#define CTF_INTERNAL_MOCK_TYPE_uint u
#define CTF_INTERNAL_MOCK_TYPE_int i
#define CTF_INTERNAL_MOCK_TYPE_ptr p
#define CTF_INTERNAL_MOCK_TYPE_str p
#define CTF_INTERNAL_MOCK_TYPE_char c

#define CTF_INTERNAL_MOCK_TYPE_ASSERT 1
#define CTF_INTERNAL_MOCK_TYPE_ONCE 2
#define CTF_INTERNAL_MOCK_TYPE_MEMORY 4
#define CTF_INTERNAL_MOCK_TYPE_ARRAY 8

#define CTF_INTERNAL_STRINGIFY(x) #x
#define CTF_INTERNAL_STRINGIFY2(x) CTF_INTERNAL_STRINGIFY(x)
#define CTF_INTERNAL_LENGTH(a) (sizeof(a) / sizeof(*(a)))
#define CTF_INTERNAL_VA_ARGS(...) __VA_ARGS__

struct ctf_internal_state {
  int status;
  int line;
  const char *file;
  char msg[CTF_CONST_STATE_MSG_SIZE];
};
struct ctf_internal_test {
  void (*const f)(void);
  const char *name;
};
struct ctf_internal_group {
  const struct ctf_internal_test *tests;
  const char *name;
};
struct ctf_internal_check {
  const char *var;
  union {
    int (*i)(intmax_t, intmax_t, const char *, const char *, int, const char *);
    int (*u)(uintmax_t, uintmax_t, const char *, const char *, int,
             const char *);
    int (*p)(const void *, const void *, const char *, const char *, int,
             const char *);
    int (*c)(char, char, const char *, const char *, int, const char *);
    int (*s)(const char *, const char *, const char *, const char *, int,
             const char *);
    int (*m)(const void *, const void *, size_t, size_t, int, const char *,
             const char *, int, const char *);
    int (*a)(const void *, const void *, size_t, size_t, size_t, int,
             const char *, const char *, int, const char *);
  } f;
  int type;
  union {
    uintmax_t u;
    intmax_t i;
    char c;
    const void *p;
  } val;
  size_t length;
  int line;
  const char *file;
  const char *print_var;
};
struct ctf_internal_mock_state {
  const void *mock_f;
  int call_count;
  int return_override;
  struct ctf_internal_check check[CTF_CONST_MOCK_CHECKS_PER_TEST];
  jmp_buf assert_jump;
  int check_count;
};
struct ctf_internal_mock {
  struct ctf_internal_mock_state state[CTF_CONST_MAX_THREADS];
};
struct ctf_internal_mock_bind {
  struct ctf_internal_mock *mock;
  const void *f;
};

extern int ctf_exit_code;
extern thread_local struct ctf_internal_state *ctf_internal_states;
extern thread_local int ctf_internal_state_index;
extern thread_local int ctf_internal_parallel_thread_index;
extern thread_local struct ctf_internal_mock_state
  *ctf_internal_mock_reset_queue[CTF_CONST_MOCKS_PER_TEST];
extern thread_local int ctf_internal_mock_reset_count;

void ctf_internal_group_run(struct ctf_internal_group group);
void ctf_internal_groups_run(int count, ...);
#define ctf_group_run(test) ctf_internal_group_run(CTF_P_GROUP(test))
#define ctf_groups_run(...)                                    \
  ctf_internal_groups_run(                                     \
    sizeof((const struct ctf_internal_group[]){__VA_ARGS__}) / \
      sizeof(const struct ctf_internal_group),                 \
    __VA_ARGS__)
void ctf_print_buffer_dump(void);
void ctf_sigsegv_handler(int unused);
void ctf_parallel_start(void);
void ctf_parallel_stop(void);
void ctf_parallel_sync(void);
#define ctf_barrier()       \
  do {                      \
    ctf_parallel_sync();    \
    if(ctf_exit_code) {     \
      ctf_parallel_stop();  \
      return ctf_exit_code; \
    }                       \
  } while(0)

int ctf_internal_fail(const char *, int, const char *);
int ctf_internal_pass(const char *, int, const char *);
int ctf_internal_expect_msg(int, const char *, int, const char *);
int ctf_internal_expect_true(int, const char *, int, const char *);

#define CTF_EXTERN_TEST(name)                        \
  extern const char ctf_internal_test_name_##name[]; \
  void name(void)
#define CTF_TEST(name)                                \
  const char ctf_internal_test_name_##name[] = #name; \
  void name(void)
#define CTF_P_TEST(name) \
  (const struct ctf_internal_test) { name, ctf_internal_test_name_##name }
#define CTF_EXTERN_GROUP(name)                        \
  extern const char ctf_internal_group_name_##name[]; \
  extern const struct ctf_internal_test name[CTF_CONST_GROUP_SIZE];
#define CTF_GROUP(name)                                \
  const char ctf_internal_group_name_##name[] = #name; \
  const struct ctf_internal_test name[CTF_CONST_GROUP_SIZE]
#define CTF_P_GROUP(name) \
  (const struct ctf_internal_group) { name, ctf_internal_group_name_##name }

#define CTF_MOCK(ret_type, name, typed, args)                              \
  ret_type ctf_internal_mock_return_##name[CTF_CONST_MAX_THREADS];         \
  struct ctf_internal_mock ctf_internal_mock_##name;                       \
  ret_type __real_##name typed;                                            \
  ret_type __wrap_##name typed {                                           \
    struct ctf_internal_mock_state *_data =                                \
      ctf_internal_mock_##name.state + ctf_internal_parallel_thread_index; \
    ret_type(*const _mock) typed = _data->mock_f;                          \
    if(_mock == NULL) {                                                    \
      return __real_##name args;                                           \
    } else {                                                               \
      _data->call_count++;                                                 \
      if(_data->return_override) {                                         \
        _mock args;                                                        \
        if(_data->return_override == 2) _data->return_override = 0;        \
        return ctf_internal_mock_return_##name                             \
          [ctf_internal_parallel_thread_index];                            \
      }                                                                    \
      return _mock args;                                                   \
    }                                                                      \
  }
#define CTF_EXTERN_MOCK(ret_type, name, typed)                            \
  extern ret_type ctf_internal_mock_return_##name[CTF_CONST_MAX_THREADS]; \
  extern struct ctf_internal_mock ctf_internal_mock_##name;               \
  ret_type __real_##name typed;                                           \
  ret_type __wrap_##name typed;
#define CTF_MOCK_BIND(fn, mock) \
  (struct ctf_internal_mock_bind) { &ctf_internal_mock_##fn, mock }
#define CTF_MOCK_GROUP(name) \
  struct ctf_internal_mock_bind name[CTF_CONST_MOCK_GROUP_SIZE]
#define CTF_EXTERN_MOCK_GROUP(name) \
  extern struct ctf_internal_mock_state_bind name[CTF_CONST_MOCK_GROUP_SIZE];
#define ctf_mock_group(name)                                                 \
  do {                                                                       \
    for(size_t _i = 0; _i < CTF_CONST_MOCK_GROUP_SIZE && name[_i].f; _i++) { \
      name[_i].mock->state[ctf_internal_parallel_thread_index].mock_f =      \
        name[_i].f;                                                          \
      ctf_internal_mock_reset_queue[ctf_internal_mock_reset_count++] =       \
        name[_i].mock->state + ctf_internal_parallel_thread_index;           \
    }                                                                        \
  } while(0)
#define ctf_mock(fn, mock)                                                  \
  do {                                                                      \
    struct ctf_internal_mock_state *const state =                           \
      ctf_internal_mock_##fn.state + ctf_internal_parallel_thread_index;    \
    state->mock_f = mock;                                                   \
    ctf_internal_mock_reset_queue[ctf_internal_mock_reset_count++] = state; \
  } while(0)
#define ctf_unmock(fn) \
  ctf_internal_mock_##fn.state[ctf_internal_parallel_thread_index].mock_f = NULL
#define ctf_mock_call_count(name)                                     \
  (ctf_internal_mock_##name.state[ctf_internal_parallel_thread_index] \
     .call_count)
#define ctf_mock_will_return(name, val)                                        \
  do {                                                                         \
    ctf_internal_mock_return_##name[ctf_internal_parallel_thread_index] = val; \
    ctf_internal_mock_##name.state[ctf_internal_parallel_thread_index]         \
      .return_override = 1;                                                    \
  } while(0)
#define ctf_mock_will_return_once(name, val)                                   \
  do {                                                                         \
    ctf_internal_mock_return_##name[ctf_internal_parallel_thread_index] = val; \
    ctf_internal_mock_##name.state[ctf_internal_parallel_thread_index]         \
      .return_override = 2;                                                    \
  } while(0)

int ctf_internal_pass(const char *, int, const char *);
int ctf_internal_fail(const char *, int, const char *);
#define ctf_pass(m) ctf_internal_pass(m, __LINE__, __FILE__)
#define ctf_fail(m) ctf_internal_fail(m, __LINE__, __FILE__)

// clang-format off
/*
define(`MOCK_FUNCTION_HELPER', `void ctf_internal_mock_$1(struct ctf_internal_mock*, int, int, const char *, const char*, void *, const char*, $2 val);')
define(`MOCK_FUNCTION', `MOCK_FUNCTION_HELPER(`$1',TYPE(`$1'))')
define(`MOCK_CHECK_FUNCTION_HELPER', `void ctf_internal_mock_check_$1(struct ctf_internal_mock_state *state, $2 v, const char *v_print);')
define(`MOCK_CHECK_MEMORY_FUNCTION_HELPER', `void ctf_internal_mock_check_memory_$1(struct ctf_internal_mock_state *state, const void *v, const char *v_print, size_t step, int sign);')
define(`MOCK_CHECK_FUNCTION', `MOCK_CHECK_FUNCTION_HELPER(`$1', TYPE(`$1'))')
define(`MOCK_CHECK_MEMORY_FUNCTION', `MOCK_CHECK_MEMORY_FUNCTION_HELPER(`$1', TYPE(`$1'))')
define(`MOCK_EXPECT', `#define ctf_mock_expect_$1_$2(name, var, val) CTF_INTERNAL_MOCK_EXPECT($1, $2, 0, name, #var, val);')
define(`MOCK_ASSERT', `#define ctf_mock_assert_$1_$2(name, var, val) CTF_INTERNAL_MOCK_EXPECT($1, $2, CTF_INTERNAL_MOCK_TYPE_ASSERT, name, #var, val);')
define(`MOCK_EXPECT_ONCE', `#define ctf_mock_expect_once_$1_$2(name, var, val) CTF_INTERNAL_MOCK_EXPECT($1, $2, CTF_INTERNAL_MOCK_TYPE_ONCE, name, #var, val);')
define(`MOCK_ASSERT_ONCE', `#define ctf_mock_assert_once_$1_$2(name, var, val) CTF_INTERNAL_MOCK_EXPECT($1, $2, CTF_INTERNAL_MOCK_TYPE_ONCE | CTF_INTERNAL_MOCK_TYPE_ASSERT, name, #var, val);')
define(`MOCK_EXPECT_MEMORY', `#define ctf_mock_expect_memory_$1_$2(name, var, val, length) CTF_INTERNAL_MOCK_EXPECT_MEMORY($1, $2, 0, name, #var, val, length);')
define(`MOCK_ASSERT_MEMORY', `#define ctf_mock_assert_memory_$1_$2(name, var, val, length) CTF_INTERNAL_MOCK_ASSERT_MEMORY($1, $2, 0, name, #var, val, length);')
define(`MOCK_EXPECT_ARRAY', `#define ctf_mock_expect_array_$1_$2(name, var, val) CTF_INTERNAL_MOCK_EXPECT_MEMORY($1, $2, 0, name, #var, val, sizeof(val)/sizeof(*(val)));')
define(`MOCK_ASSERT_ARRAY', `#define ctf_mock_assert_array_$1_$2(name, var, val) CTF_INTERNAL_MOCK_ASSERT_MEMORY($1, $2, 0, name, #var, val, sizeof(val)/sizeof(*(val)));')
define(`MOCK_EXPECT_ONCE_MEMORY', `#define ctf_mock_expect_once_memory_$1_$2(name, var, val, length) CTF_INTERNAL_MOCK_EXPECT_MEMORY($1, $2, CTF_INTERNAL_MOCK_TYPE_ONCE, name, #var, val, length);')
define(`MOCK_ASSERT_ONCE_MEMORY', `#define ctf_mock_assert_once_memory_$1_$2(name, var, val, length) CTF_INTERNAL_MOCK_ASSERT_MEMORY($1, $2, CTF_INTERNAL_MOCK_TYPE_ONCE, name, #var, val, length);')
define(`MOCK_EXPECT_ONCE_ARRAY', `#define ctf_mock_expect_once_array_$1_$2(name, var, val) CTF_INTERNAL_MOCK_EXPECT_MEMORY($1, $2, CTF_INTERNAL_MOCK_TYPE_ONCE, name, #var, val, sizeof(val)/sizeof(*(val)));')
define(`MOCK_ASSERT_ONCE_ARRAY', `#define ctf_mock_assert_once_array_$1_$2(name, var, val) CTF_INTERNAL_MOCK_ASSERT_MEMORY($1, $2, CTF_INTERNAL_MOCK_TYPE_ONCE, name, #var, val, sizeof(val)/sizeof(*(val)));')
define(`MOCK_CHECK', `#define ctf_mock_check_$1(name, v) ctf_internal_mock_check_$1(ctf_internal_mock_##name.state + ctf_internal_parallel_thread_index, v, #v)')
define(`MOCK_CHECK_STRING', `#define ctf_mock_check_str(name, v) \
ctf_internal_mock_check_ptr(ctf_internal_mock_##name.state + ctf_internal_parallel_thread_index, v, #v); \
ctf_internal_mock_check_str(ctf_internal_mock_##name.state + ctf_internal_parallel_thread_index, v, #v)')
define(`MOCK_CHECK_MEMORY',
`format(`#define ctf_mock_check_memory_$1(name, v) \
ctf_internal_mock_check_ptr(ctf_internal_mock_##name.state + ctf_internal_parallel_thread_index, v, #v); \
    ctf_internal_mock_check_memory_$1(ctf_internal_mock_##name.state + ctf_internal_parallel_thread_index, v, #v, sizeof(*(v)), %d)'
  ,ifelse(`$1',`int',1,0))')
define(`EA_TEMPLATE', `#define ctf_$3_$1_$2(a, b) CTF_INTERNAL_$4(ctf_internal_expect_$1_$2, a, b, #a, #b, __LINE__, __FILE__)')dnl
define(`EA_MEMORY_TEMPLATE',
`format(`#define ctf_$3_memory_$1_$2(a, b, length) CTF_INTERNAL_$4(ctf_internal_expect_memory_$1_$2,(const void *)a, (const void *)b, length,  sizeof(*(a)), %d, #a, #b, __LINE__, __FILE__)', ifelse(`$1', `int', `1', `0'))')dnl
define(`EA_ARRAY_TEMPLATE',
`format(`#define ctf_$3_array_$1_$2(a, b) CTF_INTERNAL_$4(ctf_internal_expect_array_$1_$2,(const void *const *)a, (const void *const *)b, sizeof(a)/sizeof(*(a)), sizeof(b)/sizeof(*(b)),  sizeof(*(a)), %d, #a, #b, __LINE__, __FILE__)', ifelse(`$1', `int', `1', `0'))')dnl
define(`EXPECT_FUNCTION',
`format(`int ctf_internal_expect_$1_$2(%s, %s, const char *, const char *, int, const char *);',TYPE(`$1'),TYPE(`$1'))')dnl
define(`EXPECT_MEMORY_PRIMITIVE_FUNCTION',
`int ctf_internal_expect_memory_$1_$2(const void *, const void *, size_t, size_t, int, const char *, const char *, int, const char *);')dnl
define(`EXPECT_ARRAY_PRIMITIVE_FUNCTION',
`int ctf_internal_expect_array_$1_$2(const void *, const void *, size_t, size_t, size_t, int, const char *, const char *, int, const char*);')dnl
define(`ASSERT_WRAP',
`indir(`$1', `$2', `$3', `assert', `ASSERT')
indir(`$1', `$2', `$3', `expect', `EXPECT')
')dnl
define(`EA_FACTORY', `foreach(`type', `$1', `foreach(`comp', `$2', `indir(`ASSERT_WRAP', `$3', type, comp)')')')dnl
*/
COMB(`MOCK_FUNCTION', `(PRIMITIVE_TYPES, str)')
COMB2(`MOCK_EXPECT', `(char, int, uint, ptr, str)', `(CMPS)')
COMB2(`MOCK_ASSERT', `(char, int, uint, ptr, str)', `(CMPS)')
COMB2(`MOCK_EXPECT_ONCE', `(char, int, uint, ptr, str)', `(CMPS)')
COMB2(`MOCK_ASSERT_ONCE', `(char, int, uint, ptr, str)', `(CMPS)')
COMB2(`MOCK_EXPECT_MEMORY', `(char, int, uint, ptr)', `(CMPS)')
COMB2(`MOCK_EXPECT_ARRAY', `(char, int, uint, ptr)', `(CMPS)')
COMB2(`MOCK_ASSERT_MEMORY', `(char, int, uint, ptr)', `(CMPS)')
COMB2(`MOCK_ASSERT_ARRAY', `(char, int, uint, ptr)', `(CMPS)')
COMB2(`MOCK_EXPECT_ONCE_MEMORY', `(char, int, uint, ptr)', `(CMPS)')
COMB2(`MOCK_EXPECT_ONCE_ARRAY', `(char, int, uint, ptr)', `(CMPS)')
COMB2(`MOCK_ASSERT_ONCE_MEMORY', `(char, int, uint, ptr)', `(CMPS)')
COMB2(`MOCK_ASSERT_ONCE_ARRAY', `(char, int, uint, ptr)', `(CMPS)')
COMB2(`EXPECT_FUNCTION', `(int, uint, char, ptr, str)', `(CMPS)')
COMB2(`EXPECT_MEMORY_PRIMITIVE_FUNCTION', `(PRIMITIVE_TYPES)', `(CMPS)')
COMB2(`EXPECT_ARRAY_PRIMITIVE_FUNCTION', `(PRIMITIVE_TYPES)', `(CMPS)')
EA_FACTORY(`(PRIMITIVE_TYPES, str)', `(CMPS)', `EA_TEMPLATE')
EA_FACTORY(`(PRIMITIVE_TYPES)', `(CMPS)', `EA_MEMORY_TEMPLATE')
EA_FACTORY(`(PRIMITIVE_TYPES)', `(CMPS)', `EA_ARRAY_TEMPLATE')
COMB(`MOCK_CHECK_FUNCTION', `(PRIMITIVE_TYPES, str)')
COMB(`MOCK_CHECK_MEMORY_FUNCTION', `(PRIMITIVE_TYPES, str)')
COMB(`MOCK_CHECK', `(PRIMITIVE_TYPES)')
MOCK_CHECK_STRING
COMB(`MOCK_CHECK_MEMORY', `(PRIMITIVE_TYPES)')
// clang-format on
void ctf_internal_mock_memory(struct ctf_internal_mock *mock, int type,
                              int, const char *, const char*, void *f,
                              const char *var, const void *val, size_t l);

#define CTF_INTERNAL_MOCK_EXPECT_MEMORY(t, comp, type, name, var, val, l)      \
  if(ctf_internal_mock_##name.state[ctf_internal_parallel_thread_index]        \
       .check_count < CTF_CONST_MOCK_CHECKS_PER_TEST)                          \
  ctf_internal_mock_memory(                                                    \
    &ctf_internal_mock_##name, type | CTF_INTERNAL_MOCK_TYPE_MEMORY, __LINE__, \
    __FILE__, #val, (void *)ctf_internal_expect_memory_##t##_##comp, var, val, \
    l)
#define CTF_INTERNAL_MOCK_ASSERT_MEMORY(t, comp, type, name, var, val, l)     \
  do {                                                                        \
    if(ctf_internal_mock_##name.state[ctf_internal_parallel_thread_index]     \
         .check_count < CTF_CONST_MOCK_CHECKS_PER_TEST)                       \
      ctf_internal_mock_memory(                                               \
        &ctf_internal_mock_##name,                                            \
        type | CTF_INTERNAL_MOCK_TYPE_ASSERT | CTF_INTERNAL_MOCK_TYPE_MEMORY, \
        __LINE__, __FILE__, #val,                                             \
        (void *)ctf_internal_expect_memory_##t##_##comp, var, val, l);        \
    if(setjmp(                                                                \
         ctf_internal_mock_##name.state[ctf_internal_parallel_thread_index]   \
           .assert_jump))                                                     \
      return;                                                                 \
  } while(0)
#define CTF_INTERNAL_MOCK_EXPECT(t, comp, type, name, var, val)              \
  if(ctf_internal_mock_##name.state[ctf_internal_parallel_thread_index]      \
       .check_count < CTF_CONST_MOCK_CHECKS_PER_TEST)                        \
  ctf_internal_mock_##t(&ctf_internal_mock_##name, type, __LINE__, __FILE__, \
                        #val, (void *)ctf_internal_expect_##t##_##comp, var, \
                        val)
#define CTF_INTERNAL_MOCK_ASSERT(t, comp, type, name, var, val)             \
  do {                                                                      \
    if(ctf_internal_mock_##name.state[ctf_internal_parallel_thread_index]   \
         .check_count < CTF_CONST_MOCK_CHECKS_PER_TEST)                     \
      ctf_internal_mock_##t(                                                \
        &ctf_internal_mock_##name, type | CTF_INTERNAL_MOCK_TYPE_ASSERT,    \
        __LINE__, __FILE__, #val, (void *)ctf_internal_expect_##t##_##comp, \
        var, val);                                                          \
    if(setjmp(                                                              \
         ctf_internal_mock_##name.state[ctf_internal_parallel_thread_index] \
           .assert_jump))                                                   \
      return;                                                               \
  } while(0)

#if CTF_ALIASES == CTF_ON
#define CTF_INTERNAL_EXPECT(f, ...) \
  if(ctf_internal_state_index < CTF_CONST_STATES_PER_THREAD) f(__VA_ARGS__)
#define CTF_INTERNAL_ASSERT(f, ...)                                            \
  do {                                                                         \
    if(ctf_internal_state_index < CTF_CONST_STATES_PER_THREAD) f(__VA_ARGS__); \
    if(ctf_internal_states[ctf_internal_state_index - 1].status) return;       \
  } while(0)
// clang-format off
/*
define(`ALIAS', `#define $1 ctf_$1
')dnl
define(`MACRO_ALIAS', `#define $1 CTF_$1
')dnl
define(`EA_ALIAS', `ALIAS($3_$1_$2(a, b))')dnl
define(`EA_MEMORY_ALIAS', `ALIAS($3_memory_$1_$2(a, b, length))')dnl
define(`EA_ARRAY_ALIAS', `ALIAS($3_array_$1_$2(a, b))')dnl
define(`MOCK_EA_ALIAS', `ALIAS(mock_$3_$1_$2(name, var, val))')
define(`MOCK_EA_ONCE_ALIAS', `ALIAS(mock_$3_once_$1_$2(name, var, val))')
define(`MOCK_EA_MEMORY_ALIAS', `ALIAS(mock_$3_memory_$1_$2(name, var, val, length))')
define(`MOCK_EA_ARRAY_ALIAS', `ALIAS(mock_$3_array_$1_$2(name, var, val))')
define(`MOCK_EA_ONCE_MEMORY_ALIAS', `ALIAS(mock_$3_once_memory_$1_$2(name, var, val, length))')
define(`MOCK_EA_ONCE_ARRAY_ALIAS', `ALIAS(mock_$3_once_array_$1_$2(name, var, val))')
define(`MOCK_CHECK_ALIAS', `ALIAS(mock_check_$1(name, val))')
define(`MOCK_CHECK_MEMORY_ALIAS', `ALIAS(mock_check_memory_$1(name, val))')
define(`EXTERN_P', `$1, EXTERN_$1, P_$1')
*/
COMB(`MACRO_ALIAS',
`(EXTERN_P(TEST(name)), EXTERN_P(GROUP(name)),
  MOCK(ret_type, name, typed, args), EXTERN_MOCK(ret_type, name, typed),
  MOCK_BIND(fn, mock), MOCK_GROUP(name), EXTERN_MOCK_GROUP(name))')
COMB(`ALIAS',
`(mock(name, f), mock_group(name), mock_call_count(name),
  mock_will_return(name, val), mock_will_return_once(name, val))')
EA_FACTORY(`(PRIMITIVE_TYPES, str)', `(CMPS)', `EA_ALIAS')
EA_FACTORY(`(PRIMITIVE_TYPES)', `(CMPS)', `EA_MEMORY_ALIAS')
EA_FACTORY(`(PRIMITIVE_TYPES)', `(CMPS)', `EA_ARRAY_ALIAS')
EA_FACTORY(`(PRIMITIVE_TYPES, str)', `(CMPS)', `MOCK_EA_ALIAS')
EA_FACTORY(`(PRIMITIVE_TYPES, str)', `(CMPS)', `MOCK_EA_ONCE_ALIAS')
EA_FACTORY(`(PRIMITIVE_TYPES)', `(CMPS)', `MOCK_EA_MEMORY_ALIAS')
EA_FACTORY(`(PRIMITIVE_TYPES)', `(CMPS)', `MOCK_EA_ARRAY_ALIAS')
EA_FACTORY(`(PRIMITIVE_TYPES)', `(CMPS)', `MOCK_EA_ONCE_MEMORY_ALIAS')
EA_FACTORY(`(PRIMITIVE_TYPES)', `(CMPS)', `MOCK_EA_ONCE_ARRAY_ALIAS')
COMB(`MOCK_CHECK_ALIAS', `(PRIMITIVE_TYPES, str)')
COMB(`MOCK_CHECK_MEMORY_ALIAS', `(PRIMITIVE_TYPES)')
// clang-format on

#endif
#endif
