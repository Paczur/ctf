#ifndef CTF_H
#define CTF_H

#ifndef CTF_ASSERT_ALIASES
#define CTF_ASSERT_ALIASES CTF_ON
#endif

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#ifdef CTF_PARALLEL
#include <pthread.h>
#endif

#define CTF_CONST_STATE_FILE_SIZE 256
#define CTF_CONST_STATE_MSG_SIZE 4096
#define CTF_CONST_STATES_PER_THREAD 64

#ifdef CTF_PARALLEL
#if __STDC_VERSION__ == 201112L
#define CTF_PARALLEL_INTERNAL_THREAD_LOCAL _Thread_local
#else
#define CTF_PARALLEL_INTERNAL_THREAD_LOCAL __thread
#endif
#endif

#define CTF_INTERNAL_STRINGIFY(x) #x
#define CTF_INTERNAL_STRINGIFY2(x) CTF_INTERNAL_STRINGIFY(x)
#define CTF_INTERNAL_EXPECT(f, ...)                                     \
  do {                                                                  \
    if(ctf_internal_state_index == CTF_CONST_STATES_PER_THREAD) return; \
    f(__VA_ARGS__);                                                     \
  } while(0)
#define CTF_INTERNAL_ASSERT(f, ...)                                      \
  do {                                                                   \
    if(ctf_internal_state_index == CTF_CONST_STATES_PER_THREAD) return;  \
    f(__VA_ARGS__);                                                      \
    if(ctf_internal_states[ctf_internal_state_index - 1].status) return; \
  } while(0)

struct ctf_internal_state {
  int status;
  int line;
  char file[CTF_CONST_STATE_FILE_SIZE];
  char msg[CTF_CONST_STATE_MSG_SIZE];
};
typedef void (*const ctf_internal_test)(void);
struct ctf_internal_group {
  const ctf_internal_test *tests;
  const char *test_names;
  const int length;
  const char *name;
};

extern int ctf_exit_code;
#ifdef CTF_PARALLEL
extern CTF_PARALLEL_INTERNAL_THREAD_LOCAL struct ctf_internal_state
  *ctf_internal_states;
extern CTF_PARALLEL_INTERNAL_THREAD_LOCAL int ctf_internal_state_index;
#else
extern struct ctf_internal_state
  ctf_internal_states[CTF_CONST_STATES_PER_THREAD];
extern int ctf_internal_state_index;
#endif

void ctf_internal_group_run(const struct ctf_internal_group *group);
void ctf_internal_groups_run(int count, ...);
size_t ctf_internal_assert_mem_print(struct ctf_internal_state *state,
                                     const void *a, const void *b, size_t la,
                                     size_t lb, size_t step, int signed,
                                     const char *a_str, const char *b_str,
                                     const char *op_str, const char *format,
                                     int line, const char *file);
void ctf_internal_assert_copy(struct ctf_internal_state *state, int line,
                              const char *file);

#ifdef CTF_PARALLEL
void ctf_parallel_internal_group_run(const struct ctf_internal_group *group);
void ctf_parallel_internal_groups_run(int count, ...);
#endif
int ctf_internal_fail(const char *, int, const char *);
int ctf_internal_pass(const char *, int, const char *);
int ctf_internal_expect_msg(int, const char *, int, const char *);
int ctf_internal_expect_true(int, const char *, int, const char *);
#define EXPECT_GEN_PRIMITIVE       \
  EXPECT_GEN(char_eq, char);       \
  EXPECT_GEN(char_neq, char);      \
  EXPECT_GEN(char_gt, char);       \
  EXPECT_GEN(char_lt, char);       \
  EXPECT_GEN(char_gte, char);      \
  EXPECT_GEN(char_lte, char);      \
  EXPECT_GEN(int_eq, intmax_t);    \
  EXPECT_GEN(int_neq, intmax_t);   \
  EXPECT_GEN(int_gt, intmax_t);    \
  EXPECT_GEN(int_lt, intmax_t);    \
  EXPECT_GEN(int_gte, intmax_t);   \
  EXPECT_GEN(int_lte, intmax_t);   \
  EXPECT_GEN(uint_eq, uintmax_t);  \
  EXPECT_GEN(uint_neq, uintmax_t); \
  EXPECT_GEN(uint_gt, uintmax_t);  \
  EXPECT_GEN(uint_lt, uintmax_t);  \
  EXPECT_GEN(uint_gte, uintmax_t); \
  EXPECT_GEN(uint_lte, uintmax_t);

#define EXPECT_GEN(name, type)                                                \
  int ctf_internal_expect_##name(type, type, const char *, const char *, int, \
                                 const char *)
EXPECT_GEN_PRIMITIVE
EXPECT_GEN(ptr_eq, const void *);
EXPECT_GEN(ptr_neq, const void *);
EXPECT_GEN(ptr_gt, const void *);
EXPECT_GEN(ptr_lt, const void *);
EXPECT_GEN(ptr_gte, const void *);
EXPECT_GEN(ptr_lte, const void *);
EXPECT_GEN(string_eq, const char *);
EXPECT_GEN(string_neq, const char *);
EXPECT_GEN(string_gt, const char *);
EXPECT_GEN(string_lt, const char *);
EXPECT_GEN(string_gte, const char *);
EXPECT_GEN(string_lte, const char *);
#undef EXPECT_GEN
#define EXPECT_GEN(name, type)                                              \
  int ctf_internal_expect_memory_##name(const void *, const void *, size_t, \
                                        size_t, int, const char *,          \
                                        const char *, int, const char *)
EXPECT_GEN_PRIMITIVE
#undef EXPECT_GEN
#define EXPECT_GEN(name, type)                                     \
  int ctf_internal_expect_memory_##name(                           \
    const void *const *, const void *const *, size_t, size_t, int, \
    const char *, const char *, int, const char *)
EXPECT_GEN(ptr_eq, const void *);
EXPECT_GEN(ptr_neq, const void *);
EXPECT_GEN(ptr_gt, const void *);
EXPECT_GEN(ptr_lt, const void *);
EXPECT_GEN(ptr_gte, const void *);
EXPECT_GEN(ptr_lte, const void *);
#undef EXPECT_GEN
#define EXPECT_GEN(name, type)                                             \
  int ctf_internal_expect_array_##name(const void *, const void *, size_t, \
                                       size_t, size_t, int, const char *,  \
                                       const char *, int, const char *)
EXPECT_GEN_PRIMITIVE
#undef EXPECT_GEN
#define EXPECT_GEN(name, type)                                             \
  int ctf_internal_expect_array_##name(                                    \
    const void *const *, const void *const *, size_t, size_t, size_t, int, \
    const char *, const char *, int, const char *)
EXPECT_GEN(ptr_eq, const void *);
EXPECT_GEN(ptr_neq, const void *);
EXPECT_GEN(ptr_gt, const void *);
EXPECT_GEN(ptr_lt, const void *);
EXPECT_GEN(ptr_gte, const void *);
EXPECT_GEN(ptr_lte, const void *);
#undef EXPECT_GEN
#undef EXPECT_GEN_PRIMITIVE

#define CTF_EXTERN_TEST(name) void name(void)
#define CTF_TEST(name) void name(void)
#define CTF_EXTERN_GROUP(group_name) \
  extern const struct ctf_internal_group group_name
#define CTF_GROUP(group_name, ...)                           \
  const struct ctf_internal_group group_name =               \
    (const struct ctf_internal_group){                       \
      .tests = (ctf_internal_test[]){__VA_ARGS__},           \
      .test_names = CTF_INTERNAL_STRINGIFY2((__VA_ARGS__)),  \
      .length = sizeof((ctf_internal_test[]){__VA_ARGS__}) / \
                sizeof(ctf_internal_test),                   \
      .name = #group_name,                                   \
    };

#ifndef CTF_PARALLEL
#define ctf_group_run(name) ctf_internal_group_run(name)
#define ctf_groups_run(...)                                            \
  ctf_internal_groups_run(                                             \
    (sizeof((const struct ctf_internal_group *const[]){__VA_ARGS__}) / \
     sizeof(struct ctf_internal_group *)),                             \
    __VA_ARGS__)
#define ctf_barrier()       \
  do {                      \
    if(ctf_exit_code) {     \
      return ctf_exit_code; \
    }                       \
  } while(0)
#define ctf_parallel_start()
#define ctf_parallel_stop()
#define ctf_parallel_sync()
#else
void ctf_parallel_start(void);
void ctf_parallel_stop(void);
void ctf_parallel_sync(void);
#define ctf_group_run(name)                  \
  do {                                       \
    if(ctf_parallel_internal_state) {        \
      ctf_parallel_internal_group_run(name); \
    } else {                                 \
      ctf_internal_group_run(name);          \
    }                                        \
  } while(0)
#define ctf_groups_run(...)                                                \
  do {                                                                     \
    if(ctf_parallel_internal_state) {                                      \
      ctf_parallel_internal_groups_run(                                    \
        (sizeof((const struct ctf_internal_group *const[]){__VA_ARGS__}) / \
         sizeof(struct ctf_internal_group *)),                             \
        __VA_ARGS__);                                                      \
    } else {                                                               \
      ctf_internal_groups_run(                                             \
        (sizeof((const struct ctf_internal_group *const[]){__VA_ARGS__}) / \
         sizeof(struct ctf_internal_group *)),                             \
        __VA_ARGS__);                                                      \
    }                                                                      \
  } while(0)
