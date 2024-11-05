#ifndef CTF_H
#define CTF_H

/* Parse options */
#define CTF_OFF 0
#define CTF_ON 1
#define CTF_AUTO 2
#define CTF_UNICODE_GENERIC 1
#define CTF_UNICODE_BRANDED 2

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

/* Consts */
#define CTF_CONST_STATE_FILE_SIZE_DEFAULT 256
#define CTF_CONST_STATE_MSG_SIZE_DEFAULT 4096
#define CTF_CONST_STATES_PER_TEST_DEFAULT 64

#ifndef CTF_CONST_STATE_FILE_SIZE
#define CTF_CONST_STATE_FILE_SIZE CTF_CONST_STATE_FILE_SIZE_DEFAULT
#endif
#ifndef CTF_CONST_STATE_MSG_SIZE
#define CTF_CONST_STATE_MSG_SIZE CTF_CONST_STATE_MSG_SIZE_DEFAULT
#endif
#ifndef CTF_CONST_STATES_PER_TEST
#define CTF_CONST_STATES_PER_TEST CTF_CONST_STATES_PER_TEST_DEFAULT
#endif

/* Utility */
#define CTF_INTERNAL_STRINGIFY(x) #x
#define CTF_INTERNAL_STRINGIFY2(x) CTF_INTERNAL_STRINGIFY(x)
#define CTF_INTERNAL_MIN(x, y) ((x) < (y) ? (x) : (y))

#define CTF_INTERNAL_ASSERT_BEGIN                              \
  do {                                                         \
    if(ctf_internal_next_state == CTF_CONST_STATES_PER_TEST) { \
      return;                                                  \
    }                                                          \
  } while(0)
#define CTF_INTERNAL_ASSERT_COPY                         \
  do {                                                   \
    err[ctf_internal_next_state].line = __LINE__;        \
    strncpy(err[ctf_internal_next_state].file, __FILE__, \
            CTF_CONST_STATE_FILE_SIZE);                  \
  } while(0)
#define CTF_INTERNAL_ASSERT_END                     \
  do {                                              \
    if(err[ctf_internal_next_state].status) return; \
    ctf_internal_next_state++;                      \
  } while(0)
#define CTF_INTERNAL_EXPECT_END \
  do {                          \
    ctf_internal_next_state++;  \
  } while(0)
