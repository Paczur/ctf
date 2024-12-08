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
#include <stdint.h>
#include <stdio.h>
#include <threads.h>

#define CTF_CONST_STATE_MSG_SIZE 4096
#define CTF_CONST_STATES_PER_THREAD 64
#define CTF_CONST_MOCKS_PER_TEST 64
#define CTF_CONST_MAX_THREADS 16
#define CTF_CONST_GROUP_SIZE 128
#define CTF_CONST_MOCK_GROUP_SIZE 64
#define CTF_CONST_MOCK_CHECKS_PER_TEST 64
#define CTF_CONST_SIGNAL_STACK_SIZE 1024

#define CTF_INTERNAL_MOCK_TYPE_ASSERT 1
#define CTF_INTERNAL_MOCK_TYPE_ONCE 2
#define CTF_INTERNAL_MOCK_TYPE_MEMORY 4

#define CTF_INTERNAL_STRINGIFY(x) #x
#define CTF_INTERNAL_STRINGIFY2(x) CTF_INTERNAL_STRINGIFY(x)
#define CTF_INTERNAL_LENGTH(a) (sizeof(a) / sizeof(*(a)))
#define CTF_INTERNAL_VA_ARGS(...) __VA_ARGS__
#define CTF_INTERNAL_EA(f, ...)                                           \
  do {                                                                    \
    prereq_assert(ctf_internal_state_index < CTF_CONST_STATES_PER_THREAD, \
                  "Limit for asserts/expects per test reached");          \
    f(__VA_ARGS__);                                                       \
  } while(0)
