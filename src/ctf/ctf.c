#include "ctf.h"

#include <stdarg.h>
#ifdef CTF_PARALLEL
#include <pthread.h>
#endif
#if CTF_COLOR == CTF_AUTO
#include <unistd.h>
#define CTF_INTERNAL_IS_TTY isatty(STDOUT_FILENO)
#endif

/* Const */
#define CTF_CONST_GROUP_NAME_SIZE_DEFAULT 256
#define CTF_CONST_PRINT_BUFF_SIZE_DEFAULT 65536

#ifndef CTF_CONST_GROUP_NAME_SIZE
#define CTF_CONST_GROUP_NAME_SIZE CTF_CONST_GROUP_NAME_SIZE_DEFAULT
#endif
#ifndef CTF_CONST_PRINT_BUFF_SIZE
#define CTF_CONST_PRINT_BUFF_SIZE CTF_CONST_PRINT_BUFF_SIZE_DEFAULT
#endif

#ifdef CTF_PARALLEL
#define CTF_PARALLEL_CONST_TASK_QUEUE_MAX_DEFAULT 64

#ifndef CTF_PARALLEL_CONST_TASK_QUEUE_MAX
#define CTF_PARALLEL_CONST_TASK_QUEUE_MAX \
  CTF_PARALLEL_CONST_TASK_QUEUE_MAX_DEFAULT
#endif
#endif

/* Utility */
#define CTF_INTERNAL_ISSPACE(c) (((c) >= '\t' && (c) <= '\r') || (c) == ' ')
#define CTF_INTERNAL_SKIP_SPACE(c)      \
  do {                                  \
    while(CTF_INTERNAL_ISSPACE(*(c))) { \
      (c)++;                            \
    }                                   \
  } while(0)
#define CTF_INTERNAL_NEXT_ID(c, i)                                           \
  do {                                                                       \
    while(!CTF_INTERNAL_ISSPACE((c)[i]) && (c)[i] != ',' && (c)[i] != ')') { \
      (i)++;                                                                 \
    }                                                                        \
  } while(0)

#define CTF_INTERNAL_SPRINTF_ADVANCE(buff, ...) \
  buff += sprintf(buff, __VA_ARGS__)