#define CTF_INTERNAL_ASSERT_FUNC_RAW(a, b, op, type, format, f)                \
  do {                                                                         \
    CTF_INTERNAL_ASSERT_BEGIN;                                                 \
    const type _x = a;                                                         \
    const type _y = b;                                                         \
    err[ctf_internal_next_state].status = !(f(_x, _y) op 0);                   \
    snprintf(err[ctf_internal_next_state].msg, CTF_CONST_STATE_MSG_SIZE,       \
             "%s %s %s (" format " %s " format ")", #a, #op, #b, _x, #op, _y); \
    CTF_INTERNAL_ASSERT_COPY;                                                  \
  } while(0)
#define CTF_INTERNAL_ASSERT_RAW(a, b, op, type, format)                        \
  do {                                                                         \
    CTF_INTERNAL_ASSERT_BEGIN;                                                 \
    const type _x = a;                                                         \
    const type _y = b;                                                         \
    err[ctf_internal_next_state].status = !(_x op _y);                         \
    snprintf(err[ctf_internal_next_state].msg, CTF_CONST_STATE_MSG_SIZE,       \
             "%s %s %s (" format " %s " format ")", #a, #op, #b, _x, #op, _y); \
    CTF_INTERNAL_ASSERT_COPY;                                                  \
  } while(0)
#define CTF_INTERNAL_ASSERT_MEM_RAW(a, b, op, type, format, length)          \
  do {                                                                       \
    __typeof__(&*(a)) _x = a;                                                \
    __typeof__(&*(b)) _y = b;                                                \
    intmax_t _v;                                                             \
    const size_t _l = length;                                                \
    size_t _index;                                                           \
    err[ctf_internal_next_state].status =                                    \
      !(memcmp((a), (b), _l * sizeof(*(a))) op 0);                           \
    CTF_INTERNAL_ASSERT_COPY;                                                \
    _index = snprintf(err[ctf_internal_next_state].msg,                      \
                      CTF_CONST_STATE_MSG_SIZE, "%s %s %s ({", #a, #op, #b); \
    if(_l > 0) {                                                             \
      for(size_t i = 0; i < _l; i++) {                                       \
        _v = _x[i];                                                          \
        _index +=                                                            \
          snprintf(err[ctf_internal_next_state].msg + _index,                \
                   CTF_CONST_STATE_MSG_SIZE - _index, format ", ", _v);      \
      }                                                                      \
      _index -= 2;                                                           \
    }                                                                        \
    _index += snprintf(err[ctf_internal_next_state].msg + _index,            \
                       CTF_CONST_STATE_MSG_SIZE - _index, "} %s {", #op);    \
    if(_l > 0) {                                                             \
      for(size_t i = 0; i < _l; i++) {                                       \
        _v = _y[i];                                                          \
        _index +=                                                            \
          snprintf(err[ctf_internal_next_state].msg + _index,                \
                   CTF_CONST_STATE_MSG_SIZE - _index, format ", ", _v);      \
      }                                                                      \
      _index -= 2;                                                           \
    }                                                                        \
    snprintf(err[ctf_internal_next_state].msg + _index,                      \
             CTF_CONST_STATE_MSG_SIZE - _index, "})");                       \
  } while(0)
#define CTF_INTERNAL_ASSERT_ARR_RAW(a, b, op, type, format)  \
  do {                                                       \
    const size_t _la = sizeof(a) / sizeof(*a);               \
    const size_t _lb = sizeof(b) / sizeof(*b);               \
    CTF_INTERNAL_ASSERT_MEM_RAW(a, b, op, type, format,      \
                                CTF_INTERNAL_MIN(_la, _lb)); \
    err[ctf_internal_next_state].status |= _la != _lb;       \
  } while(0)

#define CTF_INTERNAL_ASSERT(a, b, op, type, format)  \
  do {                                               \
    CTF_INTERNAL_ASSERT_RAW(a, b, op, type, format); \
    CTF_INTERNAL_ASSERT_END;                         \
  } while(0)
#define CTF_INTERNAL_ASSERT_FUNC(a, b, op, type, format, f)  \
  do {                                                       \
    CTF_INTERNAL_ASSERT_FUNC_RAW(a, b, op, type, format, f); \
    CTF_INTERNAL_ASSERT_END;                                 \
  } while(0)
#define CTF_INTERNAL_ASSERT_MEM(a, b, op, type, format, length)  \
  do {                                                           \
    CTF_INTERNAL_ASSERT_MEM_RAW(a, b, op, type, format, length); \
    CTF_INTERNAL_ASSERT_END;                                     \
  } while(0)
#define CTF_INTERNAL_ASSERT_ARR(a, b, op, type, format)  \
  do {                                                   \
    CTF_INTERNAL_ASSERT_ARR_RAW(a, b, op, type, format); \
    CTF_INTERNAL_ASSERT_END;                             \
  } while(0)
#define CTF_INTERNAL_EXPECT(a, b, op, type, format)  \
  do {                                               \
    CTF_INTERNAL_ASSERT_RAW(a, b, op, type, format); \
    CTF_INTERNAL_EXPECT_END;                         \
  } while(0)
#define CTF_INTERNAL_EXPECT_FUNC(a, b, op, type, format, f)  \
  do {                                                       \
    CTF_INTERNAL_ASSERT_FUNC_RAW(a, b, op, type, format, f); \
    CTF_INTERNAL_EXPECT_END;                                 \
  } while(0)
#define CTF_INTERNAL_EXPECT_MEM(a, b, op, type, format, length)  \
  do {                                                           \
    CTF_INTERNAL_ASSERT_MEM_RAW(a, b, op, type, format, length); \
    CTF_INTERNAL_EXPECT_END;                                     \
  } while(0)
#define CTF_INTERNAL_EXPECT_ARR(a, b, op, type, format)  \
  do {                                                   \
    CTF_INTERNAL_ASSERT_ARR_RAW(a, b, op, type, format); \
    CTF_INTERNAL_EXPECT_END;                             \
  } while(0)

/* Types */
struct ctf_internal_state {
  int status;
  int line;
  char file[CTF_CONST_STATE_FILE_SIZE];
  char msg[CTF_CONST_STATE_MSG_SIZE];
};
typedef void (*const ctf_internal_test)(
  struct ctf_internal_state err[static CTF_CONST_STATES_PER_TEST],
  int ctf_internal_next_state);
struct ctf_internal_group {
  const ctf_internal_test *tests;
  const char *test_names;
  const int length;
  const char *name;
};

/* Variables */
extern int ctf_exit_code;

#ifdef CTF_PARALLEL
extern int ctf_parallel_internal_state;
#endif

/* Functions */
void ctf_internal_group_run(const struct ctf_internal_group *group);
void ctf_internal_groups_run(int count, ...);

#if CTF_DETAIL != CTF_OFF
int ctf_internal_int_length(int a);
#endif

#ifdef CTF_PARALLEL
void ctf_parallel_internal_group_run(const struct ctf_internal_group *group);
void ctf_parallel_internal_groups_run(int count, ...);
void ctf_parallel_internal_wait_until_done(void);
#endif

/* UI */
#define CTF_EXTERN_TEST(name)                                                \
  void name(struct ctf_internal_state err[static CTF_CONST_STATES_PER_TEST], \
            int ctf_internal_next_state)
#define CTF_TEST(name)                                                       \
  void name(struct ctf_internal_state err[static CTF_CONST_STATES_PER_TEST], \
            int ctf_internal_next_state)
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
#else
void ctf_parallel_start(void);
void ctf_parallel_stop(void);
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
#define ctf_barrier()                        \
  do {                                       \
    ctf_parallel_internal_wait_until_done(); \
    if(ctf_exit_code) {                      \
      ctf_parallel_stop();                   \
      return ctf_exit_code;                  \
    }                                        \
    fflush(stdout);                          \
  } while(0)
#endif

#define ctf_fail(msg)                                                         \
  do {                                                                        \
    CTF_INTERNAL_ASSERT_BEGIN;                                                \
    err[ctf_internal_next_state].status = 1;                                  \
    strncpy(err[ctf_internal_next_state].msg, msg, CTF_CONST_STATE_MSG_SIZE); \
    CTF_INTERNAL_ASSERT_COPY;                                                 \
    CTF_INTERNAL_ASSERT_END                                                   \
  } while(0);
#define ctf_pass(msg)                                                         \
  do {                                                                        \
    CTF_INTERNAL_ASSERT_BEGIN;                                                \
    err[ctf_internal_next_state].status = 0;                                  \
    strncpy(err[ctf_internal_next_state].msg, msg, CTF_CONST_STATE_MSG_SIZE); \
    CTF_INTERNAL_ASSERT_COPY;                                                 \
    CTF_INTERNAL_ASSERT_END                                                   \
  } while(0);
#define ctf_assert_msg(test, m)                                             \
  do {                                                                      \
    CTF_INTERNAL_ASSERT_BEGIN;                                              \
    err[ctf_internal_next_state].status = !((int)(test));                   \
    strncpy(err[ctf_internal_next_state].msg, m, CTF_CONST_STATE_MSG_SIZE); \
    CTF_INTERNAL_ASSERT_COPY;                                               \
    CTF_INTERNAL_ASSERT_END                                                 \
  } while(0);
#define ctf_assert(test) ctf_assert_msg(test, CTF_INTERNAL_STRINGIFY2(test))
#define ctf_assert_char_eq(a, b) CTF_INTERNAL_ASSERT(a, b, ==, char, "%c");
#define ctf_assert_char_neq(a, b) CTF_INTERNAL_ASSERT(a, b, !=, char, "%c");
#define ctf_assert_char_gt(a, b) CTF_INTERNAL_ASSERT(a, b, >, char, "%c");
#define ctf_assert_char_lt(a, b) CTF_INTERNAL_ASSERT(a, b, <, char, "%c");
#define ctf_assert_char_gte(a, b) CTF_INTERNAL_ASSERT(a, b, >=, char, "%c");
#define ctf_assert_char_lte(a, b) CTF_INTERNAL_ASSERT(a, b, <=, char, "%c");
#define ctf_assert_int_eq(a, b) CTF_INTERNAL_ASSERT(a, b, ==, intmax_t, "%jd");
#define ctf_assert_int_neq(a, b) CTF_INTERNAL_ASSERT(a, b, !=, intmax_t, "%jd");
#define ctf_assert_int_gt(a, b) CTF_INTERNAL_ASSERT(a, b, >, intmax_t, "%jd");
#define ctf_assert_int_lt(a, b) CTF_INTERNAL_ASSERT(a, b, <, intmax_t, "%jd");
#define ctf_assert_int_gte(a, b) CTF_INTERNAL_ASSERT(a, b, >=, intmax_t, "%jd");
#define ctf_assert_int_lte(a, b) CTF_INTERNAL_ASSERT(a, b, <=, intmax_t, "%jd");
#define ctf_assert_uint_eq(a, b) \
  CTF_INTERNAL_ASSERT(a, b, ==, uintmax_t, "%ju");
#define ctf_assert_uint_neq(a, b) \
  CTF_INTERNAL_ASSERT(a, b, !=, uintmax_t, "%ju");
#define ctf_assert_uint_gt(a, b) CTF_INTERNAL_ASSERT(a, b, >, uintmax_t, "%ju");
#define ctf_assert_uint_lt(a, b) CTF_INTERNAL_ASSERT(a, b, <, uintmax_t, "%ju");
#define ctf_assert_uint_gte(a, b) \
  CTF_INTERNAL_ASSERT(a, b, >=, uintmax_t, "%ju");
#define ctf_assert_uint_lte(a, b) \
  CTF_INTERNAL_ASSERT(a, b, <=, uintmax_t, "%ju");
#define ctf_assert_ptr_eq(a, b) \
  CTF_INTERNAL_ASSERT(a, b, ==, void *const, "%p");
#define ctf_assert_ptr_neq(a, b) \
  CTF_INTERNAL_ASSERT(a, b, !=, void *const, "%p");
#define ctf_assert_ptr_gt(a, b) CTF_INTERNAL_ASSERT(a, b, >, void *const, "%p");
#define ctf_assert_ptr_lt(a, b) CTF_INTERNAL_ASSERT(a, b, <, void *const, "%p");
#define ctf_assert_ptr_gte(a, b) \
  CTF_INTERNAL_ASSERT(a, b, >=, void *const, "%p");
#define ctf_assert_ptr_lte(a, b) \
  CTF_INTERNAL_ASSERT(a, b, <=, void *const, "%p");
#define ctf_assert_string_eq(a, b) \
  CTF_INTERNAL_ASSERT_FUNC(a, b, ==, char *const, "%s", strcmp);
#define ctf_assert_string_neq(a, b) \
  CTF_INTERNAL_ASSERT_FUNC(a, b, !=, char *const, "%s", strcmp);
#define ctf_assert_string_gt(a, b) \
  CTF_INTERNAL_ASSERT_FUNC(a, b, >, char *const, "%s", strcmp);
#define ctf_assert_string_lt(a, b) \
  CTF_INTERNAL_ASSERT_FUNC(a, b, <, char *const, "%s", strcmp);
#define ctf_assert_string_gte(a, b) \
  CTF_INTERNAL_ASSERT_FUNC(a, b, >=, char *const, "%s", strcmp);
#define ctf_assert_string_lte(a, b) \
  CTF_INTERNAL_ASSERT_FUNC(a, b, <=, char *const, "%s", strcmp);
#define ctf_assert_memory_char_eq(a, b, length) \
  CTF_INTERNAL_ASSERT_MEM_RAW(a, b, ==, char, "%c", length)
#define ctf_assert_memory_char_neq(a, b, length) \
  CTF_INTERNAL_ASSERT_MEM_RAW(a, b, !=, char, "%c", length)
#define ctf_assert_memory_char_lt(a, b, length) \
  CTF_INTERNAL_ASSERT_MEM_RAW(a, b, <, char, "%c", length)
#define ctf_assert_memory_char_gt(a, b, length) \
  CTF_INTERNAL_ASSERT_MEM_RAW(a, b, >, char, "%c", length)
#define ctf_assert_memory_char_lte(a, b, length) \
  CTF_INTERNAL_ASSERT_MEM_RAW(a, b, <=, char, "%c", length)
#define ctf_assert_memory_char_gte(a, b, length) \
  CTF_INTERNAL_ASSERT_MEM_RAW(a, b, >=, char, "%c", length)
#define ctf_assert_memory_int_eq(a, b, length) \
  CTF_INTERNAL_ASSERT_MEM_RAW(a, b, ==, intmax_t, "%jd", length)
#define ctf_assert_memory_int_neq(a, b, length) \
  CTF_INTERNAL_ASSERT_MEM_RAW(a, b, !=, intmax_t, "%jd", length)
#define ctf_assert_memory_int_lt(a, b, length) \
  CTF_INTERNAL_ASSERT_MEM_RAW(a, b, <, intmax_t, "%jd", length)
#define ctf_assert_memory_int_gt(a, b, length) \
  CTF_INTERNAL_ASSERT_MEM_RAW(a, b, >, intmax_t, "%jd", length)
#define ctf_assert_memory_int_lte(a, b, length) \
  CTF_INTERNAL_ASSERT_MEM_RAW(a, b, <=, intmax_t, "%jd", length)
#define ctf_assert_memory_int_gte(a, b, length) \
  CTF_INTERNAL_ASSERT_MEM_RAW(a, b, >=, intmax_t, "%jd", length)
#define ctf_assert_memory_uint_eq(a, b, length) \
  CTF_INTERNAL_ASSERT_MEM_RAW(a, b, ==, uintmax_t, "%ju", length)
#define ctf_assert_memory_uint_neq(a, b, length) \
  CTF_INTERNAL_ASSERT_MEM_RAW(a, b, !=, uintmax_t, "%ju", length)
#define ctf_assert_memory_uint_lt(a, b, length) \
  CTF_INTERNAL_ASSERT_MEM_RAW(a, b, <, uintmax_t, "%ju", length)
#define ctf_assert_memory_uint_gt(a, b, length) \
  CTF_INTERNAL_ASSERT_MEM_RAW(a, b, >, uintmax_t, "%ju", length)
#define ctf_assert_memory_uint_lte(a, b, length) \
  CTF_INTERNAL_ASSERT_MEM_RAW(a, b, <=, uintmax_t, "%ju", length)
#define ctf_assert_memory_uint_gte(a, b, length) \
  CTF_INTERNAL_ASSERT_MEM_RAW(a, b, >=, uintmax_t, "%ju", length)
#define ctf_assert_memory_ptr_eq(a, b, length) \
  CTF_INTERNAL_ASSERT_MEM(a, b, ==, void *const, "%p", length)
#define ctf_assert_memory_ptr_neq(a, b, length) \
  CTF_INTERNAL_ASSERT_MEM(a, b, !=, void *const, "%p", length)
#define ctf_assert_memory_ptr_lt(a, b, length) \
  CTF_INTERNAL_ASSERT_MEM(a, b, <, void *const, "%p", length)
#define ctf_assert_memory_ptr_gt(a, b, length) \
  CTF_INTERNAL_ASSERT_MEM(a, b, >, void *const, "%p", length)
#define ctf_assert_memory_ptr_lte(a, b, length) \
  CTF_INTERNAL_ASSERT_MEM(a, b, <=, void *const, "%p", length)
#define ctf_assert_array_char_eq(a, b) \
  CTF_INTERNAL_ASSERT_ARR(a, b, ==, char, "%c")
#define ctf_assert_array_char_neq(a, b) \
  CTF_INTERNAL_ASSERT_ARR(a, b, !=, char, "%c")
#define ctf_assert_array_char_lt(a, b) \
  CTF_INTERNAL_ASSERT_ARR(a, b, <, char, "%c")
#define ctf_assert_array_char_gt(a, b) \
  CTF_INTERNAL_ASSERT_ARR(a, b, >, char, "%c")
#define ctf_assert_array_char_lte(a, b) \
  CTF_INTERNAL_ASSERT_ARR(a, b, <=, char, "%c")
#define ctf_assert_array_char_gte(a, b) \
  CTF_INTERNAL_ASSERT_ARR(a, b, >=, char, "%c")
#define ctf_assert_array_int_eq(a, b) \
  CTF_INTERNAL_ASSERT_ARR(a, b, ==, intmax_t, "%jd")
#define ctf_assert_array_int_neq(a, b) \
  CTF_INTERNAL_ASSERT_ARR(a, b, !=, intmax_t, "%jd")
#define ctf_assert_array_int_lt(a, b) \
  CTF_INTERNAL_ASSERT_ARR(a, b, <, intmax_t, "%jd")
#define ctf_assert_array_int_gt(a, b) \
  CTF_INTERNAL_ASSERT_ARR(a, b, >, intmax_t, "%jd")
#define ctf_assert_array_int_lte(a, b) \
  CTF_INTERNAL_ASSERT_ARR(a, b, <=, intmax_t, "%jd")
#define ctf_assert_array_int_gte(a, b) \
  CTF_INTERNAL_ASSERT_ARR(a, b, >=, intmax_t, "%jd")
#define ctf_assert_array_uint_eq(a, b) \
  CTF_INTERNAL_ASSERT_ARR(a, b, ==, uintmax_t, "%ju")
#define ctf_assert_array_uint_neq(a, b) \
  CTF_INTERNAL_ASSERT_ARR(a, b, !=, uintmax_t, "%ju")
#define ctf_assert_array_uint_lt(a, b) \
  CTF_INTERNAL_ASSERT_ARR(a, b, <, uintmax_t, "%ju")
#define ctf_assert_array_uint_gt(a, b) \
  CTF_INTERNAL_ASSERT_ARR(a, b, >, uintmax_t, "%ju")
#define ctf_assert_array_uint_lte(a, b) \
  CTF_INTERNAL_ASSERT_ARR(a, b, <=, uintmax_t, "%ju")
#define ctf_assert_array_uint_gte(a, b) \
  CTF_INTERNAL_ASSERT_ARR(a, b, >=, uintmax_t, "%ju")
#define ctf_assert_array_ptr_eq(a, b) \
  CTF_INTERNAL_ASSERT_ARR(a, b, ==, void *const, "%p")
#define ctf_assert_array_ptr_neq(a, b) \
  CTF_INTERNAL_ASSERT_ARR(a, b, !=, void *const, "%p")
#define ctf_assert_array_ptr_lt(a, b) \
  CTF_INTERNAL_ASSERT_ARR(a, b, <, void *const, "%p")
#define ctf_assert_array_ptr_gt(a, b) \
  CTF_INTERNAL_ASSERT_ARR(a, b, >, void *const, "%p")
#define ctf_assert_array_ptr_lte(a, b) \
  CTF_INTERNAL_ASSERT_ARR(a, b, <=, void *const, "%p")
#define ctf_assert_array_ptr_gte(a, b) \
  CTF_INTERNAL_ASSERT_ARR(a, b, >=, void *const, "%p")
#define ctf_assert_true(a)                                               \
  do {                                                                   \
    CTF_INTERNAL_ASSERT_BEGIN;                                           \
    const int _x = a;                                                    \
    err[ctf_internal_next_state].status = !_x;                           \
    snprintf(err[ctf_internal_next_state].msg, CTF_CONST_STATE_MSG_SIZE, \
             "%s == true (%d != 0)", #a, _x);                            \
    CTF_INTERNAL_ASSERT_COPY;                                            \
    CTF_INTERNAL_ASSERT_END;                                             \
  } while(0)
#define ctf_assert_false(a)                                              \
  do {                                                                   \
    CTF_INTERNAL_ASSERT_BEGIN;                                           \
    const int _x = a;                                                    \
    err[ctf_internal_next_state].status = !!_x;                          \
    snprintf(err[ctf_internal_next_state].msg, CTF_CONST_STATE_MSG_SIZE, \
             "%s == false (%d == 0)", #a, _x);                           \
    CTF_INTERNAL_ASSERT_COPY;                                            \
    CTF_INTERNAL_ASSERT_END;                                             \
  } while(0)

#define ctf_expect_msg(test, m)                                             \
  do {                                                                      \
    CTF_INTERNAL_ASSERT_BEGIN;                                              \
    err[ctf_internal_next_state].status = !((int)(test));                   \
    strncpy(err[ctf_internal_next_state].msg, m, CTF_CONST_STATE_MSG_SIZE); \
    CTF_INTERNAL_ASSERT_COPY;                                               \
    CTF_INTERNAL_EXPECT_END;                                                \
  } while(0);
#define ctf_expect(test) ctf_expect_msg(test, CTF_INTERNAL_STRINGIFY2(test))
#define ctf_expect_int_eq(a, b) CTF_INTERNAL_EXPECT(a, b, ==, intmax_t, "%jd");
#define ctf_expect_int_neq(a, b) CTF_INTERNAL_EXPECT(a, b, !=, intmax_t, "%jd");
#define ctf_expect_int_gt(a, b) CTF_INTERNAL_EXPECT(a, b, >, intmax_t, "%jd");
#define ctf_expect_int_lt(a, b) CTF_INTERNAL_EXPECT(a, b, <, intmax_t, "%jd");
#define ctf_expect_int_gte(a, b) CTF_INTERNAL_EXPECT(a, b, >=, intmax_t, "%jd");
#define ctf_expect_int_lte(a, b) CTF_INTERNAL_EXPECT(a, b, <=, intmax_t, "%jd");
#define ctf_expect_uint_eq(a, b) \
  CTF_INTERNAL_EXPECT(a, b, ==, uintmax_t, "%ju");
#define ctf_expect_uint_neq(a, b) \
  CTF_INTERNAL_EXPECT(a, b, !=, uintmax_t, "%ju");
#define ctf_expect_uint_gt(a, b) CTF_INTERNAL_EXPECT(a, b, >, uintmax_t, "%ju");
#define ctf_expect_uint_lt(a, b) CTF_INTERNAL_EXPECT(a, b, <, uintmax_t, "%ju");
#define ctf_expect_uint_gte(a, b) \
  CTF_INTERNAL_EXPECT(a, b, >=, uintmax_t, "%ju");
#define ctf_expect_uint_lte(a, b) \
  CTF_INTERNAL_EXPECT(a, b, <=, uintmax_t, "%ju");
#define ctf_expect_ptr_eq(a, b) \
  CTF_INTERNAL_EXPECT(a, b, ==, void *const, "%p");
#define ctf_expect_ptr_neq(a, b) \
  CTF_INTERNAL_EXPECT(a, b, !=, void *const, "%p");
#define ctf_expect_ptr_gt(a, b) CTF_INTERNAL_EXPECT(a, b, >, void *const, "%p");
#define ctf_expect_ptr_lt(a, b) CTF_INTERNAL_EXPECT(a, b, <, void *const, "%p");
#define ctf_expect_ptr_gte(a, b) \
  CTF_INTERNAL_EXPECT(a, b, >=, void *const, "%p");
#define ctf_expect_ptr_lte(a, b) \
  CTF_INTERNAL_EXPECT(a, b, <=, void *const, "%p");
#define ctf_expect_string_eq(a, b) \
  CTF_INTERNAL_EXPECT_FUNC(a, b, ==, char *const, "%s", strcmp);
#define ctf_expect_string_neq(a, b) \
  CTF_INTERNAL_EXPECT_FUNC(a, b, !=, char *const, "%s", strcmp);
#define ctf_expect_string_gt(a, b) \
  CTF_INTERNAL_EXPECT_FUNC(a, b, >, char *const, "%s", strcmp);
#define ctf_expect_string_lt(a, b) \
  CTF_INTERNAL_EXPECT_FUNC(a, b, <, char *const, "%s", strcmp);
#define ctf_expect_string_gte(a, b) \
  CTF_INTERNAL_EXPECT_FUNC(a, b, >=, char *const, "%s", strcmp);
#define ctf_expect_string_lte(a, b) \
  CTF_INTERNAL_EXPECT_FUNC(a, b, <=, char *const, "%s", strcmp);
#define ctf_expect_memory_int_eq(a, b, length) \
  CTF_INTERNAL_EXPECT_MEM_RAW(a, b, ==, intmax_t, "%jd", length)
#define ctf_expect_memory_int_neq(a, b, length) \
  CTF_INTERNAL_EXPECT_MEM_RAW(a, b, !=, intmax_t, "%jd", length)
#define ctf_expect_memory_int_lt(a, b, length) \
  CTF_INTERNAL_EXPECT_MEM_RAW(a, b, <, intmax_t, "%jd", length)
#define ctf_expect_memory_int_gt(a, b, length) \
  CTF_INTERNAL_EXPECT_MEM_RAW(a, b, >, intmax_t, "%jd", length)
#define ctf_expect_memory_int_lte(a, b, length) \
  CTF_INTERNAL_EXPECT_MEM_RAW(a, b, <=, intmax_t, "%jd", length)
#define ctf_expect_memory_int_gte(a, b, length) \
  CTF_INTERNAL_EXPECT_MEM_RAW(a, b, >=, intmax_t, "%jd", length)
#define ctf_expect_memory_uint_eq(a, b, length) \
  CTF_INTERNAL_EXPECT_MEM_RAW(a, b, ==, uintmax_t, "%ju", length)
#define ctf_expect_memory_uint_neq(a, b, length) \
  CTF_INTERNAL_EXPECT_MEM_RAW(a, b, !=, uintmax_t, "%ju", length)
#define ctf_expect_memory_uint_lt(a, b, length) \
  CTF_INTERNAL_EXPECT_MEM_RAW(a, b, <, uintmax_t, "%ju", length)
#define ctf_expect_memory_uint_gt(a, b, length) \
  CTF_INTERNAL_EXPECT_MEM_RAW(a, b, >, uintmax_t, "%ju", length)
#define ctf_expect_memory_uint_lte(a, b, length) \
  CTF_INTERNAL_EXPECT_MEM_RAW(a, b, <=, uintmax_t, "%ju", length)
#define ctf_expect_memory_uint_gte(a, b, length) \
  CTF_INTERNAL_EXPECT_MEM_RAW(a, b, >=, uintmax_t, "%ju", length)
#define ctf_expect_memory_ptr_eq(a, b, length) \
  CTF_INTERNAL_EXPECT_MEM(a, b, ==, void *const, "%p", length)
#define ctf_expect_memory_ptr_neq(a, b, length) \
  CTF_INTERNAL_EXPECT_MEM(a, b, !=, void *const, "%p", length)
#define ctf_expect_memory_ptr_lt(a, b, length) \
  CTF_INTERNAL_EXPECT_MEM(a, b, <, void *const, "%p", length)
#define ctf_expect_memory_ptr_gt(a, b, length) \
  CTF_INTERNAL_EXPECT_MEM(a, b, >, void *const, "%p", length)
#define ctf_expect_memory_ptr_lte(a, b, length) \
  CTF_INTERNAL_EXPECT_MEM(a, b, <=, void *const, "%p", length)
#define ctf_expect_array_int_eq(a, b) \
  CTF_INTERNAL_EXPECT_ARR(a, b, ==, intmax_t, "%jd")
#define ctf_expect_array_int_neq(a, b) \
  CTF_INTERNAL_EXPECT_ARR(a, b, !=, intmax_t, "%jd")
#define ctf_expect_array_int_lt(a, b) \
  CTF_INTERNAL_EXPECT_ARR(a, b, <, intmax_t, "%jd")
#define ctf_expect_array_int_gt(a, b) \
  CTF_INTERNAL_EXPECT_ARR(a, b, >, intmax_t, "%jd")
#define ctf_expect_array_int_lte(a, b) \
  CTF_INTERNAL_EXPECT_ARR(a, b, <=, intmax_t, "%jd")
#define ctf_expect_array_int_gte(a, b) \
  CTF_INTERNAL_EXPECT_ARR(a, b, >=, intmax_t, "%jd")
#define ctf_expect_array_uint_eq(a, b) \
  CTF_INTERNAL_EXPECT_ARR(a, b, ==, uintmax_t, "%ju")
#define ctf_expect_array_uint_neq(a, b) \
  CTF_INTERNAL_EXPECT_ARR(a, b, !=, uintmax_t, "%ju")
#define ctf_expect_array_uint_lt(a, b) \
  CTF_INTERNAL_EXPECT_ARR(a, b, <, uintmax_t, "%ju")
#define ctf_expect_array_uint_gt(a, b) \
  CTF_INTERNAL_EXPECT_ARR(a, b, >, uintmax_t, "%ju")
#define ctf_expect_array_uint_lte(a, b) \
  CTF_INTERNAL_EXPECT_ARR(a, b, <=, uintmax_t, "%ju")
#define ctf_expect_array_uint_gte(a, b) \
  CTF_INTERNAL_EXPECT_ARR(a, b, >=, uintmax_t, "%ju")
#define ctf_expect_array_ptr_eq(a, b) \
  CTF_INTERNAL_EXPECT_ARR(a, b, ==, void *const, "%p")
#define ctf_expect_array_ptr_neq(a, b) \
  CTF_INTERNAL_EXPECT_ARR(a, b, !=, void *const, "%p")
#define ctf_expect_array_ptr_lt(a, b) \
  CTF_INTERNAL_EXPECT_ARR(a, b, <, void *const, "%p")
#define ctf_expect_array_ptr_gt(a, b) \
  CTF_INTERNAL_EXPECT_ARR(a, b, >, void *const, "%p")
#define ctf_expect_array_ptr_lte(a, b) \
  CTF_INTERNAL_EXPECT_ARR(a, b, <=, void *const, "%p")
#define ctf_expect_array_ptr_gte(a, b) \
  CTF_INTERNAL_EXPECT_ARR(a, b, >=, void *const, "%p")
#define ctf_expect_true(a)                                               \
  do {                                                                   \
    CTF_INTERNAL_ASSERT_BEGIN;                                           \
    const int _x = a;                                                    \
    err[ctf_internal_next_state].status = !_x;                           \
    snprintf(err[ctf_internal_next_state].msg, CTF_CONST_STATE_MSG_SIZE, \
             "%s == true (%d != 0)", #a, _x);                            \
    CTF_INTERNAL_ASSERT_COPY;                                            \
    CTF_INTERNAL_EXPECT_END;                                             \
  } while(0)
#define ctf_expect_false(a)                                              \
  do {                                                                   \
    CTF_INTERNAL_ASSERT_BEGIN;                                           \
    const int _x = a;                                                    \
    err[ctf_internal_next_state].status = _x;                            \
    snprintf(err[ctf_internal_next_state].msg, CTF_CONST_STATE_MSG_SIZE, \
             "%s == false (%d == 0)", #a, _x);                           \
    CTF_INTERNAL_ASSERT_COPY;                                            \
    CTF_INTERNAL_EXPECT_END;                                             \
  } while(0)

#if CTF_ASSERT_ALIASES == CTF_ON
#define assert_msg(test, msg) ctf_assert_msg(test, msg)
#define assert(test) ctf_assert(test)
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
#define assert_memory_int_eq(a, b) ctf_assert_memory_int_eq(a, b)
#define assert_memory_int_neq(a, b) ctf_assert_memory_int_neq(a, b)
#define assert_memory_int_gt(a, b) ctf_assert_memory_int_gt(a, b)
#define assert_memory_int_lt(a, b) ctf_assert_memory_int_lt(a, b)
#define assert_memory_int_gte(a, b) ctf_assert_memory_int_gte(a, b)
#define assert_memory_int_lte(a, b) ctf_assert_memory_int_lte(a, b)
#define assert_memory_uint_eq(a, b) ctf_assert_memory_uint_eq(a, b)
#define assert_memory_uint_neq(a, b) ctf_assert_memory_uint_neq(a, b)
#define assert_memory_uint_gt(a, b) ctf_assert_memory_uint_gt(a, b)
#define assert_memory_uint_lt(a, b) ctf_assert_memory_uint_lt(a, b)
#define assert_memory_uint_gte(a, b) ctf_assert_memory_uint_gte(a, b)
#define assert_memory_uint_lte(a, b) ctf_assert_memory_uint_lte(a, b)
#define assert_memory_ptr_eq(a, b) ctf_assert_memory_ptr_eq(a, b)
#define assert_memory_ptr_neq(a, b) ctf_assert_memory_ptr_neq(a, b)
#define assert_memory_ptr_gt(a, b) ctf_assert_memory_ptr_gt(a, b)
#define assert_memory_ptr_lt(a, b) ctf_assert_memory_ptr_lt(a, b)
#define assert_memory_ptr_gte(a, b) ctf_assert_memory_ptr_gte(a, b)
#define assert_memory_ptr_lte(a, b) ctf_assert_memory_ptr_lte(a, b)
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
#define expect_memory_int_eq(a, b) ctf_expect_memory_int_eq(a, b)
#define expect_memory_int_neq(a, b) ctf_expect_memory_int_neq(a, b)
#define expect_memory_int_gt(a, b) ctf_expect_memory_int_gt(a, b)
#define expect_memory_int_lt(a, b) ctf_expect_memory_int_lt(a, b)
#define expect_memory_int_gte(a, b) ctf_expect_memory_int_gte(a, b)
#define expect_memory_int_lte(a, b) ctf_expect_memory_int_lte(a, b)
#define expect_memory_uint_eq(a, b) ctf_expect_memory_uint_eq(a, b)
#define expect_memory_uint_neq(a, b) ctf_expect_memory_uint_neq(a, b)
#define expect_memory_uint_gt(a, b) ctf_expect_memory_uint_gt(a, b)
#define expect_memory_uint_lt(a, b) ctf_expect_memory_uint_lt(a, b)
#define expect_memory_uint_gte(a, b) ctf_expect_memory_uint_gte(a, b)
#define expect_memory_uint_lte(a, b) ctf_expect_memory_uint_lte(a, b)
#define expect_memory_ptr_eq(a, b) ctf_expect_memory_ptr_eq(a, b)
#define expect_memory_ptr_neq(a, b) ctf_expect_memory_ptr_neq(a, b)
#define expect_memory_ptr_gt(a, b) ctf_expect_memory_ptr_gt(a, b)
#define expect_memory_ptr_lt(a, b) ctf_expect_memory_ptr_lt(a, b)
#define expect_memory_ptr_gte(a, b) ctf_expect_memory_ptr_gte(a, b)
#define expect_memory_ptr_lte(a, b) ctf_expect_memory_ptr_lte(a, b)
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