#define CTF_INTERNAL_MOCK_EXPECT_MEMORY(t, comp, type, name, var, val, l)   \
  do {                                                                      \
    prereq_assert(                                                          \
      ctf_internal_mock_st_##name.state[ctf_internal_parallel_thread_index] \
          .mock_f != NULL,                                                  \
      "Mock expect/assert used without mocking beforehand");                \
    prereq_assert(                                                          \
      ctf_internal_mock_st_##name.state[ctf_internal_parallel_thread_index] \
          .check_count < CTF_CONST_MOCK_CHECKS_PER_TEST,                    \
      "Limit for mock asserts/expects per test reached");                   \
    ctf_internal_mock_memory(                                               \
      &ctf_internal_mock_st_##name, type | CTF_INTERNAL_MOCK_TYPE_MEMORY,   \
      __LINE__, __FILE__, #val,                                             \
      (void *)ctf_internal_expect_memory_##t##_##comp, var, val, l);        \
  } while(0)
#define CTF_INTERNAL_MOCK_ASSERT_MEMORY(t, comp, type, name, var, val, l)   \
  do {                                                                      \
    prereq_assert(                                                          \
      ctf_internal_mock_st_##name.state[ctf_internal_parallel_thread_index] \
          .mock_f != NULL,                                                  \
      "Mock expect/assert used without mocking beforehand");                \
    prereq_assert(                                                          \
      ctf_internal_mock_st_##name.state[ctf_internal_parallel_thread_index] \
          .check_count < CTF_CONST_MOCK_CHECKS_PER_TEST,                    \
      "Limit for mock asserts/expects per test reached");                   \
    ctf_internal_mock_memory(                                               \
      &ctf_internal_mock_st_##name,                                         \
      type | CTF_INTERNAL_MOCK_TYPE_ASSERT | CTF_INTERNAL_MOCK_TYPE_MEMORY, \
      __LINE__, __FILE__, #val,                                             \
      (void *)ctf_internal_expect_memory_##t##_##comp, var, val, l);        \
  } while(0)
#define CTF_INTERNAL_MOCK_EXPECT(t, comp, type, name, var, val)                \
  do {                                                                         \
    prereq_assert(                                                             \
      ctf_internal_mock_st_##name.state[ctf_internal_parallel_thread_index]    \
          .mock_f != NULL,                                                     \
      "Mock expect/assert used without mocking beforehand");                   \
    prereq_assert(                                                             \
      ctf_internal_mock_st_##name.state[ctf_internal_parallel_thread_index]    \
          .check_count < CTF_CONST_MOCK_CHECKS_PER_TEST,                       \
      "Limit for mock asserts/expects per test reached");                      \
    ctf_internal_mock_##t(&ctf_internal_mock_st_##name, type, __LINE__,        \
                          __FILE__, #val,                                      \
                          (void *)ctf_internal_expect_##t##_##comp, var, val); \
  } while(0)
#define CTF_INTERNAL_MOCK_ASSERT(t, comp, type, name, var, val)                \
  do {                                                                         \
    preqreq_assert(                                                            \
      ctf_internal_mock_st_##name.state[ctf_internal_parallel_thread_index]    \
          .mock_f != NULL,                                                     \
      "Mock expect/assert used without mocking beforehand");                   \
    prereq_assert(                                                             \
      ctf_internal_mock_st_##name.state[ctf_internal_parallel_thread_index]    \
          .check_count < CTF_CONST_MOCK_CHECKS_PER_TEST,                       \
      "Limit fzoor mock asserts/expects per test reached");                    \
    ctf_internal_mock_##t(&ctf_internal_mock_st_##name,                        \
                          type | CTF_INTERNAL_MOCK_TYPE_ASSERT, __LINE__,      \
                          __FILE__, #val,                                      \
                          (void *)ctf_internal_expect_##t##_##comp, var, val); \
  } while(0)

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
  void (*setup)(void);
  void (*teardown)(void);
  void (*test_setup)(void);
  void (*test_teardown)(void);
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
    int (*m)(const void *, const void *, uintmax_t, uintmax_t, int,
             const char *, const char *, int, const char *);
  } f;
  int type;
  uintmax_t length;
  int line;
  const char *file;
  const char *print_var;
  union {
    uintmax_t u;
    intmax_t i;
    char c;
    const void *p;
  } val;
};
struct ctf_internal_mock_state {
  const void *mock_f;
  int call_count;
  int return_override;
  struct ctf_internal_check check[CTF_CONST_MOCK_CHECKS_PER_TEST];
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
extern thread_local uintmax_t ctf_internal_state_index;
extern thread_local uintmax_t ctf_internal_parallel_thread_index;
extern thread_local struct ctf_internal_mock_state
  *ctf_internal_mock_reset_queue[CTF_CONST_MOCKS_PER_TEST];
extern thread_local uintmax_t ctf_internal_mock_reset_count;

void ctf_group_run(struct ctf_internal_group group);
void ctf_internal_groups_run(int count, ...);
#define ctf_groups_run(...)                                    \
  ctf_internal_groups_run(                                     \
    sizeof((const struct ctf_internal_group[]){__VA_ARGS__}) / \
      sizeof(const struct ctf_internal_group),                 \
    __VA_ARGS__)
void ctf_sigsegv_handler(int unused);
extern char ctf_signal_altstack[CTF_CONST_SIGNAL_STACK_SIZE];
void ctf_parallel_start(void);
void ctf_parallel_stop(void);
void ctf_parallel_sync(void);
void ctf_assert_barrier(void);
void ctf_internal_assert_fold(uintmax_t count, const char *msg, int line, const char *file);
int ctf_internal_mock_call_count(struct ctf_internal_mock_state *state);
void ctf_assert_hide(uintmax_t count);
#define ctf_assert_fold(count, msg) \
  ctf_internal_assert_fold(count, msg, __LINE__, __FILE__)
#define ctf_barrier()      \
  do {                     \
    ctf_parallel_sync();   \
    if(ctf_exit_code) {    \
      ctf_parallel_stop(); \
      return;              \
    }                      \
  } while(0)

#define CTF_TEST_STATIC(test_name)                                     \
  static void ctf_internal_test_fn_##test_name(void);                  \
  static const char ctf_internal_test_name_##test_name[] = #test_name; \
  const struct ctf_internal_test test_name = {                         \
    .name = ctf_internal_test_name_##test_name,                        \
    .f = ctf_internal_test_fn_##test_name,                             \
  };                                                                   \
  static void ctf_internal_test_fn_##test_name(void)
#define CTF_TEST_EXTERN(name) extern const struct ctf_internal_test name;
#define CTF_TEST(test_name)                                            \
  static void ctf_internal_test_fn_##test_name(void);                  \
  static const char ctf_internal_test_name_##test_name[] = #test_name; \
  const struct ctf_internal_test test_name = {                         \
    .name = ctf_internal_test_name_##test_name,                        \
    .f = ctf_internal_test_fn_##test_name,                             \
  };                                                                   \
  void ctf_internal_test_fn_##test_name(void)
#define CTF_GROUP_STATIC(group_name)                                           \
  static const char ctf_internal_group_name_##group_name[] = #group_name;      \
  static const struct ctf_internal_test ctf_internal_group_arr_##group_name[]; \
  static void (*ctf_internal_group_setup_p_##group_name)(void);                \
  static void (*ctf_internal_group_teardown_p_##group_name)(void);             \
  static void (*ctf_internal_group_test_setup_p_##group_name)(void);           \
  static void (*ctf_internal_group_test_teardown_p_##group_name)(void);        \
  static void ctf_internal_group_setup_##group_name(void) {                    \
    if(ctf_internal_group_setup_p_##group_name)                                \
      ctf_internal_group_setup_p_##group_name();                               \
  }                                                                            \
  static void ctf_internal_group_teardown_##group_name(void) {                 \
    if(ctf_internal_group_teardown_p_##group_name)                             \
      ctf_internal_group_teardown_p_##group_name();                            \
  }                                                                            \
  static void ctf_internal_group_test_setup_##group_name(void) {               \
    if(ctf_internal_group_test_setup_p_##group_name)                           \
      ctf_internal_group_test_setup_p_##group_name();                          \
  }                                                                            \
  static void ctf_internal_group_test_teardown_##group_name(void) {            \
    if(ctf_internal_group_test_teardown_p_##group_name)                        \
      ctf_internal_group_test_teardown_p_##group_name();                       \
  }                                                                            \
  static const struct ctf_internal_group group_name = {                        \
    .tests = ctf_internal_group_arr_##group_name,                              \
    .setup = ctf_internal_group_setup_##group_name,                            \
    .teardown = ctf_internal_group_teardown_##group_name,                      \
    .test_setup = ctf_internal_group_test_setup_##group_name,                  \
    .test_teardown = ctf_internal_group_test_teardown_##group_name,            \
    .name = ctf_internal_group_name_##group_name,                              \
  };                                                                           \
  static const struct ctf_internal_test                                        \
    ctf_internal_group_arr_##group_name[CTF_CONST_GROUP_SIZE]
#define CTF_GROUP_EXTERN(name) extern const struct ctf_internal_group name;
#define CTF_GROUP(group_name)                                                  \
  static const char ctf_internal_group_name_##group_name[] = #group_name;      \
  static const struct ctf_internal_test ctf_internal_group_arr_##group_name[]; \
  static void (*ctf_internal_group_setup_p_##group_name)(void);                \
  static void (*ctf_internal_group_teardown_p_##group_name)(void);             \
  static void (*ctf_internal_group_test_setup_p_##group_name)(void);           \
  static void (*ctf_internal_group_test_teardown_p_##group_name)(void);        \
  static void ctf_internal_group_setup_##group_name(void) {                    \
    if(ctf_internal_group_setup_p_##group_name)                                \
      ctf_internal_group_setup_p_##group_name();                               \
  }                                                                            \
  static void ctf_internal_group_teardown_##group_name(void) {                 \
    if(ctf_internal_group_teardown_p_##group_name)                             \
      ctf_internal_group_teardown_p_##group_name();                            \
  }                                                                            \
  static void ctf_internal_group_test_setup_##group_name(void) {               \
    if(ctf_internal_group_test_setup_p_##group_name)                           \
      ctf_internal_group_test_setup_p_##group_name();                          \
  }                                                                            \
  static void ctf_internal_group_test_teardown_##group_name(void) {            \
    if(ctf_internal_group_test_teardown_p_##group_name)                        \
      ctf_internal_group_test_teardown_p_##group_name();                       \
  }                                                                            \
  const struct ctf_internal_group group_name = {                               \
    .tests = ctf_internal_group_arr_##group_name,                              \
    .setup = ctf_internal_group_setup_##group_name,                            \
    .teardown = ctf_internal_group_teardown_##group_name,                      \
    .test_setup = ctf_internal_group_test_setup_##group_name,                  \
    .test_teardown = ctf_internal_group_test_teardown_##group_name,            \
    .name = ctf_internal_group_name_##group_name,                              \
  };                                                                           \
  static const struct ctf_internal_test                                        \
    ctf_internal_group_arr_##group_name[CTF_CONST_GROUP_SIZE]
#define CTF_GROUP_SETUP(name)                              \
  static void ctf_internal_group_setup_def_##name(void);   \
  static void (*ctf_internal_group_setup_p_##name)(void) = \
    ctf_internal_group_setup_def_##name;                   \
  static void ctf_internal_group_setup_def_##name(void)
#define CTF_GROUP_TEARDOWN(name)                              \
  static void ctf_internal_group_teardown_def_##name(void);   \
  static void (*ctf_internal_group_teardown_p_##name)(void) = \
    ctf_internal_group_teardown_def_##name;                   \
  static void ctf_internal_group_teardown_def_##name(void)
#define CTF_GROUP_TEST_SETUP(name)                              \
  static void ctf_internal_group_test_setup_def_##name(void);   \
  static void (*ctf_internal_group_test_setup_p_##name)(void) = \
    ctf_internal_group_test_setup_def_##name;                   \
  static void ctf_internal_group_test_setup_def_##name(void)
#define CTF_GROUP_TEST_TEARDOWN(name)                              \
  static void ctf_internal_group_test_teardown_def_##name(void);   \
  static void (*ctf_internal_group_test_teardown_p_##name)(void) = \
    ctf_internal_group_test_teardown_def_##name;                   \
  static void ctf_internal_group_test_teardown_def_##name(void)

#define CTF_MOCK(ret_type, name, typed, args)                                 \
  ret_type ctf_internal_mock_return_##name[CTF_CONST_MAX_THREADS];            \
  struct ctf_internal_mock ctf_internal_mock_st_##name;                       \
  ret_type __real_##name typed;                                               \
  ret_type __wrap_##name typed {                                              \
    struct ctf_internal_mock_state *_data =                                   \
      ctf_internal_mock_st_##name.state + ctf_internal_parallel_thread_index; \
    ret_type(*const _mock) typed = _data->mock_f;                             \
    if(_mock == NULL) {                                                       \
      return __real_##name args;                                              \
    } else {                                                                  \
      _data->call_count++;                                                    \
      if(_data->return_override) {                                            \
        _mock args;                                                           \
        if(_data->return_override == 2) _data->return_override = 0;           \
        return ctf_internal_mock_return_##name                                \
          [ctf_internal_parallel_thread_index];                               \
      }                                                                       \
      return _mock args;                                                      \
    }                                                                         \
  }
#define CTF_MOCK_VOID_RET(name, typed, args)                                  \
  struct ctf_internal_mock ctf_internal_mock_st_##name;                       \
  void __real_##name typed;                                                   \
  void __wrap_##name typed {                                                  \
    struct ctf_internal_mock_state *_data =                                   \
      ctf_internal_mock_st_##name.state + ctf_internal_parallel_thread_index; \
    void(*const _mock) typed = _data->mock_f;                                 \
    if(_mock == NULL) {                                                       \
      return __real_##name args;                                              \
    } else {                                                                  \
      _data->call_count++;                                                    \
      return _mock args;                                                      \
    }                                                                         \
  }
#define CTF_MOCK_STATIC(ret_type, name, typed, args)                          \
  static ret_type ctf_internal_mock_return_##name[CTF_CONST_MAX_THREADS];     \
  static struct ctf_internal_mock ctf_internal_mock_st_##name;                \
  static ret_type __real_##name typed;                                        \
  static ret_type __wrap_##name typed {                                       \
    struct ctf_internal_mock_state *_data =                                   \
      ctf_internal_mock_st_##name.state + ctf_internal_parallel_thread_index; \
    ret_type(*const _mock) typed = _data->mock_f;                             \
    if(_mock == NULL) {                                                       \
      return __real_##name args;                                              \
    } else {                                                                  \
      _data->call_count++;                                                    \
      if(_data->return_override) {                                            \
        _mock args;                                                           \
        if(_data->return_override == 2) _data->return_override = 0;           \
        return ctf_internal_mock_return_##name                                \
          [ctf_internal_parallel_thread_index];                               \
      }                                                                       \
      return _mock args;                                                      \
    }                                                                         \
  }
#define CTF_MOCK_VOID_RET_STATIC(name, typed, args)                           \
  static struct ctf_internal_mock ctf_internal_mock_st_##name;                \
  static void __real_##name typed;                                            \
  static void __wrap_##name typed {                                           \
    struct ctf_internal_mock_state *_data =                                   \
      ctf_internal_mock_st_##name.state + ctf_internal_parallel_thread_index; \
    void(*const _mock) typed = _data->mock_f;                                 \
    if(_mock == NULL) {                                                       \
      return __real_##name args;                                              \
    } else {                                                                  \
      _data->call_count++;                                                    \
      return _mock args;                                                      \
    }                                                                         \
  }
#define CTF_MOCK_EXTERN(ret_type, name, typed)                            \
  extern ret_type ctf_internal_mock_return_##name[CTF_CONST_MAX_THREADS]; \
  extern struct ctf_internal_mock ctf_internal_mock_st_##name;            \
  ret_type __real_##name typed;                                           \
  ret_type __wrap_##name typed;
#define CTF_MOCK_VOID_RET_EXTERN(name, typed)                  \
  extern struct ctf_internal_mock ctf_internal_mock_st_##name; \
  void __real_##name typed;                                    \
  void __wrap_##name typed;
#define CTF_MOCK_BIND(fn, mock) \
  (struct ctf_internal_mock_bind) { &ctf_internal_mock_st_##fn, mock }
#define CTF_MOCK_GROUP(name)    \
  struct ctf_internal_mock_bind \
    ctf_internal_mock_group_st_##name[CTF_CONST_MOCK_GROUP_SIZE]
#define CTF_MOCK_GROUP_EXTERN(name)    \
  extern struct ctf_internal_mock_bind \
    ctf_internal_mock_group_st_##name[CTF_CONST_MOCK_GROUP_SIZE];
#define ctf_internal_mock_reset(state) \
  do {                                 \
    state->call_count = 0;             \
    state->return_override = 0;        \
    state->check_count = 0;            \
  } while(0)
#define ctf_mock_group(name)                                                  \
  do {                                                                        \
    struct ctf_internal_mock_state *state;                                    \
    for(uintmax_t _i = 0; _i < CTF_CONST_MOCK_GROUP_SIZE &&                   \
                          ctf_internal_mock_group_st_##name[_i].f;            \
        _i++) {                                                               \
      state = ctf_internal_mock_group_st_##name[_i].mock->state +             \
              ctf_internal_parallel_thread_index;                             \
      ctf_internal_mock_reset_queue[ctf_internal_mock_reset_count++] = state; \
      state->mock_f = ctf_internal_mock_group_st_##name[_i].f;                \
      ctf_internal_mock_reset(state);                                         \
    }                                                                         \
  } while(0)
#define ctf_mock(fn, mock)                                                  \
  do {                                                                      \
    struct ctf_internal_mock_state *const state =                           \
      ctf_internal_mock_st_##fn.state + ctf_internal_parallel_thread_index; \
    state->mock_f = mock;                                                   \
    ctf_internal_mock_reset_queue[ctf_internal_mock_reset_count++] = state; \
    ctf_internal_mock_reset(state);                                         \
  } while(0)
#define ctf_unmock(fn)                                                      \
  do {                                                                      \
    struct ctf_internal_mock_state *const state =                           \
      ctf_internal_mock_st_##fn.state + ctf_internal_parallel_thread_index; \
    state->mock_f = NULL;                                                   \
    ctf_internal_mock_reset(state);                                         \
  } while(0)
#define ctf_unmock_group(name)                                     \
  do {                                                             \
    struct ctf_internal_mock_state *state;                         \
    for(uintmax_t _i = 0; _i < CTF_CONST_MOCK_GROUP_SIZE &&        \
                          ctf_internal_mock_group_st_##name[_i].f; \
        _i++) {                                                    \
      state = ctf_internal_mock_group_st_##name[_i].mock->state +  \
              ctf_internal_parallel_thread_index;                  \
      state->mock_f = NULL;                                        \
      ctf_internal_mock_reset(state);                              \
    }                                                              \
  } while(0)
#define ctf_mock_call_count(name)                                  \
  ctf_internal_mock_call_count(ctf_internal_mock_st_##name.state + \
                               ctf_internal_parallel_thread_index)
#define ctf_mock_will_return(name, val)                                        \
  do {                                                                         \
    ctf_internal_mock_return_##name[ctf_internal_parallel_thread_index] = val; \
    ctf_internal_mock_st_##name.state[ctf_internal_parallel_thread_index]      \
      .return_override = 1;                                                    \
  } while(0)
#define ctf_mock_will_return_once(name, val)                                   \
  do {                                                                         \
    ctf_internal_mock_return_##name[ctf_internal_parallel_thread_index] = val; \
    ctf_internal_mock_st_##name.state[ctf_internal_parallel_thread_index]      \
      .return_override = 2;                                                    \
  } while(0)
#define ctf_mock_real(name) __real_##name

uintmax_t ctf_internal_pass(const char *, int, const char *,...);
uintmax_t ctf_internal_fail(const char *, int, const char *,...);
uintmax_t ctf_internal_assert_msg(int, const char *, int, const char *,...);
#define ctf_pass(m, ...) ctf_internal_pass(m, __LINE__, __FILE__, ##__VA_ARGS__)
#define ctf_fail(m, ...) ctf_internal_fail(m, __LINE__, __FILE__, ##__VA_ARGS__)
#define ctf_assert_true(a) ctf_assert_int_neq(a, 0);
#define ctf_assert_false(a) ctf_assert_int_eq(a, 0);
#define ctf_expect_true(a) ctf_expect_int_neq(a, 0);
#define ctf_expect_false(a) ctf_expect_int_eq(a, 0);
#define ctf_assert_non_null(a) ctf_assert_ptr_neq(a, NULL);
#define ctf_assert_null(a) ctf_assert_ptr_eq(a, NULL);
#define ctf_expect_non_null(a) ctf_expect_ptr_neq(a, NULL);
#define ctf_expect_null(a) ctf_expect_ptr_eq(a, NULL);
#define ctf_assert_msg(cmp, m, ...)                                    \
  CTF_INTERNAL_EA(ctf_internal_assert_msg, cmp, m, __LINE__, __FILE__, \
                  ##__VA_ARGS__)
#define ctf_assert(cmp)        \
  do {                         \
    ctf_assert_msg(cmp, #cmp); \
    ctf_assert_hide(1);        \
  } while(0)

// clang-format off
/*
define(`MOCK_FUNCTION_HELPER', `void ctf_internal_mock_$1(struct ctf_internal_mock*, int, int, const char *, const char*, void *, const char*, $2 val);')
define(`MOCK_FUNCTION', `MOCK_FUNCTION_HELPER(`$1',TYPE(`$1'))')
define(`MOCK_CHECK_FUNCTION_HELPER', `void ctf_internal_mock_check_$1(struct ctf_internal_mock_state *state, $2 v, const char *v_print);')
define(`MOCK_CHECK_MEMORY_FUNCTION_HELPER', `void ctf_internal_mock_check_memory_$1(struct ctf_internal_mock_state *state, const void *v, const char *v_print, uintmax_t step, int sign);')
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
define(`MOCK_CHECK', `#define ctf_mock_check_$1(name, v) ctf_internal_mock_check_$1(ctf_internal_mock_st_##name.state + ctf_internal_parallel_thread_index, v, #v)')
define(`MOCK_CHECK_STRING', `#define ctf_mock_check_str(name, v) \
ctf_internal_mock_check_ptr(ctf_internal_mock_st_##name.state + ctf_internal_parallel_thread_index, v, #v); \
ctf_internal_mock_check_str(ctf_internal_mock_st_##name.state + ctf_internal_parallel_thread_index, v, #v)')
define(`MOCK_CHECK_MEMORY',
`format(`#define ctf_mock_check_memory_$1(name, v) \
ctf_internal_mock_check_ptr(ctf_internal_mock_st_##name.state + ctf_internal_parallel_thread_index, v, #v); \
    ctf_internal_mock_check_memory_$1(ctf_internal_mock_st_##name.state + ctf_internal_parallel_thread_index, v, #v, sizeof(*(v)), %d)'
  ,ifelse(`$1',`int',1,0))')
define(`EA_TEMPLATE', `#define ctf_$3_$1_$2(a, b) CTF_INTERNAL_EA(ctf_internal_$3_$1_$2, a, b, #a, #b, __LINE__, __FILE__)')dnl
define(`EA_MEMORY_TEMPLATE',
`format(`#define ctf_$3_memory_$1_$2(a, b, length) CTF_INTERNAL_EA(ctf_internal_$3_memory_$1_$2,(const void *)a, (const void *)b, length,  sizeof(*(a)), %d, #a, #b, __LINE__, __FILE__)', ifelse(`$1', `int', `1', `0'))')dnl
define(`EA_ARRAY_TEMPLATE',
`format(`#define ctf_$3_array_$1_$2(a, b) CTF_INTERNAL_EA(ctf_internal_$3_array_$1_$2,(const void *const *)a, (const void *const *)b, sizeof(a)/sizeof(*(a)), sizeof(b)/sizeof(*(b)),  sizeof(*(a)), %d, #a, #b, __LINE__, __FILE__)', ifelse(`$1', `int', `1', `0'))')dnl
define(`EA_FUNCTION',
`format(`int ctf_internal_$3_$1_$2(%s, %s, const char *, const char *, int, const char *);',TYPE(`$1'),TYPE(`$1'))')dnl
define(`EA_MEMORY_PRIMITIVE_FUNCTION',
`int ctf_internal_$3_memory_$1_$2(const void *, const void *, uintmax_t, uintmax_t, int, const char *, const char *, int, const char *);')dnl
define(`EA_ARRAY_PRIMITIVE_FUNCTION',
`int ctf_internal_$3_array_$1_$2(const void *, const void *, uintmax_t, uintmax_t, uintmax_t, int, const char *, const char *, int, const char*);')dnl
define(`ASSERT_WRAP',
`indir(`$1', `$2', `$3', `assert')
indir(`$1', `$2', `$3', `expect')
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
EA_FACTORY(`(PRIMITIVE_TYPES, str)', `(CMPS)', `EA_FUNCTION')
EA_FACTORY(`(PRIMITIVE_TYPES)', `(CMPS)', `EA_MEMORY_PRIMITIVE_FUNCTION')
EA_FACTORY(`(PRIMITIVE_TYPES)', `(CMPS)', `EA_ARRAY_PRIMITIVE_FUNCTION')
EA_FACTORY(`(PRIMITIVE_TYPES, str)', `(CMPS)', `EA_TEMPLATE')
EA_FACTORY(`(PRIMITIVE_TYPES)', `(CMPS)', `EA_MEMORY_TEMPLATE')
EA_FACTORY(`(PRIMITIVE_TYPES)', `(CMPS)', `EA_ARRAY_TEMPLATE')
COMB(`MOCK_CHECK_FUNCTION', `(PRIMITIVE_TYPES, str)')
COMB(`MOCK_CHECK_MEMORY_FUNCTION', `(PRIMITIVE_TYPES, str)')
COMB(`MOCK_CHECK', `(PRIMITIVE_TYPES)')
MOCK_CHECK_STRING
COMB(`MOCK_CHECK_MEMORY', `(PRIMITIVE_TYPES)')
// clang-format on
void ctf_main(int argc, char *argv[]);
void ctf_internal_mock_memory(struct ctf_internal_mock *mock, int type,
                              int, const char *, const char*, void *f,
                              const char *var, const void *val, uintmax_t l);

#if CTF_ALIASES == CTF_ON
// clang-format off
/*
define(`ALIAS', `#define $1 ctf_$1
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
*/
COMB(`ALIAS',
     `(mock(name, f), unmock(f), mock_group(name), unmock_group(name),
       mock_call_count(name), mock_real(name),
       mock_will_return(name, val), mock_will_return_once(name, val),
       assert_barrier(), assert_fold(count, msg),
       assert_true(a), assert_false(a), expect_true(a), expect_false(a),
       assert_null(a), assert_non_null(a), expect_null(a), expect_non_null(a))')
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
