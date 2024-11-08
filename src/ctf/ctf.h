#ifndef CTF_H
#define CTF_H

/* Parse options */
#define CTF_OFF 0
#define CTF_ON 1
#define CTF_AUTO 2
#define CTF_UNICODE_GENERIC 1
#define CTF_UNICODE_BRANDED 2
#define CTF_USER_DEFINED 3

#ifndef CTF_COLOR
#define CTF_COLOR CTF_AUTO
#endif
#ifndef CTF_DETAIL
#define CTF_DETAIL CTF_AUTO
#endif
#ifndef CTF_UNICODE
#define CTF_UNICODE CTF_UNICODE_BRANDED
#endif
#ifndef CTF_ASSERT_ALIASES
#define CTF_ASSERT_ALIASES CTF_ON
#endif

/* Includes */
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#ifdef CTF_PARALLEL
#include <pthread.h>
#endif

/* Consts */
#define CTF_CONST_STATE_FILE_SIZE_DEFAULT 256
#define CTF_CONST_STATE_MSG_SIZE_DEFAULT 4096
#define CTF_CONST_STATES_PER_THREAD_DEFAULT 64

#ifndef CTF_CONST_STATE_FILE_SIZE
#define CTF_CONST_STATE_FILE_SIZE CTF_CONST_STATE_FILE_SIZE_DEFAULT
#endif
#ifndef CTF_CONST_STATE_MSG_SIZE
#define CTF_CONST_STATE_MSG_SIZE CTF_CONST_STATE_MSG_SIZE_DEFAULT
#endif
#ifndef CTF_CONST_STATES_PER_THREAD
#define CTF_CONST_STATES_PER_THREAD CTF_CONST_STATES_PER_THREAD_DEFAULT
#endif

/* System specific */
#ifdef CTF_PARALLEL
#if __STDC_VERSION__ == 201112L
#define CTF_PARALLEL_INTERNAL_THREAD_LOCAL _Thread_local
#elif defined(__GNUC__)
#define CTF_PARALLEL_INTERNAL_THREAD_LOCAL __thread
#else
#error "Thread local storage not supported"
#endif
#endif

/* Utility */
#define CTF_INTERNAL_STRINGIFY(x) #x
#define CTF_INTERNAL_STRINGIFY2(x) CTF_INTERNAL_STRINGIFY(x)
#define CTF_INTERNAL_MIN(x, y) ((x) < (y) ? (x) : (y))
#define CTF_INTERNAL_IS_SIGNED(a) ((a) - (a) - 1 < 0)
#define CTF_INTERNAL_LENGTH(a) (sizeof(a) / sizeof(*(a)))

#ifdef CTF_PARALLEL
#define CTF_INTERNAL_EXPECT_END                                             \
  ctf_parallel_internal_states_index[ctf_parallel_internal_thread_index]++; \
  ctf_internal_state_index++;                                               \
  ctf_internal_state_p++
#else
#define CTF_INTERNAL_EXPECT_END \
  ctf_internal_state_index++;   \
  ctf_internal_state_p++
#endif

#define CTF_INTERNAL_ASSERT_BEGIN \
  if(ctf_internal_state_index == CTF_CONST_STATES_PER_THREAD) return
#define CTF_INTERNAL_ASSERT_END \
  CTF_INTERNAL_EXPECT_END;      \
  if(ctf_internal_state_p->status) return

#if CTF_DETAIL == CTF_OFF
#define CTF_INTERNAL_ASSERT_COPY
#else
#define CTF_INTERNAL_ASSERT_COPY \
  ctf_internal_assert_copy(ctf_internal_state_p, __LINE__, __FILE__)
#endif