#define ctf_barrier()       \
  do {                      \
    ctf_parallel_sync();    \
    if(ctf_exit_code) {     \
      ctf_parallel_stop();  \
      return ctf_exit_code; \
    }                       \
  } while(0)
#endif

#define ctf_fail(m)                                                \
  do {                                                             \
    CTF_INTERNAL_EXPECT(ctf_internal_fail, m, __LINE__, __FILE__); \
    return;                                                        \
  } while(0)
#define ctf_pass(m) \
  CTF_INTERNAL_EXPECT(ctf_internal_pass, m, __LINE__, __FILE__)
#define ctf_assert_msg(test, m) \
  CTF_INTERNAL_ASSERT(ctf_internal_expect_msg, test, m, __LINE__, __FILE__)
#define ctf_assert(test) ctf_assert_msg(test, CTF_INTERNAL_STRINGIFY2(test))
#define ctf_assert_char_eq(a, b)                                           \
  CTF_INTERNAL_ASSERT(ctf_internal_expect_char_eq, a, b, #a, #b, __LINE__, \
                      __FILE__)
#define ctf_assert_char_neq(a, b)                                           \
  CTF_INTERNAL_ASSERT(ctf_internal_expect_char_neq, a, b, #a, #b, __LINE__, \
                      __FILE__)
#define ctf_assert_char_gt(a, b)                                           \
  CTF_INTERNAL_ASSERT(ctf_internal_expect_char_gt, a, b, #a, #b, __LINE__, \
                      __FILE__)
#define ctf_assert_char_lt(a, b)                                           \
  CTF_INTERNAL_ASSERT(ctf_internal_expect_char_lt, a, b, #a, #b, __LINE__, \
                      __FILE__)
#define ctf_assert_char_gte(a, b)                                           \
  CTF_INTERNAL_ASSERT(ctf_internal_expect_char_gte, a, b, #a, #b, __LINE__, \
                      __FILE__)
#define ctf_assert_char_lte(a, b)                                           \
  CTF_INTERNAL_ASSERT(ctf_internal_expect_char_lte, a, b, #a, #b, __LINE__, \
                      __FILE__)
#define ctf_assert_int_eq(a, b)                                           \
  CTF_INTERNAL_ASSERT(ctf_internal_expect_int_eq, a, b, #a, #b, __LINE__, \
                      __FILE__)
#define ctf_assert_int_neq(a, b)                                           \
  CTF_INTERNAL_ASSERT(ctf_internal_expect_int_neq, a, b, #a, #b, __LINE__, \
                      __FILE__)
#define ctf_assert_int_gt(a, b)                                           \
  CTF_INTERNAL_ASSERT(ctf_internal_expect_int_gt, a, b, #a, #b, __LINE__, \
                      __FILE__)
#define ctf_assert_int_lt(a, b)                                           \
  CTF_INTERNAL_ASSERT(ctf_internal_expect_int_lt, a, b, #a, #b, __LINE__, \
                      __FILE__)
#define ctf_assert_int_gte(a, b)                                           \
  CTF_INTERNAL_ASSERT(ctf_internal_expect_int_gte, a, b, #a, #b, __LINE__, \
                      __FILE__)
#define ctf_assert_int_lte(a, b)                                           \
  CTF_INTERNAL_ASSERT(ctf_internal_expect_int_lte, a, b, #a, #b, __LINE__, \
                      __FILE__)
#define ctf_assert_uint_eq(a, b)                                           \
  CTF_INTERNAL_ASSERT(ctf_internal_expect_uint_eq, a, b, #a, #b, __LINE__, \
                      __FILE__)
#define ctf_assert_uint_neq(a, b)                                           \
  CTF_INTERNAL_ASSERT(ctf_internal_expect_uint_neq, a, b, #a, #b, __LINE__, \
                      __FILE__)
#define ctf_assert_uint_gt(a, b)                                           \
  CTF_INTERNAL_ASSERT(ctf_internal_expect_uint_gt, a, b, #a, #b, __LINE__, \
                      __FILE__)
#define ctf_assert_uint_lt(a, b)                                           \
  CTF_INTERNAL_ASSERT(ctf_internal_expect_uint_lt, a, b, #a, #b, __LINE__, \
                      __FILE__)
#define ctf_assert_uint_gte(a, b)                                           \
  CTF_INTERNAL_ASSERT(ctf_internal_expect_uint_gte, a, b, #a, #b, __LINE__, \
                      __FILE__)
#define ctf_assert_uint_lte(a, b)                                           \
  CTF_INTERNAL_ASSERT(ctf_internal_expect_uint_lte, a, b, #a, #b, __LINE__, \
                      __FILE__)
#define ctf_assert_ptr_eq(a, b)                                           \
  CTF_INTERNAL_ASSERT(ctf_internal_expect_ptr_eq, a, b, #a, #b, __LINE__, \
                      __FILE__)
#define ctf_assert_ptr_neq(a, b)                                           \
  CTF_INTERNAL_ASSERT(ctf_internal_expect_ptr_neq, a, b, #a, #b, __LINE__, \
                      __FILE__)
#define ctf_assert_ptr_gt(a, b)                                           \
  CTF_INTERNAL_ASSERT(ctf_internal_expect_ptr_gt, a, b, #a, #b, __LINE__, \
                      __FILE__)
#define ctf_assert_ptr_lt(a, b)                                           \
  CTF_INTERNAL_ASSERT(ctf_internal_expect_ptr_lt, a, b, #a, #b, __LINE__, \
                      __FILE__)
#define ctf_assert_ptr_gte(a, b)                                           \
  CTF_INTERNAL_ASSERT(ctf_internal_expect_ptr_gte, a, b, #a, #b, __LINE__, \
                      __FILE__)
#define ctf_assert_ptr_lte(a, b)                                           \
  CTF_INTERNAL_ASSERT(ctf_internal_expect_ptr_lte, a, b, #a, #b, __LINE__, \
                      __FILE__)
#define ctf_assert_string_eq(a, b)                                           \
  CTF_INTERNAL_ASSERT(ctf_internal_expect_string_eq, a, b, #a, #b, __LINE__, \
                      __FILE__)
#define ctf_assert_string_neq(a, b)                                           \
  CTF_INTERNAL_ASSERT(ctf_internal_expect_string_neq, a, b, #a, #b, __LINE__, \
                      __FILE__)
#define ctf_assert_string_gt(a, b)                                           \
  CTF_INTERNAL_ASSERT(ctf_internal_expect_string_gt, a, b, #a, #b, __LINE__, \
                      __FILE__)
#define ctf_assert_string_lt(a, b)                                           \
  CTF_INTERNAL_ASSERT(ctf_internal_expect_string_lt, a, b, #a, #b, __LINE__, \
                      __FILE__)
#define ctf_assert_string_gte(a, b)                                           \
  CTF_INTERNAL_ASSERT(ctf_internal_expect_string_gte, a, b, #a, #b, __LINE__, \
                      __FILE__)
#define ctf_assert_string_lte(a, b)                                           \
  CTF_INTERNAL_ASSERT(ctf_internal_expect_string_lte, a, b, #a, #b, __LINE__, \
                      __FILE__)