#if CTF_COLOR == CTF_AUTO
#define CTF_INTERNAL_COLOR_DISPATCH(macro, buff, ...)                   \
  do {                                                                  \
    if(tty) {                                                           \
      CTF_INTERNAL_SPRINTF_ADVANCE(buff, macro##_COLORED(__VA_ARGS__)); \
    } else {                                                            \
      CTF_INTERNAL_SPRINTF_ADVANCE(buff, macro(__VA_ARGS__));           \
    }                                                                   \
  } while(0)
#elif CTF_COLOR == CTF_ON
#define CTF_INTERNAL_COLOR_DISPATCH(macro, buff, ...) \
  CTF_INTERNAL_SPRINTF_ADVANCE(buff, macro##_COLORED(__VA_ARGS__))
#else
#define CTF_INTERNAL_COLOR_DISPATCH(macro, buff, ...) \
  CTF_INTERNAL_SPRINTF_ADVANCE(buff, macro(__VA_ARGS__))
#endif
#if CTF_DETAIL == CTF_AUTO
#define CTF_INTERNAL_DETAIL_DISPATCH(amacro, macro, ...) \
  do {                                                   \
    if(tty) {                                            \
      amacro(macro, __VA_ARGS__);                        \
    } else {                                             \
      amacro(macro##_DETAIL, __VA_ARGS__);               \
    }                                                    \
  } while(0)
#elif CTF_DETAIL == CTF_ON
#define CTF_INTERNAL_DETAIL_DISPATCH(amacro, macro, ...) \
  amacro(macro##_DETAIL, __VA_ARGS__)
#else
#define CTF_INTERNAL_DETAIL_DISPATCH(amacro, macro, ...) \
  amacro(macro, __VA_ARGS__)
#endif

#if CTF_UNICODE == CTF_UNICODE_GENERIC
#define CTF_PRINT_PASS "✓"
#define CTF_PRINT_FAIL "✗"
#elif CTF_UNICODE == CTF_UNICODE_BRANDED
#define CTF_PRINT_PASS "✓"
#define CTF_PRINT_FAIL "⚑"
#elif CTF_UNICODE == CTF_OFF
#define CTF_PRINT_PASS "P"
#define CTF_PRINT_FAIL "F"
#endif

#define CTF_INTERNAL_PRINT_GROUP_PASS_RAW(name, before, after) \
  "[" before CTF_PRINT_PASS after "] %s\n", (name)
#define CTF_INTERNAL_PRINT_GROUP_FAIL_RAW(name, before, after) \
  "[" before CTF_PRINT_FAIL after "] %s\n", (name)
#define CTF_INTERNAL_PRINT_TEST_PASS_RAW(name, name_len, before, after) \
  "    [" before CTF_PRINT_PASS after "] %.*s\n", (name_len), (name)
#define CTF_INTERNAL_PRINT_TEST_FAIL_HEADER_RAW(name, name_len, before, after) \
  "    [" before CTF_PRINT_FAIL after "] %.*s\n", (name_len), (name)
#define CTF_INTERNAL_PRINT_TEST_PASS_HEADER_RAW(name, name_len, before, after) \
  "    [" before CTF_PRINT_PASS after "] %.*s\n", (name_len), (name)
#define CTF_INTERNAL_PRINT_TEST_PASS_INFO_RAW(state, before, after) \
  "        [" before CTF_PRINT_PASS after "] %s\n", (state).msg
#define CTF_INTERNAL_PRINT_TEST_FAIL_INFO_RAW(state, before, after) \
  "        [" before CTF_PRINT_FAIL after "] %s\n", (state).msg
#define CTF_INTERNAL_PRINT_TEST_PASS_INFO_DETAIL_RAW(state, before, after)   \
  "        %*s[" before CTF_PRINT_PASS after "] %s\n",                       \
    ((int)strlen((state).file) + 2 + ctf_internal_int_length((state).line)), \
    "", (state).msg
#define CTF_INTERNAL_PRINT_TEST_FAIL_INFO_DETAIL_RAW(state, before, after) \
  "        [%s|%d|" before "E" after "] %s\n", (state).file, (state).line, \
    (state).msg

#define CTF_INTERNAL_PRINT_GROUP_PASS(name) \
  CTF_INTERNAL_PRINT_GROUP_PASS_RAW(name, "", "")
#define CTF_INTERNAL_PRINT_GROUP_FAIL(name) \
  CTF_INTERNAL_PRINT_GROUP_FAIL_RAW(name, "", "")
#define CTF_INTERNAL_PRINT_TEST_PASS(name, name_len) \
  CTF_INTERNAL_PRINT_TEST_PASS_RAW(name, name_len, "", "")
#define CTF_INTERNAL_PRINT_TEST_FAIL_HEADER(name, name_len) \
  CTF_INTERNAL_PRINT_TEST_FAIL_HEADER_RAW(name, name_len, "", "")
#define CTF_INTERNAL_PRINT_TEST_PASS_HEADER(name, name_len) \
  CTF_INTERNAL_PRINT_TEST_PASS_HEADER_RAW(name, name_len, "", "")
#define CTF_INTERNAL_PRINT_TEST_PASS_INFO(state) \
  CTF_INTERNAL_PRINT_TEST_PASS_INFO_RAW(state, "", "")
#define CTF_INTERNAL_PRINT_TEST_FAIL_INFO(state) \
  CTF_INTERNAL_PRINT_TEST_FAIL_INFO_RAW(state, "", "")
#define CTF_INTERNAL_PRINT_TEST_PASS_INFO_DETAIL(state) \
  CTF_INTERNAL_PRINT_TEST_PASS_INFO_DETAIL_RAW(state, "", "")
#define CTF_INTERNAL_PRINT_TEST_FAIL_INFO_DETAIL(state) \
  CTF_INTERNAL_PRINT_TEST_FAIL_INFO_DETAIL_RAW(state, "", "")

#if CTF_COLOR != CTF_OFF
#define CTF_INTERNAL_PRINT_COLOR_GREEN "\x1b[32m"
#define CTF_INTERNAL_PRINT_COLOR_RED "\x1b[31m"
#define CTF_INTERNAL_PRINT_COLOR_RESET "\x1b[0m"
#define CTF_INTERNAL_PRINT_GROUP_PASS_COLORED(name)                       \
  CTF_INTERNAL_PRINT_GROUP_PASS_RAW(name, CTF_INTERNAL_PRINT_COLOR_GREEN, \
                                    CTF_INTERNAL_PRINT_COLOR_RESET)
#define CTF_INTERNAL_PRINT_GROUP_FAIL_COLORED(name)                     \
  CTF_INTERNAL_PRINT_GROUP_FAIL_RAW(name, CTF_INTERNAL_PRINT_COLOR_RED, \
                                    CTF_INTERNAL_PRINT_COLOR_RESET)
#define CTF_INTERNAL_PRINT_TEST_PASS_COLORED(name, name_len)       \
  CTF_INTERNAL_PRINT_TEST_PASS_RAW(name, name_len,                 \
                                   CTF_INTERNAL_PRINT_COLOR_GREEN, \
                                   CTF_INTERNAL_PRINT_COLOR_RESET)
#define CTF_INTERNAL_PRINT_TEST_FAIL_HEADER_COLORED(name, name_len)     \
  CTF_INTERNAL_PRINT_TEST_FAIL_HEADER_RAW(name, name_len,               \
                                          CTF_INTERNAL_PRINT_COLOR_RED, \
                                          CTF_INTERNAL_PRINT_COLOR_RESET)
#define CTF_INTERNAL_PRINT_TEST_PASS_HEADER_COLORED(name, name_len)       \
  CTF_INTERNAL_PRINT_TEST_PASS_HEADER_RAW(name, name_len,                 \
                                          CTF_INTERNAL_PRINT_COLOR_GREEN, \
                                          CTF_INTERNAL_PRINT_COLOR_RESET)
#define CTF_INTERNAL_PRINT_TEST_PASS_INFO_COLORED(state)                       \
  CTF_INTERNAL_PRINT_TEST_PASS_INFO_RAW(state, CTF_INTERNAL_PRINT_COLOR_GREEN, \
                                        CTF_INTERNAL_PRINT_COLOR_RESET)
#define CTF_INTERNAL_PRINT_TEST_FAIL_INFO_COLORED(state)                     \
  CTF_INTERNAL_PRINT_TEST_FAIL_INFO_RAW(state, CTF_INTERNAL_PRINT_COLOR_RED, \
                                        CTF_INTERNAL_PRINT_COLOR_RESET)
#define CTF_INTERNAL_PRINT_TEST_PASS_INFO_DETAIL_COLORED(state) \
  CTF_INTERNAL_PRINT_TEST_PASS_INFO_DETAIL_RAW(                 \
    state, CTF_INTERNAL_PRINT_COLOR_GREEN, CTF_INTERNAL_PRINT_COLOR_RESET)
#define CTF_INTERNAL_PRINT_TEST_FAIL_INFO_DETAIL_COLORED(state) \
  CTF_INTERNAL_PRINT_TEST_FAIL_INFO_DETAIL_RAW(                 \
    state, CTF_INTERNAL_PRINT_COLOR_RED, CTF_INTERNAL_PRINT_COLOR_RESET)
#endif

/* Variables */
int ctf_exit_code = 0;

#ifdef CTF_PARALLEL
static pthread_mutex_t ctf_parallel_internal_print_mutex =
  PTHREAD_MUTEX_INITIALIZER;
pthread_t ctf_parallel_internal_threads[CTF_PARALLEL];
static int ctf_parallel_internal_threads_waiting = 0;
static const struct ctf_internal_group
  *ctf_parallel_internal_task_queue[CTF_PARALLEL_CONST_TASK_QUEUE_MAX] = {0};
static pthread_mutex_t ctf_parallel_internal_task_queue_mutex =
  PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t ctf_parallel_internal_threads_waiting_all =
  PTHREAD_COND_INITIALIZER;
static pthread_cond_t ctf_parallel_internal_task_queue_non_empty =
  PTHREAD_COND_INITIALIZER;
static pthread_cond_t ctf_parallel_internal_task_queue_non_full =
  PTHREAD_COND_INITIALIZER;
int ctf_parallel_internal_state = 0;
struct ctf_internal_state
  ctf_parallel_internal_states[CTF_PARALLEL][CTF_CONST_STATES_PER_THREAD];
int ctf_parallel_internal_states_index[CTF_PARALLEL];
#else
struct ctf_internal_state ctf_internal_states[CTF_CONST_STATES_PER_THREAD];
int ctf_internal_state_index = 0;
#endif

/* Functions */
void ctf_internal_group_run_helper(const struct ctf_internal_group *group,
                                   char *print_buff) {
#if CTF_COLOR == CTF_AUTO
  const int tty = CTF_INTERNAL_IS_TTY;
#endif
#ifdef CTF_PARALLEL
  CTF_PARALLEL_INTERNAL_THREAD_INDEX;
#endif
  char *print_buff_p = print_buff;
  int test_passed;
  int all_passed = 1;
  const char *test_name = group->test_names;
  int test_name_len = 1;
  CTF_INTERNAL_COLOR_DISPATCH(CTF_INTERNAL_PRINT_GROUP_FAIL, print_buff_p,
                              group->name);
  for(int i = 0; i < group->length; i++) {
    test_passed = 1;
    CTF_INTERNAL_STATE_INDEX_UPDATE(0);
    group->tests[i]();
#ifdef CTF_PARALLEL
    CTF_PARALLEL_INTERNAL_STATE_INDEX_REFRESH;
#endif
    CTF_INTERNAL_SKIP_SPACE(test_name);
    test_name++; /* skips paren */
    CTF_INTERNAL_SKIP_SPACE(test_name);
    test_name_len = 1;
    CTF_INTERNAL_NEXT_ID(test_name, test_name_len);
    for(int j = 0; j < ctf_internal_state_index; j++) {
      if(ctf_internal_states[j].status == 1) {
        test_passed = 0;
        all_passed = 0;
        break;
      }
    }
    if(!test_passed) {
      CTF_INTERNAL_COLOR_DISPATCH(CTF_INTERNAL_PRINT_TEST_FAIL_HEADER,
                                  print_buff_p, test_name, test_name_len);
      for(int j = 0; j < ctf_internal_state_index; j++) {
        if(ctf_internal_states[j].status == 0) {
          CTF_INTERNAL_DETAIL_DISPATCH(CTF_INTERNAL_COLOR_DISPATCH,
                                       CTF_INTERNAL_PRINT_TEST_PASS_INFO,
                                       print_buff_p, ctf_internal_states[j]);
        } else if(ctf_internal_states[j].status == 1) {
          CTF_INTERNAL_DETAIL_DISPATCH(CTF_INTERNAL_COLOR_DISPATCH,
                                       CTF_INTERNAL_PRINT_TEST_FAIL_INFO,
                                       print_buff_p, ctf_internal_states[j]);
        }
      }
    } else {
      CTF_INTERNAL_COLOR_DISPATCH(CTF_INTERNAL_PRINT_TEST_PASS_HEADER,
                                  print_buff_p, test_name, test_name_len);
    }
    test_name += test_name_len;
  }
  if(all_passed) {
    CTF_INTERNAL_COLOR_DISPATCH(CTF_INTERNAL_PRINT_GROUP_PASS, print_buff,
                                group->name);
  } else {
    ctf_exit_code = 1;
  }
}
void ctf_internal_group_run(const struct ctf_internal_group *group) {
  char print_buff[CTF_CONST_PRINT_BUFF_SIZE];
  ctf_internal_group_run_helper(group, print_buff);
  printf("%s", print_buff);
  fflush(stdout);
}
void ctf_internal_groups_run(int count, ...) {
  char print_buff[CTF_CONST_PRINT_BUFF_SIZE];
  va_list args;
  va_start(args, count);
  for(int i = 0; i < count; i++) {
    ctf_internal_group_run_helper(
      va_arg(args, const struct ctf_internal_group *), print_buff);
    printf("%s", print_buff);
  }
  fflush(stdout);
  va_end(args);
}

#if CTF_DETAIL != CTF_OFF
int ctf_internal_int_length(int a) {
  int counter = 1;
  int i = 1;
  while(i <= a) {
    i *= 10;
    counter++;
  }
  return counter - 1;
}
#endif

#ifdef CTF_PARALLEL
static void *ctf_parallel_internal_thread_loop(void *data) {
  (void)data;
  const struct ctf_internal_group *group;
  char print_buff[CTF_CONST_PRINT_BUFF_SIZE];
  while(1) {
    pthread_mutex_lock(&ctf_parallel_internal_task_queue_mutex);
    if(ctf_parallel_internal_task_queue[0] == NULL) {
      if(ctf_parallel_internal_threads_waiting == CTF_PARALLEL - 1)
        pthread_cond_signal(&ctf_parallel_internal_threads_waiting_all);
      ctf_parallel_internal_threads_waiting++;
      while(ctf_parallel_internal_task_queue[0] == NULL &&
            ctf_parallel_internal_state) {
        pthread_cond_wait(&ctf_parallel_internal_task_queue_non_empty,
                          &ctf_parallel_internal_task_queue_mutex);
      }
      ctf_parallel_internal_threads_waiting--;
      if(ctf_parallel_internal_task_queue[0] == NULL &&
         !ctf_parallel_internal_state) {
        pthread_mutex_unlock(&ctf_parallel_internal_task_queue_mutex);
        return NULL;
      }
    }
    group = ctf_parallel_internal_task_queue[0];
    for(int i = 0; i < CTF_PARALLEL_CONST_TASK_QUEUE_MAX - 1 &&
                   ctf_parallel_internal_task_queue[i] != NULL;
        i++) {
      ctf_parallel_internal_task_queue[i] =
        ctf_parallel_internal_task_queue[i + 1];
    }
    pthread_cond_signal(&ctf_parallel_internal_task_queue_non_full);
    pthread_mutex_unlock(&ctf_parallel_internal_task_queue_mutex);
    ctf_internal_group_run_helper(group, print_buff);
    pthread_mutex_lock(&ctf_parallel_internal_print_mutex);
    printf("%s", print_buff);
    fflush(stdout);
    pthread_mutex_unlock(&ctf_parallel_internal_print_mutex);
  }
}
void ctf_parallel_internal_group_run(const struct ctf_internal_group *group) {
  pthread_mutex_lock(&ctf_parallel_internal_task_queue_mutex);
  while(
    ctf_parallel_internal_task_queue[CTF_PARALLEL_CONST_TASK_QUEUE_MAX - 1] !=
    NULL) {
    pthread_cond_wait(&ctf_parallel_internal_task_queue_non_full,
                      &ctf_parallel_internal_task_queue_mutex);
  }
  for(int i = 0; i < CTF_PARALLEL_CONST_TASK_QUEUE_MAX; i++) {
    if(ctf_parallel_internal_task_queue[i] == NULL) {
      ctf_parallel_internal_task_queue[i] = group;
      break;
    }
  }
  pthread_cond_signal(&ctf_parallel_internal_task_queue_non_empty);
  pthread_mutex_unlock(&ctf_parallel_internal_task_queue_mutex);
}
void ctf_parallel_internal_groups_run(int count, ...) {
  va_list args;
  const struct ctf_internal_group *group;

  va_start(args, count);
  pthread_mutex_lock(&ctf_parallel_internal_task_queue_mutex);
  for(int j = 0; j < count; j++) {
    group = va_arg(args, const struct ctf_internal_group *);
    while(
      ctf_parallel_internal_task_queue[CTF_PARALLEL_CONST_TASK_QUEUE_MAX - 1] !=
      NULL) {
      pthread_cond_wait(&ctf_parallel_internal_task_queue_non_full,
                        &ctf_parallel_internal_task_queue_mutex);
    }
    for(int i = 0; i < CTF_PARALLEL_CONST_TASK_QUEUE_MAX; i++) {
      if(ctf_parallel_internal_task_queue[i] == NULL) {
        ctf_parallel_internal_task_queue[i] = group;
        break;
      }
    }
  }
  va_end(args);
  pthread_cond_broadcast(&ctf_parallel_internal_task_queue_non_empty);
  pthread_mutex_unlock(&ctf_parallel_internal_task_queue_mutex);
}
int ctf_parallel_internal_thread_index(int *state_index,
                                       struct ctf_internal_state **states) {
  const pthread_t thread = pthread_self();
  for(int i = 0; i < CTF_PARALLEL; i++) {
    if(pthread_equal(ctf_parallel_internal_threads[i], thread)) {
      *state_index = ctf_parallel_internal_states_index[i];
      *states = ctf_parallel_internal_states[i];
      return i;
    }
  }
  return -1;
}
void ctf_parallel_sync(void) {
  pthread_mutex_lock(&ctf_parallel_internal_task_queue_mutex);
  while(ctf_parallel_internal_threads_waiting != CTF_PARALLEL ||
        ctf_parallel_internal_task_queue[0] != NULL) {
    pthread_cond_wait(&ctf_parallel_internal_threads_waiting_all,
                      &ctf_parallel_internal_task_queue_mutex);
  }
  fflush(stdout);
  pthread_mutex_unlock(&ctf_parallel_internal_task_queue_mutex);
}
void ctf_parallel_start(void) {
  ctf_parallel_internal_state = 1;
  for(int i = 0; i < CTF_PARALLEL; i++) {
    pthread_create(ctf_parallel_internal_threads + i, NULL,
                   ctf_parallel_internal_thread_loop, NULL);
  }
}
void ctf_parallel_stop(void) {
  ctf_parallel_internal_state = 0;
  pthread_mutex_lock(&ctf_parallel_internal_task_queue_mutex);
  pthread_cond_broadcast(&ctf_parallel_internal_task_queue_non_empty);
  pthread_mutex_unlock(&ctf_parallel_internal_task_queue_mutex);
  for(int i = 0; i < CTF_PARALLEL; i++) {
    pthread_join(ctf_parallel_internal_threads[i], NULL);
  }
}
#endif