#define CTF_INTERNAL_ASSERT_FUNC_RAW(a, b, op, type, format, f)               \
  do {                                                                        \
    const type _x = a;                                                        \
    const type _y = b;                                                        \
    ctf_internal_state_p->status = !(f(_x, _y) op 0);                         \
    snprintf(ctf_internal_state_p->msg, CTF_CONST_STATE_MSG_SIZE, format, #a, \
             #b, _x, _y);                                                     \
    CTF_INTERNAL_ASSERT_COPY;                                                 \
  } while(0)
#define CTF_INTERNAL_ASSERT_RAW(a, b, op, type, format)                       \
  do {                                                                        \
    const type _x = a;                                                        \
    const type _y = b;                                                        \
    ctf_internal_state_p->status = !(_x op _y);                               \
    snprintf(ctf_internal_state_p->msg, CTF_CONST_STATE_MSG_SIZE, format, #a, \
             #b, _x, _y);                                                     \
    CTF_INTERNAL_ASSERT_COPY;                                                 \
  } while(0)
#define CTF_INTERNAL_ASSERT_MEM_ARR_RAW(a, b, op, format, length1, length2)   \
  _Pragma("GCC diagnostic ignored \"-Wtype-limits\"");                        \
  ctf_internal_assert_mem_print(ctf_internal_state_p, a, b, length1, length2, \
                                sizeof(*(a)), CTF_INTERNAL_IS_SIGNED(*(a)),   \
                                #a, #b, #op, format, __LINE__, __FILE__);     \
  _Pragma("GCC diagnostic pop")
#define CTF_INTERNAL_ASSERT_MEM_RAW(a, b, op, format, length)        \
  CTF_INTERNAL_ASSERT_MEM_ARR_RAW(a, b, op, format, length, length); \
  ctf_internal_state_p->status = !(ctf_internal_state_p->status op 0)
#define CTF_INTERNAL_ASSERT_ARR_RAW(a, b, op, format)                       \
  CTF_INTERNAL_ASSERT_MEM_ARR_RAW(a, b, op, format, CTF_INTERNAL_LENGTH(a), \
                                  CTF_INTERNAL_LENGTH(b));                  \
  if(ctf_internal_state_p->status == 0) {                                   \
    ctf_internal_state_p->status =                                          \
      !(CTF_INTERNAL_LENGTH(a) op CTF_INTERNAL_LENGTH(b));                  \
  } else {                                                                  \
    ctf_internal_state_p->status = !(ctf_internal_state_p->status op 0);    \
  }
#define CTF_INTERNAL_ASSERT(a, b, op, type, format) \
  CTF_INTERNAL_ASSERT_BEGIN;                        \
  CTF_INTERNAL_ASSERT_RAW(a, b, op, type, format);  \
  CTF_INTERNAL_ASSERT_END
#define CTF_INTERNAL_ASSERT_FUNC(a, b, op, type, format, f) \
  CTF_INTERNAL_ASSERT_BEGIN;                                \
  CTF_INTERNAL_ASSERT_FUNC_RAW(a, b, op, type, format, f);  \
  CTF_INTERNAL_ASSERT_END
#define CTF_INTERNAL_ASSERT_MEM(a, b, op, format, length) \
  CTF_INTERNAL_ASSERT_BEGIN;                              \
  CTF_INTERNAL_ASSERT_MEM_RAW(a, b, op, format, length);  \
  CTF_INTERNAL_ASSERT_END
#define CTF_INTERNAL_ASSERT_ARR(a, b, op, format) \
  CTF_INTERNAL_ASSERT_BEGIN;                      \
  CTF_INTERNAL_ASSERT_ARR_RAW(a, b, op, format);  \
  CTF_INTERNAL_ASSERT_END
#define CTF_INTERNAL_EXPECT(a, b, op, type, format) \
  CTF_INTERNAL_ASSERT_BEGIN;                        \
  CTF_INTERNAL_ASSERT_RAW(a, b, op, type, format);  \
  CTF_INTERNAL_EXPECT_END
#define CTF_INTERNAL_EXPECT_FUNC(a, b, op, type, format, f) \
  CTF_INTERNAL_ASSERT_BEGIN;                                \
  CTF_INTERNAL_ASSERT_FUNC_RAW(a, b, op, type, format, f);  \
  CTF_INTERNAL_EXPECT_END
#define CTF_INTERNAL_EXPECT_MEM(a, b, op, format, length) \
  CTF_INTERNAL_ASSERT_BEGIN;                              \
  CTF_INTERNAL_ASSERT_MEM_RAW(a, b, op, format, length);  \
  CTF_INTERNAL_EXPECT_END
#define CTF_INTERNAL_EXPECT_ARR(a, b, op, format) \
  CTF_INTERNAL_ASSERT_BEGIN;                      \
  CTF_INTERNAL_ASSERT_ARR_RAW(a, b, op, format);  \
  CTF_INTERNAL_EXPECT_END

/* Types */
struct ctf_internal_state {
  int status;
#if CTF_DETAIL != CTF_OFF
  int line;
  char file[CTF_CONST_STATE_FILE_SIZE];
#endif
  char msg[CTF_CONST_STATE_MSG_SIZE];
};
typedef void (*const ctf_internal_test)(void);
struct ctf_internal_group {
  const ctf_internal_test *tests;
  const char *test_names;
  const int length;
  const char *name;
};

/* Variables */
extern int ctf_exit_code;
extern const char ctf_internal_print_arr_char[];
extern const char ctf_internal_print_arr_int[];
extern const char ctf_internal_print_arr_uint[];
extern const char ctf_internal_print_arr_ptr[];

extern const char ctf_internal_print_char_eq[];
extern const char ctf_internal_print_char_neq[];
extern const char ctf_internal_print_char_gt[];
extern const char ctf_internal_print_char_lt[];
extern const char ctf_internal_print_char_gte[];
extern const char ctf_internal_print_char_lte[];

extern const char ctf_internal_print_int_eq[];
extern const char ctf_internal_print_int_neq[];
extern const char ctf_internal_print_int_gt[];
extern const char ctf_internal_print_int_lt[];
extern const char ctf_internal_print_int_gte[];
extern const char ctf_internal_print_int_lte[];

extern const char ctf_internal_print_uint_eq[];
extern const char ctf_internal_print_uint_neq[];
extern const char ctf_internal_print_uint_gt[];
extern const char ctf_internal_print_uint_lt[];
extern const char ctf_internal_print_uint_gte[];
extern const char ctf_internal_print_uint_lte[];

extern const char ctf_internal_print_ptr_eq[];
extern const char ctf_internal_print_ptr_neq[];
extern const char ctf_internal_print_ptr_gt[];
extern const char ctf_internal_print_ptr_lt[];
extern const char ctf_internal_print_ptr_gte[];
extern const char ctf_internal_print_ptr_lte[];

extern const char ctf_internal_print_str_eq[];
extern const char ctf_internal_print_str_neq[];
extern const char ctf_internal_print_str_gt[];
extern const char ctf_internal_print_str_lt[];
extern const char ctf_internal_print_str_gte[];
extern const char ctf_internal_print_str_lte[];

#ifdef CTF_PARALLEL
extern int ctf_parallel_internal_state;
extern int ctf_parallel_internal_state;
extern pthread_t ctf_parallel_internal_threads[CTF_PARALLEL];
extern struct ctf_internal_state
  ctf_parallel_internal_states[CTF_PARALLEL][CTF_CONST_STATES_PER_THREAD];
extern int ctf_parallel_internal_states_index[CTF_PARALLEL];
extern CTF_PARALLEL_INTERNAL_THREAD_LOCAL int
  ctf_parallel_internal_thread_index;
extern CTF_PARALLEL_INTERNAL_THREAD_LOCAL struct ctf_internal_state
  *ctf_internal_states;
extern CTF_PARALLEL_INTERNAL_THREAD_LOCAL int ctf_internal_state_index;
extern CTF_PARALLEL_INTERNAL_THREAD_LOCAL struct ctf_internal_state
  *ctf_internal_state_p;
#else
extern struct ctf_internal_state
  ctf_internal_states[CTF_CONST_STATES_PER_THREAD];
extern int ctf_internal_state_index;
extern struct ctf_internal_state *ctf_internal_state_p;
#endif

/* Functions */
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
#if CTF_DETAIL != CTF_OFF
int ctf_internal_int_length(int a);
#endif

#ifdef CTF_PARALLEL
void ctf_parallel_internal_group_run(const struct ctf_internal_group *group);
void ctf_parallel_internal_groups_run(int count, ...);
#endif

/* UI */
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

#define ctf_fail(msg)                                                  \
  CTF_INTERNAL_ASSERT_BEGIN;                                           \
  do {                                                                 \
    ctf_internal_state_p->status = 1;                                  \
    strncpy(ctf_internal_state_p->msg, msg, CTF_CONST_STATE_MSG_SIZE); \
    CTF_INTERNAL_ASSERT_COPY;                                          \
    CTF_INTERNAL_ASSERT_END                                            \
  } while(0);
#define ctf_pass(msg)                                                  \
  do {                                                                 \
    CTF_INTERNAL_ASSERT_BEGIN;                                         \
    ctf_internal_state_p->status = 0;                                  \
    strncpy(ctf_internal_state_p->msg, msg, CTF_CONST_STATE_MSG_SIZE); \
    CTF_INTERNAL_ASSERT_COPY;                                          \
    CTF_INTERNAL_ASSERT_END                                            \
  } while(0);
#define ctf_assert_msg(test, m)                                      \
  do {                                                               \
    CTF_INTERNAL_ASSERT_BEGIN;                                       \
    ctf_internal_state_p->status = !((int)(test));                   \
    strncpy(ctf_internal_state_p->msg, m, CTF_CONST_STATE_MSG_SIZE); \
    CTF_INTERNAL_ASSERT_COPY;                                        \
    CTF_INTERNAL_ASSERT_END                                          \
  } while(0);
#define ctf_assert(test) ctf_assert_msg(test, CTF_INTERNAL_STRINGIFY2(test))
#define ctf_assert_char_eq(a, b) \
  CTF_INTERNAL_ASSERT(a, b, ==, char, ctf_internal_print_char_eq)
#define ctf_assert_char_neq(a, b) \
  CTF_INTERNAL_ASSERT(a, b, !=, char, ctf_internal_print_char_neq)
#define ctf_assert_char_gt(a, b) \
  CTF_INTERNAL_ASSERT(a, b, >, char, ctf_internal_print_char_gt)
#define ctf_assert_char_lt(a, b) \
  CTF_INTERNAL_ASSERT(a, b, <, char, ctf_internal_print_char_lt)
#define ctf_assert_char_gte(a, b) \
  CTF_INTERNAL_ASSERT(a, b, >=, char, ctf_internal_print_char_gte)
#define ctf_assert_char_lte(a, b) \
  CTF_INTERNAL_ASSERT(a, b, <=, char, ctf_internal_print_char_lte)
#define ctf_assert_int_eq(a, b) \
  CTF_INTERNAL_ASSERT(a, b, ==, intmax_t, ctf_internal_print_int_eq)
#define ctf_assert_int_neq(a, b) \
  CTF_INTERNAL_ASSERT(a, b, !=, intmax_t, ctf_internal_print_int_neq)
#define ctf_assert_int_gt(a, b) \
  CTF_INTERNAL_ASSERT(a, b, >, intmax_t, ctf_internal_print_int_gt)
#define ctf_assert_int_lt(a, b) \
  CTF_INTERNAL_ASSERT(a, b, <, intmax_t, ctf_internal_print_int_lt)
#define ctf_assert_int_gte(a, b) \
  CTF_INTERNAL_ASSERT(a, b, >=, intmax_t, ctf_internal_print_int_gte)
#define ctf_assert_int_lte(a, b) \
  CTF_INTERNAL_ASSERT(a, b, <=, intmax_t, ctf_internal_print_int_lte)
#define ctf_assert_uint_eq(a, b) \
  CTF_INTERNAL_ASSERT(a, b, ==, uintmax_t, ctf_internal_print_uint_eq)
#define ctf_assert_uint_neq(a, b) \
  CTF_INTERNAL_ASSERT(a, b, !=, uintmax_t, ctf_internal_print_uint_neq)
#define ctf_assert_uint_gt(a, b) \
  CTF_INTERNAL_ASSERT(a, b, >, uintmax_t, ctf_internal_print_uint_gt)
#define ctf_assert_uint_lt(a, b) \
  CTF_INTERNAL_ASSERT(a, b, <, uintmax_t, ctf_internal_print_uint_lt)
#define ctf_assert_uint_gte(a, b) \
  CTF_INTERNAL_ASSERT(a, b, >=, uintmax_t, ctf_internal_print_uint_gte)
#define ctf_assert_uint_lte(a, b) \
  CTF_INTERNAL_ASSERT(a, b, <=, uintmax_t, ctf_internal_print_uint_lte)
#define ctf_assert_ptr_eq(a, b) \
  CTF_INTERNAL_ASSERT(a, b, ==, void *const, ctf_internal_print_ptr_eq)
#define ctf_assert_ptr_neq(a, b) \
  CTF_INTERNAL_ASSERT(a, b, !=, void *const, ctf_internal_print_ptr_neq)
#define ctf_assert_ptr_gt(a, b) \
  CTF_INTERNAL_ASSERT(a, b, >, void *const, ctf_internal_print_ptr_gt)
#define ctf_assert_ptr_lt(a, b) \
  CTF_INTERNAL_ASSERT(a, b, <, void *const, ctf_internal_print_ptr_lt)
#define ctf_assert_ptr_gte(a, b) \
  CTF_INTERNAL_ASSERT(a, b, >=, void *const, ctf_internal_print_ptr_gte)
#define ctf_assert_ptr_lte(a, b) \
  CTF_INTERNAL_ASSERT(a, b, <=, void *const, ctf_internal_print_ptr_lte)
#define ctf_assert_string_eq(a, b)                                           \
  CTF_INTERNAL_ASSERT_FUNC(a, b, ==, char *const, ctf_internal_print_str_eq, \
                           strcmp)
#define ctf_assert_string_neq(a, b)                                           \
  CTF_INTERNAL_ASSERT_FUNC(a, b, !=, char *const, ctf_internal_print_str_neq, \
                           strcmp)
#define ctf_assert_string_gt(a, b)                                          \
  CTF_INTERNAL_ASSERT_FUNC(a, b, >, char *const, ctf_internal_print_str_gt, \
                           strcmp)
#define ctf_assert_string_lt(a, b)                                          \
  CTF_INTERNAL_ASSERT_FUNC(a, b, <, char *const, ctf_internal_print_str_lt, \
                           strcmp)
#define ctf_assert_string_gte(a, b)                                           \
  CTF_INTERNAL_ASSERT_FUNC(a, b, >=, char *const, ctf_internal_print_str_gte, \
                           strcmp)
#define ctf_assert_string_lte(a, b)                                           \
  CTF_INTERNAL_ASSERT_FUNC(a, b, <=, char *const, ctf_internal_print_str_lte, \
                           strcmp)
#define ctf_assert_memory_char_eq(a, b, length) \
  CTF_INTERNAL_ASSERT_MEM(a, b, ==, ctf_internal_print_arr_char, length)
#define ctf_assert_memory_char_neq(a, b, length) \
  CTF_INTERNAL_ASSERT_MEM(a, b, !=, ctf_internal_print_arr_char, length)
#define ctf_assert_memory_char_lt(a, b, length) \
  CTF_INTERNAL_ASSERT_MEM(a, b, <, ctf_internal_print_arr_char, length)
#define ctf_assert_memory_char_gt(a, b, length) \
  CTF_INTERNAL_ASSERT_MEM(a, b, >, ctf_internal_print_arr_char, length)
#define ctf_assert_memory_char_lte(a, b, length) \
  CTF_INTERNAL_ASSERT_MEM(a, b, <=, ctf_internal_print_arr_char, length)
#define ctf_assert_memory_char_gte(a, b, length) \
  CTF_INTERNAL_ASSERT_MEM(a, b, >=, ctf_internal_print_arr_char, length)
#define ctf_assert_memory_int_eq(a, b, length) \
  CTF_INTERNAL_ASSERT_MEM(a, b, ==, ctf_internal_print_arr_int, length)
#define ctf_assert_memory_int_neq(a, b, length) \
  CTF_INTERNAL_ASSERT_MEM(a, b, !=, ctf_internal_print_arr_int, length)
#define ctf_assert_memory_int_lt(a, b, length) \
  CTF_INTERNAL_ASSERT_MEM(a, b, <, ctf_internal_print_arr_int, length)
#define ctf_assert_memory_int_gt(a, b, length) \
  CTF_INTERNAL_ASSERT_MEM(a, b, >, ctf_internal_print_arr_int, length)
#define ctf_assert_memory_int_lte(a, b, length) \
  CTF_INTERNAL_ASSERT_MEM(a, b, <=, ctf_internal_print_arr_int, length)
#define ctf_assert_memory_int_gte(a, b, length) \
  CTF_INTERNAL_ASSERT_MEM(a, b, >=, ctf_internal_print_arr_int, length)
#define ctf_assert_memory_uint_eq(a, b, length) \
  CTF_INTERNAL_ASSERT_MEM(a, b, ==, ctf_internal_print_arr_uint, length)
#define ctf_assert_memory_uint_neq(a, b, length) \
  CTF_INTERNAL_ASSERT_MEM(a, b, !=, ctf_internal_print_arr_uint, length)
#define ctf_assert_memory_uint_lt(a, b, length) \
  CTF_INTERNAL_ASSERT_MEM(a, b, <, ctf_internal_print_arr_uint, length)
#define ctf_assert_memory_uint_gt(a, b, length) \
  CTF_INTERNAL_ASSERT_MEM(a, b, >, ctf_internal_print_arr_uint, length)
#define ctf_assert_memory_uint_lte(a, b, length) \
  CTF_INTERNAL_ASSERT_MEM(a, b, <=, ctf_internal_print_arr_uint, length)
#define ctf_assert_memory_uint_gte(a, b, length) \
  CTF_INTERNAL_ASSERT_MEM(a, b, >=, ctf_internal_print_arr_uint, length)
#define ctf_assert_memory_ptr_eq(a, b, length) \
  CTF_INTERNAL_ASSERT_MEM(a, b, ==, ctf_internal_print_arr_ptr, length)
#define ctf_assert_memory_ptr_neq(a, b, length) \
  CTF_INTERNAL_ASSERT_MEM(a, b, !=, ctf_internal_print_arr_ptr, length)
#define ctf_assert_memory_ptr_lt(a, b, length) \
  CTF_INTERNAL_ASSERT_MEM(a, b, <, ctf_internal_print_arr_ptr, length)
#define ctf_assert_memory_ptr_gt(a, b, length) \
  CTF_INTERNAL_ASSERT_MEM(a, b, >, ctf_internal_print_arr_ptr, length)
#define ctf_assert_memory_ptr_lte(a, b, length) \
  CTF_INTERNAL_ASSERT_MEM(a, b, <=, ctf_internal_print_arr_ptr, length)
#define ctf_assert_memory_ptr_gte(a, b, length) \
  CTF_INTERNAL_ASSERT_MEM(a, b, >=, ctf_internal_print_arr_ptr, length)
#define ctf_assert_array_char_eq(a, b) \
  CTF_INTERNAL_ASSERT_ARR(a, b, ==, ctf_internal_print_arr_char)
#define ctf_assert_array_char_neq(a, b) \
  CTF_INTERNAL_ASSERT_ARR(a, b, !=, ctf_internal_print_arr_char)
#define ctf_assert_array_char_lt(a, b) \
  CTF_INTERNAL_ASSERT_ARR(a, b, <, ctf_internal_print_arr_char)
#define ctf_assert_array_char_gt(a, b) \
  CTF_INTERNAL_ASSERT_ARR(a, b, >, ctf_internal_print_arr_char)
#define ctf_assert_array_char_lte(a, b) \
  CTF_INTERNAL_ASSERT_ARR(a, b, <=, ctf_internal_print_arr_char)
#define ctf_assert_array_char_gte(a, b) \
  CTF_INTERNAL_ASSERT_ARR(a, b, >=, ctf_internal_print_arr_char)
#define ctf_assert_array_int_eq(a, b) \
  CTF_INTERNAL_ASSERT_ARR(a, b, ==, ctf_internal_print_arr_int)
#define ctf_assert_array_int_neq(a, b) \
  CTF_INTERNAL_ASSERT_ARR(a, b, !=, ctf_internal_print_arr_int)
#define ctf_assert_array_int_lt(a, b) \
  CTF_INTERNAL_ASSERT_ARR(a, b, <, ctf_internal_print_arr_int)
#define ctf_assert_array_int_gt(a, b) \
  CTF_INTERNAL_ASSERT_ARR(a, b, >, ctf_internal_print_arr_int)
#define ctf_assert_array_int_lte(a, b) \
  CTF_INTERNAL_ASSERT_ARR(a, b, <=, ctf_internal_print_arr_int)
#define ctf_assert_array_int_gte(a, b) \
  CTF_INTERNAL_ASSERT_ARR(a, b, >=, ctf_internal_print_arr_int)
#define ctf_assert_array_uint_eq(a, b) \
  CTF_INTERNAL_ASSERT_ARR(a, b, ==, ctf_internal_print_arr_uint)
#define ctf_assert_array_uint_neq(a, b) \
  CTF_INTERNAL_ASSERT_ARR(a, b, !=, ctf_internal_print_arr_uint)
#define ctf_assert_array_uint_lt(a, b) \
  CTF_INTERNAL_ASSERT_ARR(a, b, <, ctf_internal_print_arr_uint)
#define ctf_assert_array_uint_gt(a, b) \
  CTF_INTERNAL_ASSERT_ARR(a, b, >, ctf_internal_print_arr_uint)
#define ctf_assert_array_uint_lte(a, b) \
  CTF_INTERNAL_ASSERT_ARR(a, b, <=, ctf_internal_print_arr_uint)
#define ctf_assert_array_uint_gte(a, b) \
  CTF_INTERNAL_ASSERT_ARR(a, b, >=, ctf_internal_print_arr_uint)
#define ctf_assert_array_ptr_eq(a, b) \
  CTF_INTERNAL_ASSERT_ARR(a, b, ==, ctf_internal_print_arr_ptr)
#define ctf_assert_array_ptr_neq(a, b) \
  CTF_INTERNAL_ASSERT_ARR(a, b, !=, ctf_internal_print_arr_ptr)
#define ctf_assert_array_ptr_lt(a, b) \
  CTF_INTERNAL_ASSERT_ARR(a, b, <, ctf_internal_print_arr_ptr)
#define ctf_assert_array_ptr_gt(a, b) \
  CTF_INTERNAL_ASSERT_ARR(a, b, >, ctf_internal_print_arr_ptr)
#define ctf_assert_array_ptr_lte(a, b) \
  CTF_INTERNAL_ASSERT_ARR(a, b, <=, ctf_internal_print_arr_ptr)
#define ctf_assert_array_ptr_gte(a, b) \
  CTF_INTERNAL_ASSERT_ARR(a, b, >=, ctf_internal_print_arr_ptr)
#define ctf_assert_true(a)                                        \
  do {                                                            \
    CTF_INTERNAL_ASSERT_BEGIN;                                    \
    const int _x = a;                                             \
    ctf_internal_state_p->status = !_x;                           \
    snprintf(ctf_internal_state_p->msg, CTF_CONST_STATE_MSG_SIZE, \
             "%s == true (%d != 0)", #a, _x);                     \
    CTF_INTERNAL_ASSERT_COPY;                                     \
    CTF_INTERNAL_ASSERT_END;                                      \
  } while(0)
#define ctf_assert_false(a)                                       \
  do {                                                            \
    CTF_INTERNAL_ASSERT_BEGIN;                                    \
    const int _x = a;                                             \
    ctf_internal_state_p->status = !!_x;                          \
    snprintf(ctf_internal_state_p->msg, CTF_CONST_STATE_MSG_SIZE, \
             "%s == false (%d == 0)", #a, _x);                    \
    CTF_INTERNAL_ASSERT_COPY;                                     \
    CTF_INTERNAL_ASSERT_END;                                      \
  } while(0)

#define ctf_expect_msg(test, m)                                      \
  do {                                                               \
    CTF_INTERNAL_ASSERT_BEGIN;                                       \
    ctf_internal_state_p->status = !((int)(test));                   \
    strncpy(ctf_internal_state_p->msg, m, CTF_CONST_STATE_MSG_SIZE); \
    CTF_INTERNAL_ASSERT_COPY;                                        \
    CTF_INTERNAL_EXPECT_END;                                         \
  } while(0);
#define ctf_expect(test) ctf_expect_msg(test, CTF_INTERNAL_STRINGIFY2(test))
#define ctf_expect_char_eq(a, b) \
  CTF_INTERNAL_EXPECT(a, b, ==, char, ctf_internal_print_char_eq)
#define ctf_expect_char_neq(a, b) \
  CTF_INTERNAL_EXPECT(a, b, !=, char, ctf_internal_print_char_neq)
#define ctf_expect_char_gt(a, b) \
  CTF_INTERNAL_EXPECT(a, b, >, char, ctf_internal_print_char_gt)
#define ctf_expect_char_lt(a, b) \
  CTF_INTERNAL_EXPECT(a, b, <, char, ctf_internal_print_char_lt)
#define ctf_expect_char_gte(a, b) \
  CTF_INTERNAL_EXPECT(a, b, >=, char, ctf_internal_print_char_gte)
#define ctf_expect_char_lte(a, b) \
  CTF_INTERNAL_EXPECT(a, b, <=, char, ctf_internal_print_char_lte)
#define ctf_expect_int_eq(a, b) \
  CTF_INTERNAL_EXPECT(a, b, ==, intmax_t, ctf_internal_print_int_eq)
#define ctf_expect_int_neq(a, b) \
  CTF_INTERNAL_EXPECT(a, b, !=, intmax_t, ctf_internal_print_int_neq)
#define ctf_expect_int_gt(a, b) \
  CTF_INTERNAL_EXPECT(a, b, >, intmax_t, ctf_internal_print_int_gt)
#define ctf_expect_int_lt(a, b) \
  CTF_INTERNAL_EXPECT(a, b, <, intmax_t, ctf_internal_print_int_lt)
#define ctf_expect_int_gte(a, b) \
  CTF_INTERNAL_EXPECT(a, b, >=, intmax_t, ctf_internal_print_int_gte)
#define ctf_expect_int_lte(a, b) \
  CTF_INTERNAL_EXPECT(a, b, <=, intmax_t, ctf_internal_print_int_lte)
#define ctf_expect_uint_eq(a, b) \
  CTF_INTERNAL_EXPECT(a, b, ==, uintmax_t, ctf_internal_print_uint_eq)
#define ctf_expect_uint_neq(a, b) \
  CTF_INTERNAL_EXPECT(a, b, !=, uintmax_t, ctf_internal_print_uint_neq)
#define ctf_expect_uint_gt(a, b) \
  CTF_INTERNAL_EXPECT(a, b, >, uintmax_t, ctf_internal_print_uint_gt)
#define ctf_expect_uint_lt(a, b) \
  CTF_INTERNAL_EXPECT(a, b, <, uintmax_t, ctf_internal_print_uint_lt)
#define ctf_expect_uint_gte(a, b) \
  CTF_INTERNAL_EXPECT(a, b, >=, uintmax_t, ctf_internal_print_uint_gte)
#define ctf_expect_uint_lte(a, b) \
  CTF_INTERNAL_EXPECT(a, b, <=, uintmax_t, ctf_internal_print_uint_lte)
#define ctf_expect_ptr_eq(a, b) \
  CTF_INTERNAL_EXPECT(a, b, ==, void *const, ctf_internal_print_ptr_eq)
#define ctf_expect_ptr_neq(a, b) \
  CTF_INTERNAL_EXPECT(a, b, !=, void *const, ctf_internal_print_ptr_neq)
#define ctf_expect_ptr_gt(a, b) \
  CTF_INTERNAL_EXPECT(a, b, >, void *const, ctf_internal_print_ptr_gt)
#define ctf_expect_ptr_lt(a, b) \
  CTF_INTERNAL_EXPECT(a, b, <, void *const, ctf_internal_print_ptr_lt)
#define ctf_expect_ptr_gte(a, b) \
  CTF_INTERNAL_EXPECT(a, b, >=, void *const, ctf_internal_print_ptr_gte)
#define ctf_expect_ptr_lte(a, b) \
  CTF_INTERNAL_EXPECT(a, b, <=, void *const, ctf_internal_print_ptr_lte)
#define ctf_expect_string_eq(a, b)                                           \
  CTF_INTERNAL_EXPECT_FUNC(a, b, ==, char *const, ctf_internal_print_str_eq, \
                           strcmp)
#define ctf_expect_string_neq(a, b)                                           \
  CTF_INTERNAL_EXPECT_FUNC(a, b, !=, char *const, ctf_internal_print_str_neq, \
                           strcmp)
#define ctf_expect_string_gt(a, b)                                          \
  CTF_INTERNAL_EXPECT_FUNC(a, b, >, char *const, ctf_internal_print_str_gt, \
                           strcmp)
#define ctf_expect_string_lt(a, b)                                          \
  CTF_INTERNAL_EXPECT_FUNC(a, b, <, char *const, ctf_internal_print_str_lt, \
                           strcmp)
#define ctf_expect_string_gte(a, b)                                           \
  CTF_INTERNAL_EXPECT_FUNC(a, b, >=, char *const, ctf_internal_print_str_gte, \
                           strcmp)
#define ctf_expect_string_lte(a, b)                                           \
  CTF_INTERNAL_EXPECT_FUNC(a, b, <=, char *const, ctf_internal_print_str_lte, \
                           strcmp)
#define ctf_expect_memory_char_eq(a, b, length) \
  CTF_INTERNAL_EXPECT_MEM(a, b, ==, ctf_internal_print_arr_char, length)
#define ctf_expect_memory_char_neq(a, b, length) \
  CTF_INTERNAL_EXPECT_MEM(a, b, !=, ctf_internal_print_arr_char, length)
#define ctf_expect_memory_char_lt(a, b, length) \
  CTF_INTERNAL_EXPECT_MEM(a, b, <, ctf_internal_print_arr_char, length)
#define ctf_expect_memory_char_gt(a, b, length) \
  CTF_INTERNAL_EXPECT_MEM(a, b, >, ctf_internal_print_arr_char, length)
#define ctf_expect_memory_char_lte(a, b, length) \
  CTF_INTERNAL_EXPECT_MEM(a, b, <=, ctf_internal_print_arr_char, length)
#define ctf_expect_memory_char_gte(a, b, length) \
  CTF_INTERNAL_EXPECT_MEM(a, b, >=, ctf_internal_print_arr_char, length)
#define ctf_expect_memory_int_eq(a, b, length) \
  CTF_INTERNAL_EXPECT_MEM(a, b, ==, ctf_internal_print_arr_int, length)
#define ctf_expect_memory_int_neq(a, b, length) \
  CTF_INTERNAL_EXPECT_MEM(a, b, !=, ctf_internal_print_arr_int, length)
#define ctf_expect_memory_int_lt(a, b, length) \
  CTF_INTERNAL_EXPECT_MEM(a, b, <, ctf_internal_print_arr_int, length)
#define ctf_expect_memory_int_gt(a, b, length) \
  CTF_INTERNAL_EXPECT_MEM(a, b, >, ctf_internal_print_arr_int, length)
#define ctf_expect_memory_int_lte(a, b, length) \
  CTF_INTERNAL_EXPECT_MEM(a, b, <=, ctf_internal_print_arr_int, length)
#define ctf_expect_memory_int_gte(a, b, length) \
  CTF_INTERNAL_EXPECT_MEM(a, b, >=, ctf_internal_print_arr_int, length)
#define ctf_expect_memory_uint_eq(a, b, length) \
  CTF_INTERNAL_EXPECT_MEM(a, b, ==, ctf_internal_print_arr_uint, length)
#define ctf_expect_memory_uint_neq(a, b, length) \
  CTF_INTERNAL_EXPECT_MEM(a, b, !=, ctf_internal_print_arr_uint, length)
#define ctf_expect_memory_uint_lt(a, b, length) \
  CTF_INTERNAL_EXPECT_MEM(a, b, <, ctf_internal_print_arr_uint, length)
#define ctf_expect_memory_uint_gt(a, b, length) \
  CTF_INTERNAL_EXPECT_MEM(a, b, >, ctf_internal_print_arr_uint, length)
#define ctf_expect_memory_uint_lte(a, b, length) \
  CTF_INTERNAL_EXPECT_MEM(a, b, <=, ctf_internal_print_arr_uint, length)
#define ctf_expect_memory_uint_gte(a, b, length) \
  CTF_INTERNAL_EXPECT_MEM(a, b, >=, ctf_internal_print_arr_uint, length)
#define ctf_expect_memory_ptr_eq(a, b, length) \
  CTF_INTERNAL_EXPECT_MEM(a, b, ==, ctf_internal_print_arr_ptr, length)
#define ctf_expect_memory_ptr_neq(a, b, length) \
  CTF_INTERNAL_EXPECT_MEM(a, b, !=, ctf_internal_print_arr_ptr, length)
#define ctf_expect_memory_ptr_lt(a, b, length) \
  CTF_INTERNAL_EXPECT_MEM(a, b, <, ctf_internal_print_arr_ptr, length)
#define ctf_expect_memory_ptr_gt(a, b, length) \
  CTF_INTERNAL_EXPECT_MEM(a, b, >, ctf_internal_print_arr_ptr, length)
#define ctf_expect_memory_ptr_lte(a, b, length) \
  CTF_INTERNAL_EXPECT_MEM(a, b, <=, ctf_internal_print_arr_ptr, length)
#define ctf_expect_memory_ptr_gte(a, b, length) \
  CTF_INTERNAL_EXPECT_MEM(a, b, >=, ctf_internal_print_arr_ptr, length)
#define ctf_expect_array_char_eq(a, b) \
  CTF_INTERNAL_EXPECT_ARR(a, b, ==, ctf_internal_print_arr_char)
#define ctf_expect_array_char_neq(a, b) \
  CTF_INTERNAL_EXPECT_ARR(a, b, !=, ctf_internal_print_arr_char)
#define ctf_expect_array_char_lt(a, b) \
  CTF_INTERNAL_EXPECT_ARR(a, b, <, ctf_internal_print_arr_char)
#define ctf_expect_array_char_gt(a, b) \
  CTF_INTERNAL_EXPECT_ARR(a, b, >, ctf_internal_print_arr_char)
#define ctf_expect_array_char_lte(a, b) \
  CTF_INTERNAL_EXPECT_ARR(a, b, <=, ctf_internal_print_arr_char)
#define ctf_expect_array_char_gte(a, b) \
  CTF_INTERNAL_EXPECT_ARR(a, b, >=, ctf_internal_print_arr_char)
#define ctf_expect_array_int_eq(a, b) \
  CTF_INTERNAL_EXPECT_ARR(a, b, ==, ctf_internal_print_arr_int)
#define ctf_expect_array_int_neq(a, b) \
  CTF_INTERNAL_EXPECT_ARR(a, b, !=, ctf_internal_print_arr_int)
#define ctf_expect_array_int_lt(a, b) \
  CTF_INTERNAL_EXPECT_ARR(a, b, <, ctf_internal_print_arr_int)
#define ctf_expect_array_int_gt(a, b) \
  CTF_INTERNAL_EXPECT_ARR(a, b, >, ctf_internal_print_arr_int)
#define ctf_expect_array_int_lte(a, b) \
  CTF_INTERNAL_EXPECT_ARR(a, b, <=, ctf_internal_print_arr_int)
#define ctf_expect_array_int_gte(a, b) \
  CTF_INTERNAL_EXPECT_ARR(a, b, >=, ctf_internal_print_arr_int)
#define ctf_expect_array_uint_eq(a, b) \
  CTF_INTERNAL_EXPECT_ARR(a, b, ==, ctf_internal_print_arr_uint)
#define ctf_expect_array_uint_neq(a, b) \
  CTF_INTERNAL_EXPECT_ARR(a, b, !=, ctf_internal_print_arr_uint)
#define ctf_expect_array_uint_lt(a, b) \
  CTF_INTERNAL_EXPECT_ARR(a, b, <, ctf_internal_print_arr_uint)
#define ctf_expect_array_uint_gt(a, b) \
  CTF_INTERNAL_EXPECT_ARR(a, b, >, ctf_internal_print_arr_uint)
#define ctf_expect_array_uint_lte(a, b) \
  CTF_INTERNAL_EXPECT_ARR(a, b, <=, ctf_internal_print_arr_uint)
#define ctf_expect_array_uint_gte(a, b) \
  CTF_INTERNAL_EXPECT_ARR(a, b, >=, ctf_internal_print_arr_uint)
#define ctf_expect_array_ptr_eq(a, b) \
  CTF_INTERNAL_EXPECT_ARR(a, b, ==, ctf_internal_print_arr_ptr)
#define ctf_expect_array_ptr_neq(a, b) \
  CTF_INTERNAL_EXPECT_ARR(a, b, !=, ctf_internal_print_arr_ptr)
#define ctf_expect_array_ptr_lt(a, b) \
  CTF_INTERNAL_EXPECT_ARR(a, b, <, ctf_internal_print_arr_ptr)
#define ctf_expect_array_ptr_gt(a, b) \
  CTF_INTERNAL_EXPECT_ARR(a, b, >, ctf_internal_print_arr_ptr)
#define ctf_expect_array_ptr_lte(a, b) \
  CTF_INTERNAL_EXPECT_ARR(a, b, <=, ctf_internal_print_arr_ptr)
#define ctf_expect_array_ptr_gte(a, b) \
  CTF_INTERNAL_EXPECT_ARR(a, b, >=, ctf_internal_print_arr_ptr)
#define ctf_expect_true(a)                                        \
  do {                                                            \
    CTF_INTERNAL_ASSERT_BEGIN;                                    \
    const int _x = a;                                             \
    ctf_internal_state_p->status = !_x;                           \
    snprintf(ctf_internal_state_p->msg, CTF_CONST_STATE_MSG_SIZE, \
             "%s == true (%d != 0)", #a, _x);                     \
    CTF_INTERNAL_ASSERT_COPY;                                     \
    CTF_INTERNAL_EXPECT_END;                                      \
  } while(0)
#define ctf_expect_false(a)                                       \
  do {                                                            \
    CTF_INTERNAL_ASSERT_BEGIN;                                    \
    const int _x = a;                                             \
    ctf_internal_state_p->status = _x;                            \
    snprintf(ctf_internal_state_p->msg, CTF_CONST_STATE_MSG_SIZE, \
             "%s == false (%d == 0)", #a, _x);                    \
    CTF_INTERNAL_ASSERT_COPY;                                     \
    CTF_INTERNAL_EXPECT_END;                                      \
  } while(0)

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