#define ctf_assert_memory_char_eq(a, b, length)                               \
  CTF_INTERNAL_ASSERT(ctf_internal_expect_memory_char_eq,                     \
                      (const void *const *)a, (const void *const *)b, length, \
                      sizeof(*(a)), 0, #a, #b, __LINE__, __FILE__);
#define ctf_assert_memory_char_neq(a, b, length)                              \
  CTF_INTERNAL_ASSERT(ctf_internal_expect_memory_char_neq,                    \
                      (const void *const *)a, (const void *const *)b, length, \
                      sizeof(*(a)), 0, #a, #b, __LINE__, __FILE__);
#define ctf_assert_memory_char_gt(a, b, length)                               \
  CTF_INTERNAL_ASSERT(ctf_internal_expect_memory_char_gt,                     \
                      (const void *const *)a, (const void *const *)b, length, \
                      sizeof(*(a)), 0, #a, #b, __LINE__, __FILE__);
#define ctf_assert_memory_char_lt(a, b, length)                               \
  CTF_INTERNAL_ASSERT(ctf_internal_expect_memory_char_lt,                     \
                      (const void *const *)a, (const void *const *)b, length, \
                      sizeof(*(a)), 0, #a, #b, __LINE__, __FILE__);
#define ctf_assert_memory_char_gte(a, b, length)                              \
  CTF_INTERNAL_ASSERT(ctf_internal_expect_memory_char_gte,                    \
                      (const void *const *)a, (const void *const *)b, length, \
                      sizeof(*(a)), 0, #a, #b, __LINE__, __FILE__);
#define ctf_assert_memory_char_lte(a, b, length)                              \
  CTF_INTERNAL_ASSERT(ctf_internal_expect_memory_char_lte,                    \
                      (const void *const *)a, (const void *const *)b, length, \
                      sizeof(*(a)), 0, #a, #b, __LINE__, __FILE__);
#define ctf_assert_memory_int_eq(a, b, length)                                \
  CTF_INTERNAL_ASSERT(ctf_internal_expect_memory_int_eq,                      \
                      (const void *const *)a, (const void *const *)b, length, \
                      sizeof(*(a)), 1, #a, #b, __LINE__, __FILE__);
#define ctf_assert_memory_int_neq(a, b, length)                               \
  CTF_INTERNAL_ASSERT(ctf_internal_expect_memory_int_neq,                     \
                      (const void *const *)a, (const void *const *)b, length, \
                      sizeof(*(a)), 1, #a, #b, __LINE__, __FILE__);
#define ctf_assert_memory_int_gt(a, b, length)                                \
  CTF_INTERNAL_ASSERT(ctf_internal_expect_memory_int_gt,                      \
                      (const void *const *)a, (const void *const *)b, length, \
                      sizeof(*(a)), 1, #a, #b, __LINE__, __FILE__);
#define ctf_assert_memory_int_lt(a, b, length)                                \
  CTF_INTERNAL_ASSERT(ctf_internal_expect_memory_int_lt,                      \
                      (const void *const *)a, (const void *const *)b, length, \
                      sizeof(*(a)), 1, #a, #b, __LINE__, __FILE__);
#define ctf_assert_memory_int_gte(a, b, length)                               \
  CTF_INTERNAL_ASSERT(ctf_internal_expect_memory_int_gte,                     \
                      (const void *const *)a, (const void *const *)b, length, \
                      sizeof(*(a)), 1, #a, #b, __LINE__, __FILE__);
#define ctf_assert_memory_int_lte(a, b, length)                               \
  CTF_INTERNAL_ASSERT(ctf_internal_expect_memory_int_lte,                     \
                      (const void *const *)a, (const void *const *)b, length, \
                      sizeof(*(a)), 1, #a, #b, __LINE__, __FILE__);
#define ctf_assert_memory_uint_eq(a, b, length)                               \
  CTF_INTERNAL_ASSERT(ctf_internal_expect_memory_uint_eq,                     \
                      (const void *const *)a, (const void *const *)b, length, \
                      sizeof(*(a)), 0, #a, #b, __LINE__, __FILE__);
#define ctf_assert_memory_uint_neq(a, b, length)                              \
  CTF_INTERNAL_ASSERT(ctf_internal_expect_memory_uint_neq,                    \
                      (const void *const *)a, (const void *const *)b, length, \
                      sizeof(*(a)), 0, #a, #b, __LINE__, __FILE__);
#define ctf_assert_memory_uint_gt(a, b, length)                               \
  CTF_INTERNAL_ASSERT(ctf_internal_expect_memory_uint_gt,                     \
                      (const void *const *)a, (const void *const *)b, length, \
                      sizeof(*(a)), 0, #a, #b, __LINE__, __FILE__);
#define ctf_assert_memory_uint_lt(a, b, length)                               \
  CTF_INTERNAL_ASSERT(ctf_internal_expect_memory_uint_lt,                     \
                      (const void *const *)a, (const void *const *)b, length, \
                      sizeof(*(a)), 0, #a, #b, __LINE__, __FILE__);
#define ctf_assert_memory_uint_gte(a, b, length)                              \
  CTF_INTERNAL_ASSERT(ctf_internal_expect_memory_uint_gte,                    \
                      (const void *const *)a, (const void *const *)b, length, \
                      sizeof(*(a)), 0, #a, #b, __LINE__, __FILE__);
#define ctf_assert_memory_uint_lte(a, b, length)                              \
  CTF_INTERNAL_ASSERT(ctf_internal_expect_memory_uint_lte,                    \
                      (const void *const *)a, (const void *const *)b, length, \
                      sizeof(*(a)), 0, #a, #b, __LINE__, __FILE__);
#define ctf_assert_memory_ptr_eq(a, b, length)                                \
  CTF_INTERNAL_ASSERT(ctf_internal_expect_memory_ptr_eq,                      \
                      (const void *const *)a, (const void *const *)b, length, \
                      sizeof(*(a)), 0, #a, #b, __LINE__, __FILE__);
#define ctf_assert_memory_ptr_neq(a, b, length)                               \
  CTF_INTERNAL_ASSERT(ctf_internal_expect_memory_ptr_neq,                     \
                      (const void *const *)a, (const void *const *)b, length, \
                      sizeof(*(a)), 0, #a, #b, __LINE__, __FILE__);
#define ctf_assert_memory_ptr_gt(a, b, length)                                \
  CTF_INTERNAL_ASSERT(ctf_internal_expect_memory_ptr_gt,                      \
                      (const void *const *)a, (const void *const *)b, length, \
                      sizeof(*(a)), 0, #a, #b, __LINE__, __FILE__);
#define ctf_assert_memory_ptr_lt(a, b, length)                                \
  CTF_INTERNAL_ASSERT(ctf_internal_expect_memory_ptr_lt,                      \
                      (const void *const *)a, (const void *const *)b, length, \
                      sizeof(*(a)), 0, #a, #b, __LINE__, __FILE__);
#define ctf_assert_memory_ptr_gte(a, b, length)                               \
  CTF_INTERNAL_ASSERT(ctf_internal_expect_memory_ptr_gte,                     \
                      (const void *const *)a, (const void *const *)b, length, \
                      sizeof(*(a)), 0, #a, #b, __LINE__, __FILE__);
#define ctf_assert_memory_ptr_lte(a, b, length)                               \
  CTF_INTERNAL_ASSERT(ctf_internal_expect_memory_ptr_lte,                     \
                      (const void *const *)a, (const void *const *)b, length, \
                      sizeof(*(a)), 0, #a, #b, __LINE__, __FILE__);
#define ctf_assert_array_char_eq(a, b)                                    \
  CTF_INTERNAL_ASSERT(ctf_internal_expect_array_char_eq,                  \
                      (const void *const *)a, (const void *const *)b,     \
                      sizeof(a) / sizeof(*(a)), sizeof(b) / sizeof(*(b)), \
                      sizeof(*(a)), 0, #a, #b, __LINE__, __FILE__);
#define ctf_assert_array_char_neq(a, b)                                   \
  CTF_INTERNAL_ASSERT(ctf_internal_expect_array_char_neq,                 \
                      (const void *const *)a, (const void *const *)b,     \
                      sizeof(a) / sizeof(*(a)), sizeof(b) / sizeof(*(b)), \
                      sizeof(*(a)), 0, #a, #b, __LINE__, __FILE__)
#define ctf_assert_array_char_gt(a, b)                                    \
  CTF_INTERNAL_ASSERT(ctf_internal_expect_array_char_gt,                  \
                      (const void *const *)a, (const void *const *)b,     \
                      sizeof(a) / sizeof(*(a)), sizeof(b) / sizeof(*(b)), \
                      sizeof(*(a)), 0, #a, #b, __LINE__, __FILE__);
#define ctf_assert_array_char_lt(a, b)                                    \
  CTF_INTERNAL_ASSERT(ctf_internal_expect_array_char_lt,                  \
                      (const void *const *)a, (const void *const *)b,     \
                      sizeof(a) / sizeof(*(a)), sizeof(b) / sizeof(*(b)), \
                      sizeof(*(a)), 0, #a, #b, __LINE__, __FILE__);
#define ctf_assert_array_char_gte(a, b)                                   \
  CTF_INTERNAL_ASSERT(ctf_internal_expect_array_char_gte,                 \
                      (const void *const *)a, (const void *const *)b,     \
                      sizeof(a) / sizeof(*(a)), sizeof(b) / sizeof(*(b)), \
                      sizeof(*(a)), 0, #a, #b, __LINE__, __FILE__);
#define ctf_assert_array_char_lte(a, b)                                   \
  CTF_INTERNAL_ASSERT(ctf_internal_expect_array_char_lte,                 \
                      (const void *const *)a, (const void *const *)b,     \
                      sizeof(a) / sizeof(*(a)), sizeof(b) / sizeof(*(b)), \
                      sizeof(*(a)), 0, #a, #b, __LINE__, __FILE__);
#define ctf_assert_array_int_eq(a, b)                                     \
  CTF_INTERNAL_ASSERT(ctf_internal_expect_array_int_eq,                   \
                      (const void *const *)a, (const void *const *)b,     \
                      sizeof(a) / sizeof(*(a)), sizeof(b) / sizeof(*(b)), \
                      sizeof(*(a)), 1, #a, #b, __LINE__, __FILE__);
#define ctf_assert_array_int_neq(a, b)                                    \
  CTF_INTERNAL_ASSERT(ctf_internal_expect_array_int_neq,                  \
                      (const void *const *)a, (const void *const *)b,     \
                      sizeof(a) / sizeof(*(a)), sizeof(b) / sizeof(*(b)), \
                      sizeof(*(a)), 1, #a, #b, __LINE__, __FILE__);
#define ctf_assert_array_int_gt(a, b)                                     \
  CTF_INTERNAL_ASSERT(ctf_internal_expect_array_int_gt,                   \
                      (const void *const *)a, (const void *const *)b,     \
                      sizeof(a) / sizeof(*(a)), sizeof(b) / sizeof(*(b)), \
                      sizeof(*(a)), 1, #a, #b, __LINE__, __FILE__);
#define ctf_assert_array_int_lt(a, b)                                     \
  CTF_INTERNAL_ASSERT(ctf_internal_expect_array_int_lt,                   \
                      (const void *const *)a, (const void *const *)b,     \
                      sizeof(a) / sizeof(*(a)), sizeof(b) / sizeof(*(b)), \
                      sizeof(*(a)), 1, #a, #b, __LINE__, __FILE__);
#define ctf_assert_array_int_gte(a, b)                                    \
  CTF_INTERNAL_ASSERT(ctf_internal_expect_array_int_gte,                  \
                      (const void *const *)a, (const void *const *)b,     \
                      sizeof(a) / sizeof(*(a)), sizeof(b) / sizeof(*(b)), \
                      sizeof(*(a)), 1, #a, #b, __LINE__, __FILE__);
#define ctf_assert_array_int_lte(a, b)                                    \
  CTF_INTERNAL_ASSERT(ctf_internal_expect_array_int_lte,                  \
                      (const void *const *)a, (const void *const *)b,     \
                      sizeof(a) / sizeof(*(a)), sizeof(b) / sizeof(*(b)), \
                      sizeof(*(a)), 1, #a, #b, __LINE__, __FILE__);
#define ctf_assert_array_uint_eq(a, b)                                    \
  CTF_INTERNAL_ASSERT(ctf_internal_expect_array_uint_eq,                  \
                      (const void *const *)a, (const void *const *)b,     \
                      sizeof(a) / sizeof(*(a)), sizeof(b) / sizeof(*(b)), \
                      sizeof(*(a)), 0, #a, #b, __LINE__, __FILE__);
#define ctf_assert_array_uint_neq(a, b)                                   \
  CTF_INTERNAL_ASSERT(ctf_internal_expect_array_uint_neq,                 \
                      (const void *const *)a, (const void *const *)b,     \
                      sizeof(a) / sizeof(*(a)), sizeof(b) / sizeof(*(b)), \
                      sizeof(*(a)), 0, #a, #b, __LINE__, __FILE__);
#define ctf_assert_array_uint_gt(a, b)                                    \
  CTF_INTERNAL_ASSERT(ctf_internal_expect_array_uint_gt,                  \
                      (const void *const *)a, (const void *const *)b,     \
                      sizeof(a) / sizeof(*(a)), sizeof(b) / sizeof(*(b)), \
                      sizeof(*(a)), 0, #a, #b, __LINE__, __FILE__);
#define ctf_assert_array_uint_lt(a, b)                                    \
  CTF_INTERNAL_ASSERT(ctf_internal_expect_array_uint_lt,                  \
                      (const void *const *)a, (const void *const *)b,     \
                      sizeof(a) / sizeof(*(a)), sizeof(b) / sizeof(*(b)), \
                      sizeof(*(a)), 0, #a, #b, __LINE__, __FILE__);
#define ctf_assert_array_uint_gte(a, b)                                   \
  CTF_INTERNAL_ASSERT(ctf_internal_expect_array_uint_gte,                 \
                      (const void *const *)a, (const void *const *)b,     \
                      sizeof(a) / sizeof(*(a)), sizeof(b) / sizeof(*(b)), \
                      sizeof(*(a)), 0, #a, #b, __LINE__, __FILE__);
#define ctf_assert_array_uint_lte(a, b)                                   \
  CTF_INTERNAL_ASSERT(ctf_internal_expect_array_uint_lte,                 \
                      (const void *const *)a, (const void *const *)b,     \
                      sizeof(a) / sizeof(*(a)), sizeof(b) / sizeof(*(b)), \
                      sizeof(*(a)), 0, #a, #b, __LINE__, __FILE__);
#define ctf_assert_array_ptr_eq(a, b)                                     \
  CTF_INTERNAL_ASSERT(ctf_internal_expect_array_ptr_eq,                   \
                      (const void *const *)a, (const void *const *)b,     \
                      sizeof(a) / sizeof(*(a)), sizeof(b) / sizeof(*(b)), \
                      sizeof(*(a)), 0, #a, #b, __LINE__, __FILE__);
#define ctf_assert_array_ptr_neq(a, b)                                    \
  CTF_INTERNAL_ASSERT(ctf_internal_expect_array_ptr_neq,                  \
                      (const void *const *)a, (const void *const *)b,     \
                      sizeof(a) / sizeof(*(a)), sizeof(b) / sizeof(*(b)), \
                      sizeof(*(a)), 0, #a, #b, __LINE__, __FILE__);
#define ctf_assert_array_ptr_gt(a, b)                                     \
  CTF_INTERNAL_ASSERT(ctf_internal_expect_array_ptr_gt,                   \
                      (const void *const *)a, (const void *const *)b,     \
                      sizeof(a) / sizeof(*(a)), sizeof(b) / sizeof(*(b)), \
                      sizeof(*(a)), 0, #a, #b, __LINE__, __FILE__);
#define ctf_assert_array_ptr_lt(a, b)                                     \
  CTF_INTERNAL_ASSERT(ctf_internal_expect_array_ptr_lt,                   \
                      (const void *const *)a, (const void *const *)b,     \
                      sizeof(a) / sizeof(*(a)), sizeof(b) / sizeof(*(b)), \
                      sizeof(*(a)), 0, #a, #b, __LINE__, __FILE__);
#define ctf_assert_array_ptr_gte(a, b)                                    \
  CTF_INTERNAL_ASSERT(ctf_internal_expect_array_ptr_gte,                  \
                      (const void *const *)a, (const void *const *)b,     \
                      sizeof(a) / sizeof(*(a)), sizeof(b) / sizeof(*(b)), \
                      sizeof(*(a)), 0, #a, #b, __LINE__, __FILE__);
#define ctf_assert_array_ptr_lte(a, b)                                    \
  CTF_INTERNAL_ASSERT(ctf_internal_expect_array_ptr_lte,                  \
                      (const void *const *)a, (const void *const *)b,     \
                      sizeof(a) / sizeof(*(a)), sizeof(b) / sizeof(*(b)), \
                      sizeof(*(a)), 0, #a, #b, __LINE__, __FILE__);
#define ctf_assert_true(a) \
  CTF_INTERNAL_ASSERT(ctf_internal_expect_true, a, #a, __LINE__, __FILE__)
#define ctf_assert_false(a) \
  CTF_INTERNAL_ASSERT(ctf_internal_expect_false, a, #a, __LINE__, __FILE__)

#define ctf_expect_msg(test, m) \
  CTF_INTERNAL_EXPECT(ctf_internal_expect_msg, test, m, __LINE__, __FILE__)
#define ctf_expect(test) ctf_expect_msg(test, CTF_INTERNAL_STRINGIFY2(test))
#define ctf_expect_char_eq(a, b)                                           \
  CTF_INTERNAL_EXPECT(ctf_internal_expect_char_eq, a, b, #a, #b, __LINE__, \
                      __FILE__)
#define ctf_expect_char_neq(a, b)                                           \
  CTF_INTERNAL_EXPECT(ctf_internal_expect_char_neq, a, b, #a, #b, __LINE__, \
                      __FILE__)
#define ctf_expect_char_gt(a, b)                                           \
  CTF_INTERNAL_EXPECT(ctf_internal_expect_char_gt, a, b, #a, #b, __LINE__, \
                      __FILE__)
#define ctf_expect_char_lt(a, b)                                           \
  CTF_INTERNAL_EXPECT(ctf_internal_expect_char_lt, a, b, #a, #b, __LINE__, \
                      __FILE__)
#define ctf_expect_char_gte(a, b)                                           \
  CTF_INTERNAL_EXPECT(ctf_internal_expect_char_gte, a, b, #a, #b, __LINE__, \
                      __FILE__)
#define ctf_expect_char_lte(a, b)                                           \
  CTF_INTERNAL_EXPECT(ctf_internal_expect_char_lte, a, b, #a, #b, __LINE__, \
                      __FILE__)
#define ctf_expect_int_eq(a, b)                                           \
  CTF_INTERNAL_EXPECT(ctf_internal_expect_int_eq, a, b, #a, #b, __LINE__, \
                      __FILE__)
#define ctf_expect_int_neq(a, b)                                           \
  CTF_INTERNAL_EXPECT(ctf_internal_expect_int_neq, a, b, #a, #b, __LINE__, \
                      __FILE__)
#define ctf_expect_int_gt(a, b)                                           \
  CTF_INTERNAL_EXPECT(ctf_internal_expect_int_gt, a, b, #a, #b, __LINE__, \
                      __FILE__)
#define ctf_expect_int_lt(a, b)                                           \
  CTF_INTERNAL_EXPECT(ctf_internal_expect_int_lt, a, b, #a, #b, __LINE__, \
                      __FILE__)
#define ctf_expect_int_gte(a, b)                                           \
  CTF_INTERNAL_EXPECT(ctf_internal_expect_int_gte, a, b, #a, #b, __LINE__, \
                      __FILE__)
#define ctf_expect_int_lte(a, b)                                           \
  CTF_INTERNAL_EXPECT(ctf_internal_expect_int_lte, a, b, #a, #b, __LINE__, \
                      __FILE__)
#define ctf_expect_uint_eq(a, b)                                           \
  CTF_INTERNAL_EXPECT(ctf_internal_expect_uint_eq, a, b, #a, #b, __LINE__, \
                      __FILE__)
#define ctf_expect_uint_neq(a, b)                                           \
  CTF_INTERNAL_EXPECT(ctf_internal_expect_uint_neq, a, b, #a, #b, __LINE__, \
                      __FILE__)
#define ctf_expect_uint_gt(a, b)                                           \
  CTF_INTERNAL_EXPECT(ctf_internal_expect_uint_gt, a, b, #a, #b, __LINE__, \
                      __FILE__)
#define ctf_expect_uint_lt(a, b)                                           \
  CTF_INTERNAL_EXPECT(ctf_internal_expect_uint_lt, a, b, #a, #b, __LINE__, \
                      __FILE__)
#define ctf_expect_uint_gte(a, b)                                           \
  CTF_INTERNAL_EXPECT(ctf_internal_expect_uint_gte, a, b, #a, #b, __LINE__, \
                      __FILE__)
#define ctf_expect_uint_lte(a, b)                                           \
  CTF_INTERNAL_EXPECT(ctf_internal_expect_uint_lte, a, b, #a, #b, __LINE__, \
                      __FILE__)
#define ctf_expect_ptr_eq(a, b)                                           \
  CTF_INTERNAL_EXPECT(ctf_internal_expect_ptr_eq, a, b, #a, #b, __LINE__, \
                      __FILE__)
#define ctf_expect_ptr_neq(a, b)                                           \
  CTF_INTERNAL_EXPECT(ctf_internal_expect_ptr_neq, a, b, #a, #b, __LINE__, \
                      __FILE__)
#define ctf_expect_ptr_gt(a, b)                                           \
  CTF_INTERNAL_EXPECT(ctf_internal_expect_ptr_gt, a, b, #a, #b, __LINE__, \
                      __FILE__)
#define ctf_expect_ptr_lt(a, b)                                           \
  CTF_INTERNAL_EXPECT(ctf_internal_expect_ptr_lt, a, b, #a, #b, __LINE__, \
                      __FILE__)
#define ctf_expect_ptr_gte(a, b)                                           \
  CTF_INTERNAL_EXPECT(ctf_internal_expect_ptr_gte, a, b, #a, #b, __LINE__, \
                      __FILE__)
#define ctf_expect_ptr_lte(a, b)                                           \
  CTF_INTERNAL_EXPECT(ctf_internal_expect_ptr_lte, a, b, #a, #b, __LINE__, \
                      __FILE__)
#define ctf_expect_string_eq(a, b)                                           \
  CTF_INTERNAL_EXPECT(ctf_internal_expect_string_eq, a, b, #a, #b, __LINE__, \
                      __FILE__)
#define ctf_expect_string_neq(a, b)                                           \
  CTF_INTERNAL_EXPECT(ctf_internal_expect_string_neq, a, b, #a, #b, __LINE__, \
                      __FILE__)
#define ctf_expect_string_gt(a, b)                                           \
  CTF_INTERNAL_EXPECT(ctf_internal_expect_string_gt, a, b, #a, #b, __LINE__, \
                      __FILE__)
#define ctf_expect_string_lt(a, b)                                           \
  CTF_INTERNAL_EXPECT(ctf_internal_expect_string_lt, a, b, #a, #b, __LINE__, \
                      __FILE__)
#define ctf_expect_string_gte(a, b)                                           \
  CTF_INTERNAL_EXPECT(ctf_internal_expect_string_gte, a, b, #a, #b, __LINE__, \
                      __FILE__)
#define ctf_expect_string_lte(a, b)                                           \
  CTF_INTERNAL_EXPECT(ctf_internal_expect_string_lte, a, b, #a, #b, __LINE__, \
                      __FILE__)
#define ctf_expect_memory_char_eq(a, b, length)                               \
  CTF_INTERNAL_EXPECT(ctf_internal_expect_memory_char_eq,                     \
                      (const void *const *)a, (const void *const *)b, length, \
                      sizeof(*(a)), 0, #a, #b, __LINE__, __FILE__);
#define ctf_expect_memory_char_neq(a, b, length)                              \
  CTF_INTERNAL_EXPECT(ctf_internal_expect_memory_char_neq,                    \
                      (const void *const *)a, (const void *const *)b, length, \
                      sizeof(*(a)), 0, #a, #b, __LINE__, __FILE__);
#define ctf_expect_memory_char_gt(a, b, length)                               \
  CTF_INTERNAL_EXPECT(ctf_internal_expect_memory_char_gt,                     \
                      (const void *const *)a, (const void *const *)b, length, \
                      sizeof(*(a)), 0, #a, #b, __LINE__, __FILE__);
#define ctf_expect_memory_char_lt(a, b, length)                               \
  CTF_INTERNAL_EXPECT(ctf_internal_expect_memory_char_lt,                     \
                      (const void *const *)a, (const void *const *)b, length, \
                      sizeof(*(a)), 0, #a, #b, __LINE__, __FILE__);
#define ctf_expect_memory_char_gte(a, b, length)                              \
  CTF_INTERNAL_EXPECT(ctf_internal_expect_memory_char_gte,                    \
                      (const void *const *)a, (const void *const *)b, length, \
                      sizeof(*(a)), 0, #a, #b, __LINE__, __FILE__);
#define ctf_expect_memory_char_lte(a, b, length)                              \
  CTF_INTERNAL_EXPECT(ctf_internal_expect_memory_char_lte,                    \
                      (const void *const *)a, (const void *const *)b, length, \
                      sizeof(*(a)), 0, #a, #b, __LINE__, __FILE__);
#define ctf_expect_memory_int_eq(a, b, length)                                \
  CTF_INTERNAL_EXPECT(ctf_internal_expect_memory_int_eq,                      \
                      (const void *const *)a, (const void *const *)b, length, \
                      sizeof(*(a)), 1, #a, #b, __LINE__, __FILE__);
#define ctf_expect_memory_int_neq(a, b, length)                               \
  CTF_INTERNAL_EXPECT(ctf_internal_expect_memory_int_neq,                     \
                      (const void *const *)a, (const void *const *)b, length, \
                      sizeof(*(a)), 1, #a, #b, __LINE__, __FILE__);
#define ctf_expect_memory_int_gt(a, b, length)                                \
  CTF_INTERNAL_EXPECT(ctf_internal_expect_memory_int_gt,                      \
                      (const void *const *)a, (const void *const *)b, length, \
                      sizeof(*(a)), 1, #a, #b, __LINE__, __FILE__);
#define ctf_expect_memory_int_lt(a, b, length)                                \
  CTF_INTERNAL_EXPECT(ctf_internal_expect_memory_int_lt,                      \
                      (const void *const *)a, (const void *const *)b, length, \
                      sizeof(*(a)), 1, #a, #b, __LINE__, __FILE__);
#define ctf_expect_memory_int_gte(a, b, length)                               \
  CTF_INTERNAL_EXPECT(ctf_internal_expect_memory_int_gte,                     \
                      (const void *const *)a, (const void *const *)b, length, \
                      sizeof(*(a)), 1, #a, #b, __LINE__, __FILE__);
#define ctf_expect_memory_int_lte(a, b, length)                               \
  CTF_INTERNAL_EXPECT(ctf_internal_expect_memory_int_lte,                     \
                      (const void *const *)a, (const void *const *)b, length, \
                      sizeof(*(a)), 1, #a, #b, __LINE__, __FILE__);
#define ctf_expect_memory_uint_eq(a, b, length)                               \
  CTF_INTERNAL_EXPECT(ctf_internal_expect_memory_uint_eq,                     \
                      (const void *const *)a, (const void *const *)b, length, \
                      sizeof(*(a)), 0, #a, #b, __LINE__, __FILE__);
#define ctf_expect_memory_uint_neq(a, b, length)                              \
  CTF_INTERNAL_EXPECT(ctf_internal_expect_memory_uint_neq,                    \
                      (const void *const *)a, (const void *const *)b, length, \
                      sizeof(*(a)), 0, #a, #b, __LINE__, __FILE__);
#define ctf_expect_memory_uint_gt(a, b, length)                               \
  CTF_INTERNAL_EXPECT(ctf_internal_expect_memory_uint_gt,                     \
                      (const void *const *)a, (const void *const *)b, length, \
                      sizeof(*(a)), 0, #a, #b, __LINE__, __FILE__);
#define ctf_expect_memory_uint_lt(a, b, length)                               \
  CTF_INTERNAL_EXPECT(ctf_internal_expect_memory_uint_lt,                     \
                      (const void *const *)a, (const void *const *)b, length, \
                      sizeof(*(a)), 0, #a, #b, __LINE__, __FILE__);
#define ctf_expect_memory_uint_gte(a, b, length)                              \
  CTF_INTERNAL_EXPECT(ctf_internal_expect_memory_uint_gte,                    \
                      (const void *const *)a, (const void *const *)b, length, \
                      sizeof(*(a)), 0, #a, #b, __LINE__, __FILE__);
#define ctf_expect_memory_uint_lte(a, b, length)                              \
  CTF_INTERNAL_EXPECT(ctf_internal_expect_memory_uint_lte,                    \
                      (const void *const *)a, (const void *const *)b, length, \
                      sizeof(*(a)), 0, #a, #b, __LINE__, __FILE__);
#define ctf_expect_memory_ptr_eq(a, b, length)                                \
  CTF_INTERNAL_EXPECT(ctf_internal_expect_memory_ptr_eq,                      \
                      (const void *const *)a, (const void *const *)b, length, \
                      sizeof(*(a)), 0, #a, #b, __LINE__, __FILE__);
#define ctf_expect_memory_ptr_neq(a, b, length)                               \
  CTF_INTERNAL_EXPECT(ctf_internal_expect_memory_ptr_neq,                     \
                      (const void *const *)a, (const void *const *)b, length, \
                      sizeof(*(a)), 0, #a, #b, __LINE__, __FILE__);
#define ctf_expect_memory_ptr_gt(a, b, length)                                \
  CTF_INTERNAL_EXPECT(ctf_internal_expect_memory_ptr_gt,                      \
                      (const void *const *)a, (const void *const *)b, length, \
                      sizeof(*(a)), 0, #a, #b, __LINE__, __FILE__);
#define ctf_expect_memory_ptr_lt(a, b, length)                                \
  CTF_INTERNAL_EXPECT(ctf_internal_expect_memory_ptr_lt,                      \
                      (const void *const *)a, (const void *const *)b, length, \
                      sizeof(*(a)), 0, #a, #b, __LINE__, __FILE__);
#define ctf_expect_memory_ptr_gte(a, b, length)                               \
  CTF_INTERNAL_EXPECT(ctf_internal_expect_memory_ptr_gte,                     \
                      (const void *const *)a, (const void *const *)b, length, \
                      sizeof(*(a)), 0, #a, #b, __LINE__, __FILE__);
#define ctf_expect_memory_ptr_lte(a, b, length)                               \
  CTF_INTERNAL_EXPECT(ctf_internal_expect_memory_ptr_lte,                     \
                      (const void *const *)a, (const void *const *)b, length, \
                      sizeof(*(a)), 0, #a, #b, __LINE__, __FILE__);
#define ctf_expect_array_char_eq(a, b)                                    \
  CTF_INTERNAL_EXPECT(ctf_internal_expect_array_char_eq,                  \
                      (const void *const *)a, (const void *const *)b,     \
                      sizeof(a) / sizeof(*(a)), sizeof(b) / sizeof(*(b)), \
                      sizeof(*(a)), 0, #a, #b, __LINE__, __FILE__);
#define ctf_expect_array_char_neq(a, b)                                   \
  CTF_INTERNAL_EXPECT(ctf_internal_expect_array_char_neq,                 \
                      (const void *const *)a, (const void *const *)b,     \
                      sizeof(a) / sizeof(*(a)), sizeof(b) / sizeof(*(b)), \
                      sizeof(*(a)), 0, #a, #b, __LINE__, __FILE__)
#define ctf_expect_array_char_gt(a, b)                                    \
  CTF_INTERNAL_EXPECT(ctf_internal_expect_array_char_gt,                  \
                      (const void *const *)a, (const void *const *)b,     \
                      sizeof(a) / sizeof(*(a)), sizeof(b) / sizeof(*(b)), \
                      sizeof(*(a)), 0, #a, #b, __LINE__, __FILE__);
#define ctf_expect_array_char_lt(a, b)                                    \
  CTF_INTERNAL_EXPECT(ctf_internal_expect_array_char_lt,                  \
                      (const void *const *)a, (const void *const *)b,     \
                      sizeof(a) / sizeof(*(a)), sizeof(b) / sizeof(*(b)), \
                      sizeof(*(a)), 0, #a, #b, __LINE__, __FILE__);
#define ctf_expect_array_char_gte(a, b)                                   \
  CTF_INTERNAL_EXPECT(ctf_internal_expect_array_char_gte,                 \
                      (const void *const *)a, (const void *const *)b,     \
                      sizeof(a) / sizeof(*(a)), sizeof(b) / sizeof(*(b)), \
                      sizeof(*(a)), 0, #a, #b, __LINE__, __FILE__);
#define ctf_expect_array_char_lte(a, b)                                   \
  CTF_INTERNAL_EXPECT(ctf_internal_expect_array_char_lte,                 \
                      (const void *const *)a, (const void *const *)b,     \
                      sizeof(a) / sizeof(*(a)), sizeof(b) / sizeof(*(b)), \
                      sizeof(*(a)), 0, #a, #b, __LINE__, __FILE__);
#define ctf_expect_array_int_eq(a, b)                                     \
  CTF_INTERNAL_EXPECT(ctf_internal_expect_array_int_eq,                   \
                      (const void *const *)a, (const void *const *)b,     \
                      sizeof(a) / sizeof(*(a)), sizeof(b) / sizeof(*(b)), \
                      sizeof(*(a)), 1, #a, #b, __LINE__, __FILE__);
#define ctf_expect_array_int_neq(a, b)                                    \
  CTF_INTERNAL_EXPECT(ctf_internal_expect_array_int_neq,                  \
                      (const void *const *)a, (const void *const *)b,     \
                      sizeof(a) / sizeof(*(a)), sizeof(b) / sizeof(*(b)), \
                      sizeof(*(a)), 1, #a, #b, __LINE__, __FILE__);
#define ctf_expect_array_int_gt(a, b)                                     \
  CTF_INTERNAL_EXPECT(ctf_internal_expect_array_int_gt,                   \
                      (const void *const *)a, (const void *const *)b,     \
                      sizeof(a) / sizeof(*(a)), sizeof(b) / sizeof(*(b)), \
                      sizeof(*(a)), 1, #a, #b, __LINE__, __FILE__);
#define ctf_expect_array_int_lt(a, b)                                     \
  CTF_INTERNAL_EXPECT(ctf_internal_expect_array_int_lt,                   \
                      (const void *const *)a, (const void *const *)b,     \
                      sizeof(a) / sizeof(*(a)), sizeof(b) / sizeof(*(b)), \
                      sizeof(*(a)), 1, #a, #b, __LINE__, __FILE__);
#define ctf_expect_array_int_gte(a, b)                                    \
  CTF_INTERNAL_EXPECT(ctf_internal_expect_array_int_gte,                  \
                      (const void *const *)a, (const void *const *)b,     \
                      sizeof(a) / sizeof(*(a)), sizeof(b) / sizeof(*(b)), \
                      sizeof(*(a)), 1, #a, #b, __LINE__, __FILE__);
#define ctf_expect_array_int_lte(a, b)                                    \
  CTF_INTERNAL_EXPECT(ctf_internal_expect_array_int_lte,                  \
                      (const void *const *)a, (const void *const *)b,     \
                      sizeof(a) / sizeof(*(a)), sizeof(b) / sizeof(*(b)), \
                      sizeof(*(a)), 1, #a, #b, __LINE__, __FILE__);
#define ctf_expect_array_uint_eq(a, b)                                    \
  CTF_INTERNAL_EXPECT(ctf_internal_expect_array_uint_eq,                  \
                      (const void *const *)a, (const void *const *)b,     \
                      sizeof(a) / sizeof(*(a)), sizeof(b) / sizeof(*(b)), \
                      sizeof(*(a)), 0, #a, #b, __LINE__, __FILE__);
#define ctf_expect_array_uint_neq(a, b)                                   \
  CTF_INTERNAL_EXPECT(ctf_internal_expect_array_uint_neq,                 \
                      (const void *const *)a, (const void *const *)b,     \
                      sizeof(a) / sizeof(*(a)), sizeof(b) / sizeof(*(b)), \
                      sizeof(*(a)), 0, #a, #b, __LINE__, __FILE__);
#define ctf_expect_array_uint_gt(a, b)                                    \
  CTF_INTERNAL_EXPECT(ctf_internal_expect_array_uint_gt,                  \
                      (const void *const *)a, (const void *const *)b,     \
                      sizeof(a) / sizeof(*(a)), sizeof(b) / sizeof(*(b)), \
                      sizeof(*(a)), 0, #a, #b, __LINE__, __FILE__);
#define ctf_expect_array_uint_lt(a, b)                                    \
  CTF_INTERNAL_EXPECT(ctf_internal_expect_array_uint_lt,                  \
                      (const void *const *)a, (const void *const *)b,     \
                      sizeof(a) / sizeof(*(a)), sizeof(b) / sizeof(*(b)), \
                      sizeof(*(a)), 0, #a, #b, __LINE__, __FILE__);
#define ctf_expect_array_uint_gte(a, b)                                   \
  CTF_INTERNAL_EXPECT(ctf_internal_expect_array_uint_gte,                 \
                      (const void *const *)a, (const void *const *)b,     \
                      sizeof(a) / sizeof(*(a)), sizeof(b) / sizeof(*(b)), \
                      sizeof(*(a)), 0, #a, #b, __LINE__, __FILE__);
#define ctf_expect_array_uint_lte(a, b)                                   \
  CTF_INTERNAL_EXPECT(ctf_internal_expect_array_uint_lte,                 \
                      (const void *const *)a, (const void *const *)b,     \
                      sizeof(a) / sizeof(*(a)), sizeof(b) / sizeof(*(b)), \
                      sizeof(*(a)), 0, #a, #b, __LINE__, __FILE__);
#define ctf_expect_array_ptr_eq(a, b)                                     \
  CTF_INTERNAL_EXPECT(ctf_internal_expect_array_ptr_eq,                   \
                      (const void *const *)a, (const void *const *)b,     \
                      sizeof(a) / sizeof(*(a)), sizeof(b) / sizeof(*(b)), \
                      sizeof(*(a)), 0, #a, #b, __LINE__, __FILE__);
#define ctf_expect_array_ptr_neq(a, b)                                    \
  CTF_INTERNAL_EXPECT(ctf_internal_expect_array_ptr_neq,                  \
                      (const void *const *)a, (const void *const *)b,     \
                      sizeof(a) / sizeof(*(a)), sizeof(b) / sizeof(*(b)), \
                      sizeof(*(a)), 0, #a, #b, __LINE__, __FILE__);
#define ctf_expect_array_ptr_gt(a, b)                                     \
  CTF_INTERNAL_EXPECT(ctf_internal_expect_array_ptr_gt,                   \
                      (const void *const *)a, (const void *const *)b,     \
                      sizeof(a) / sizeof(*(a)), sizeof(b) / sizeof(*(b)), \
                      sizeof(*(a)), 0, #a, #b, __LINE__, __FILE__);
#define ctf_expect_array_ptr_lt(a, b)                                     \
  CTF_INTERNAL_EXPECT(ctf_internal_expect_array_ptr_lt,                   \
                      (const void *const *)a, (const void *const *)b,     \
                      sizeof(a) / sizeof(*(a)), sizeof(b) / sizeof(*(b)), \
                      sizeof(*(a)), 0, #a, #b, __LINE__, __FILE__);
#define ctf_expect_array_ptr_gte(a, b)                                    \
  CTF_INTERNAL_EXPECT(ctf_internal_expect_array_ptr_gte,                  \
                      (const void *const *)a, (const void *const *)b,     \
                      sizeof(a) / sizeof(*(a)), sizeof(b) / sizeof(*(b)), \
                      sizeof(*(a)), 0, #a, #b, __LINE__, __FILE__);
#define ctf_expect_array_ptr_lte(a, b)                                    \
  CTF_INTERNAL_EXPECT(ctf_internal_expect_array_ptr_lte,                  \
                      (const void *const *)a, (const void *const *)b,     \
                      sizeof(a) / sizeof(*(a)), sizeof(b) / sizeof(*(b)), \
                      sizeof(*(a)), 0, #a, #b, __LINE__, __FILE__);
#define ctf_expect_true(a) \
  CTF_INTERNAL_EXPECT(ctf_internal_expect_true, a, #a, __LINE__, __FILE__)
#define ctf_expect_false(a) \
  CTF_INTERNAL_EXPECT(ctf_internal_expect_false, a, #a, __LINE__, __FILE__)

#if CTF_ASSERT_ALIASES == CTF_ON
#define assert_msg(test, msg) ctf_assert_msg(test, msg)
#define assert(test) ctf_assert(test)
#define assert_char_eq(a, b) ctf_assert_char_eq(a, b)
#define assert_char_neq(a, b) ctf_assert_char_neq(a, b)
#define assert_char_gt(a, b) ctf_assert_char_gt(a, b)
#define assert_char_lt(a, b) ctf_assert_char_lt(a, b)
#define assert_char_gte(a, b) ctf_assert_char_gte(a, b)
#define assert_char_lte(a, b) ctf_assert_char_lte(a, b)
#define assert_int_eq(a, b) ctf_assert_int_eq(a, b)
#define assert_int_neq(a, b) ctf_assert_int_neq(a, b)
#define assert_int_gt(a, b) ctf_assert_int_gt(a, b)
#define assert_int_lt(a, b) ctf_assert_int_lt(a, b)
#define assert_int_gte(a, b) ctf_assert_int_gte(a, b)
#define assert_int_lte(a, b) ctf_assert_int_lte(a, b)
#define assert_uint_eq(a, b) ctf_assert_uint_eq(a, b)
#define assert_uint_neq(a, b) ctf_assert_uint_neq(a, b)
#define assert_uint_gt(a, b) ctf_assert_uint_gt(a, b)
#define assert_uint_lt(a, b) ctf_assert_uint_lt(a, b)
#define assert_uint_gte(a, b) ctf_assert_uint_gte(a, b)
#define assert_uint_lte(a, b) ctf_assert_uint_lte(a, b)
#define assert_ptr_eq(a, b) ctf_assert_ptr_eq(a, b)
#define assert_ptr_neq(a, b) ctf_assert_ptr_neq(a, b)
#define assert_ptr_gt(a, b) ctf_assert_ptr_gt(a, b)
#define assert_ptr_lt(a, b) ctf_assert_ptr_lt(a, b)
#define assert_ptr_gte(a, b) ctf_assert_ptr_gte(a, b)
#define assert_ptr_lte(a, b) ctf_assert_ptr_lte(a, b)
#define assert_string_eq(a, b) ctf_assert_string_eq(a, b)
#define assert_string_neq(a, b) ctf_assert_string_neq(a, b)
#define assert_string_gt(a, b) ctf_assert_string_gt(a, b)
#define assert_string_lt(a, b) ctf_assert_string_lt(a, b)
#define assert_string_gte(a, b) ctf_assert_string_gte(a, b)
#define assert_string_lte(a, b) ctf_assert_string_lte(a, b)
#define assert_memory_char_eq(a, b, length) \
  ctf_assert_memory_char_eq(a, b, length)
#define assert_memory_char_neq(a, b, length) \
  ctf_assert_memory_char_neq(a, b, length)
#define assert_memory_char_gt(a, b, length) \
  ctf_assert_memory_char_gt(a, b, length)
#define assert_memory_char_lt(a, b, length) \
  ctf_assert_memory_char_lt(a, b, length)
#define assert_memory_char_gte(a, b, length) \
  ctf_assert_memory_char_gte(a, b, length)
#define assert_memory_char_lte(a, b, length) \
  ctf_assert_memory_char_lte(a, b, length)
#define assert_memory_int_eq(a, b, length) \
  ctf_assert_memory_int_eq(a, b, length)
#define assert_memory_int_neq(a, b, length) \
  ctf_assert_memory_int_neq(a, b, length)
#define assert_memory_int_gt(a, b, length) \
  ctf_assert_memory_int_gt(a, b, length)
#define assert_memory_int_lt(a, b, length) \
  ctf_assert_memory_int_lt(a, b, length)
#define assert_memory_int_gte(a, b, length) \
  ctf_assert_memory_int_gte(a, b, length)
#define assert_memory_int_lte(a, b, length) \
  ctf_assert_memory_int_lte(a, b, length)
#define assert_memory_uint_eq(a, b, length) \
  ctf_assert_memory_uint_eq(a, b, length)
#define assert_memory_uint_neq(a, b, length) \
  ctf_assert_memory_uint_neq(a, b, length)
#define assert_memory_uint_gt(a, b, length) \
  ctf_assert_memory_uint_gt(a, b, length)
#define assert_memory_uint_lt(a, b, length) \
  ctf_assert_memory_uint_lt(a, b, length)
#define assert_memory_uint_gte(a, b, length) \
  ctf_assert_memory_uint_gte(a, b, length)
#define assert_memory_uint_lte(a, b, length) \
  ctf_assert_memory_uint_lte(a, b, length)
#define assert_memory_ptr_eq(a, b, length) \
  ctf_assert_memory_ptr_eq(a, b, length)
#define assert_memory_ptr_neq(a, b, length) \
  ctf_assert_memory_ptr_neq(a, b, length)
#define assert_memory_ptr_gt(a, b, length) \
  ctf_assert_memory_ptr_gt(a, b, length)
#define assert_memory_ptr_lt(a, b, length) \
  ctf_assert_memory_ptr_lt(a, b, length)
#define assert_memory_ptr_gte(a, b, length) \
  ctf_assert_memory_ptr_gte(a, b, length)
#define assert_memory_ptr_lte(a, b, length) \
  ctf_assert_memory_ptr_lte(a, b, length)
#define assert_array_char_eq(a, b) ctf_assert_array_char_eq(a, b)
#define assert_array_char_neq(a, b) ctf_assert_array_char_neq(a, b)
#define assert_array_char_gt(a, b) ctf_assert_array_char_gt(a, b)
#define assert_array_char_lt(a, b) ctf_assert_array_char_lt(a, b)
#define assert_array_char_gte(a, b) ctf_assert_array_char_gte(a, b)
#define assert_array_char_lte(a, b) ctf_assert_array_char_lte(a, b)
#define assert_array_int_eq(a, b) ctf_assert_array_int_eq(a, b)
#define assert_array_int_neq(a, b) ctf_assert_array_int_neq(a, b)
#define assert_array_int_gt(a, b) ctf_assert_array_int_gt(a, b)
#define assert_array_int_lt(a, b) ctf_assert_array_int_lt(a, b)
#define assert_array_int_gte(a, b) ctf_assert_array_int_gte(a, b)
#define assert_array_int_lte(a, b) ctf_assert_array_int_lte(a, b)
#define assert_array_uint_eq(a, b) ctf_assert_array_uint_eq(a, b)
#define assert_array_uint_neq(a, b) ctf_assert_array_uint_neq(a, b)
#define assert_array_uint_gt(a, b) ctf_assert_array_uint_gt(a, b)
#define assert_array_uint_lt(a, b) ctf_assert_array_uint_lt(a, b)
#define assert_array_uint_gte(a, b) ctf_assert_array_uint_gte(a, b)
#define assert_array_uint_lte(a, b) ctf_assert_array_uint_lte(a, b)
#define assert_array_ptr_eq(a, b) ctf_assert_array_ptr_eq(a, b)
#define assert_array_ptr_neq(a, b) ctf_assert_array_ptr_neq(a, b)
#define assert_array_ptr_gt(a, b) ctf_assert_array_ptr_gt(a, b)
#define assert_array_ptr_lt(a, b) ctf_assert_array_ptr_lt(a, b)
#define assert_array_ptr_gte(a, b) ctf_assert_array_ptr_gte(a, b)
#define assert_array_ptr_lte(a, b) ctf_assert_array_ptr_lte(a, b)
#define assert_true(a) ctf_assert_true(a)
#define assert_false(a) ctf_assert_false(a)

#define expect_msg(test, msg) ctf_expect_msg(test, msg)
#define expect(test) ctf_expect(test)
#define expect_char_eq(a, b) ctf_expect_char_eq(a, b)
#define expect_char_neq(a, b) ctf_expect_char_neq(a, b)
#define expect_char_gt(a, b) ctf_expect_char_gt(a, b)
#define expect_char_lt(a, b) ctf_expect_char_lt(a, b)
#define expect_char_gte(a, b) ctf_expect_char_gte(a, b)
#define expect_char_lte(a, b) ctf_expect_char_lte(a, b)
#define expect_int_eq(a, b) ctf_expect_int_eq(a, b)
#define expect_int_neq(a, b) ctf_expect_int_neq(a, b)
#define expect_int_gt(a, b) ctf_expect_int_gt(a, b)
#define expect_int_lt(a, b) ctf_expect_int_lt(a, b)
#define expect_int_gte(a, b) ctf_expect_int_gte(a, b)
#define expect_int_lte(a, b) ctf_expect_int_lte(a, b)
#define expect_uint_eq(a, b) ctf_expect_uint_eq(a, b)
#define expect_uint_neq(a, b) ctf_expect_uint_neq(a, b)
#define expect_uint_gt(a, b) ctf_expect_uint_gt(a, b)
#define expect_uint_lt(a, b) ctf_expect_uint_lt(a, b)
#define expect_uint_gte(a, b) ctf_expect_uint_gte(a, b)
#define expect_uint_lte(a, b) ctf_expect_uint_lte(a, b)
#define expect_ptr_eq(a, b) ctf_expect_ptr_eq(a, b)
#define expect_ptr_neq(a, b) ctf_expect_ptr_neq(a, b)
#define expect_ptr_gt(a, b) ctf_expect_ptr_gt(a, b)
#define expect_ptr_lt(a, b) ctf_expect_ptr_lt(a, b)
#define expect_ptr_gte(a, b) ctf_expect_ptr_gte(a, b)
#define expect_ptr_lte(a, b) ctf_expect_ptr_lte(a, b)
#define expect_string_eq(a, b) ctf_expect_string_eq(a, b)
#define expect_string_neq(a, b) ctf_expect_string_neq(a, b)
#define expect_string_gt(a, b) ctf_expect_string_gt(a, b)
#define expect_string_lt(a, b) ctf_expect_string_lt(a, b)
#define expect_string_gte(a, b) ctf_expect_string_gte(a, b)
#define expect_string_lte(a, b) ctf_expect_string_lte(a, b)
#define expect_memory_char_eq(a, b, length) \
  ctf_expect_memory_char_eq(a, b, length)
#define expect_memory_char_neq(a, b, length) \
  ctf_expect_memory_char_neq(a, b, length)
#define expect_memory_char_gt(a, b, length) \
  ctf_expect_memory_char_gt(a, b, length)
#define expect_memory_char_lt(a, b, length) \
  ctf_expect_memory_char_lt(a, b, length)
#define expect_memory_char_gte(a, b, length) \
  ctf_expect_memory_char_gte(a, b, length)
#define expect_memory_char_lte(a, b, length) \
  ctf_expect_memory_char_lte(a, b, length)
#define expect_memory_int_eq(a, b, length) \
  ctf_expect_memory_int_eq(a, b, length)
#define expect_memory_int_neq(a, b, length) \
  ctf_expect_memory_int_neq(a, b, length)
#define expect_memory_int_gt(a, b, length) \
  ctf_expect_memory_int_gt(a, b, length)
#define expect_memory_int_lt(a, b, length) \
  ctf_expect_memory_int_lt(a, b, length)
#define expect_memory_int_gte(a, b, length) \
  ctf_expect_memory_int_gte(a, b, length)
#define expect_memory_int_lte(a, b, length) \
  ctf_expect_memory_int_lte(a, b, length)
#define expect_memory_uint_eq(a, b, length) \
  ctf_expect_memory_uint_eq(a, b, length)
#define expect_memory_uint_neq(a, b, length) \
  ctf_expect_memory_uint_neq(a, b, length)
#define expect_memory_uint_gt(a, b, length) \
  ctf_expect_memory_uint_gt(a, b, length)
#define expect_memory_uint_lt(a, b, length) \
  ctf_expect_memory_uint_lt(a, b, length)
#define expect_memory_uint_gte(a, b, length) \
  ctf_expect_memory_uint_gte(a, b, length)
#define expect_memory_uint_lte(a, b, length) \
  ctf_expect_memory_uint_lte(a, b, length)
#define expect_memory_ptr_eq(a, b, length) \
  ctf_expect_memory_ptr_eq(a, b, length)
#define expect_memory_ptr_neq(a, b, length) \
  ctf_expect_memory_ptr_neq(a, b, length)
#define expect_memory_ptr_gt(a, b, length) \
  ctf_expect_memory_ptr_gt(a, b, length)
#define expect_memory_ptr_lt(a, b, length) \
  ctf_expect_memory_ptr_lt(a, b, length)
#define expect_memory_ptr_gte(a, b, length) \
  ctf_expect_memory_ptr_gte(a, b, length)
#define expect_memory_ptr_lte(a, b, length) \
  ctf_expect_memory_ptr_lte(a, b, length)
#define expect_array_char_eq(a, b) ctf_expect_array_char_eq(a, b)
#define expect_array_char_neq(a, b) ctf_expect_array_char_neq(a, b)
#define expect_array_char_gt(a, b) ctf_expect_array_char_gt(a, b)
#define expect_array_char_lt(a, b) ctf_expect_array_char_lt(a, b)
#define expect_array_char_gte(a, b) ctf_expect_array_char_gte(a, b)
#define expect_array_char_lte(a, b) ctf_expect_array_char_lte(a, b)
#define expect_array_int_eq(a, b) ctf_expect_array_int_eq(a, b)
#define expect_array_int_neq(a, b) ctf_expect_array_int_neq(a, b)
#define expect_array_int_gt(a, b) ctf_expect_array_int_gt(a, b)
#define expect_array_int_lt(a, b) ctf_expect_array_int_lt(a, b)
#define expect_array_int_gte(a, b) ctf_expect_array_int_gte(a, b)
#define expect_array_int_lte(a, b) ctf_expect_array_int_lte(a, b)
#define expect_array_uint_eq(a, b) ctf_expect_array_uint_eq(a, b)
#define expect_array_uint_neq(a, b) ctf_expect_array_uint_neq(a, b)
#define expect_array_uint_gt(a, b) ctf_expect_array_uint_gt(a, b)
#define expect_array_uint_lt(a, b) ctf_expect_array_uint_lt(a, b)
#define expect_array_uint_gte(a, b) ctf_expect_array_uint_gte(a, b)
#define expect_array_uint_lte(a, b) ctf_expect_array_uint_lte(a, b)
#define expect_array_ptr_eq(a, b) ctf_expect_array_ptr_eq(a, b)
#define expect_array_ptr_neq(a, b) ctf_expect_array_ptr_neq(a, b)
#define expect_array_ptr_gt(a, b) ctf_expect_array_ptr_gt(a, b)
#define expect_array_ptr_lt(a, b) ctf_expect_array_ptr_lt(a, b)
#define expect_array_ptr_gte(a, b) ctf_expect_array_ptr_gte(a, b)
#define expect_array_ptr_lte(a, b) ctf_expect_array_ptr_lte(a, b)
#define expect_true(a) ctf_expect_true(a)
#define expect_false(a) ctf_expect_false(a)
#endif

#endif
