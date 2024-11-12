#include "ctf.h"

#ifndef CTF_NO_SIGNAL
#include <signal.h>
#endif
#include <pthread.h>
#include <stdarg.h>
#include <stdlib.h>
#include <unistd.h>
#define IS_TTY !isatty(STDOUT_FILENO)

#define HELP_MSG                                                             \
  "Run tests embedded in this executable.\n"                                 \
  "\n"                                                                       \
  "Options:\n"                                                               \
  "-h, --help     Show this message\n"                                       \
  "-u, --unicode  (off|generic|branded*) display of unicode symbols\n"       \
  "-c, --color    (off|on|auto*) color coding for failed and passed tests\n" \
  "-d, --detail   (off|on|auto*) detailed info about failed tests\n"         \
  "-f, --failed   Print only groups that failed\n"                           \
  "-j, --jobs     Number of parallel threads to run (default 1)\n"

#define OFF 0
#define ON 1
#define AUTO 2
#define GENERIC 1
#define BRANDED 2

#define PRINT_BUFF_SIZE 65536
#define TASK_QUEUE_MAX 64
#define MAX_THREADS 128
#define SIGNAL_STACK_SIZE 1024

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
#define MIN(a, b) ((a) < (b) ? (a) : (b))

static int opt_unicode = BRANDED;
static int opt_color = AUTO;
static int opt_detail = AUTO;
static int opt_failed = OFF;
static int opt_threads = 1;
static int tty_present = 0;
static const char print_color_reset[] = "\x1b[0m";
static int color = 0;
static int detail = 1;

int ctf_exit_code = 0;

static char signal_stack[SIGNAL_STACK_SIZE];
static pthread_mutex_t parallel_print_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_t parallel_threads[MAX_THREADS];
static int parallel_threads_waiting = 0;
static const struct ctf_internal_group *parallel_task_queue[TASK_QUEUE_MAX] = {
  0};
static pthread_mutex_t parallel_task_queue_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t parallel_threads_waiting_all = PTHREAD_COND_INITIALIZER;
static pthread_cond_t parallel_task_queue_non_empty = PTHREAD_COND_INITIALIZER;
static pthread_cond_t parallel_task_queue_non_full = PTHREAD_COND_INITIALIZER;
static int parallel_state = 0;
static struct ctf_internal_state parallel_states[MAX_THREADS]
                                                [CTF_CONST_STATES_PER_THREAD];
static int parallel_states_index[MAX_THREADS];
static CTF_INTERNAL_PARALLEL_THREAD_LOCAL int parallel_thread_index;
static struct ctf_internal_state states[CTF_CONST_STATES_PER_THREAD];
static char print_buff[MAX_THREADS][PRINT_BUFF_SIZE];

CTF_INTERNAL_PARALLEL_THREAD_LOCAL struct ctf_internal_state
  *ctf_internal_states = states;
CTF_INTERNAL_PARALLEL_THREAD_LOCAL int ctf_internal_state_index = 0;

static int get_value(const char *opt) {
  if(!strcmp(opt, "on")) return ON;
  if(!strcmp(opt, "off")) return OFF;
  if(!strcmp(opt, "auto")) return AUTO;
  puts(HELP_MSG);
  exit(0);
}

static void err(void) {
  puts(HELP_MSG);
  exit(0);
}

static int int_length(int a) {
  int counter = 1;
  int i = 1;
  while(i <= a) {
    i *= 10;
    counter++;
  }
  return counter - 1;
}

static size_t print_int(char *buff, size_t buff_len, int a) {
  int radix = 1;
  size_t index = 0;
  while(radix * 10 <= a) {
    radix *= 10;
  }
  while(a > 0) {
    if(index == buff_len) return index;
    buff[index++] = a / radix + '0';
    a %= radix;
    radix /= 10;
  }
  return index;
}

static unsigned print_pass_indicator(char *arr, int len) {
  static const char print_pass_color[] = "\x1b[32m";
  static const char print_pass_branded[] = "✓";
  static const char print_pass_generic[] = "✓";
  static const char print_pass_off[] = "P";
  int index = 0;
  arr[index++] = '[';
  if(color) {
    strlcpy(arr + index, print_pass_color, len - index);
    index += sizeof(print_pass_color) - 1;
  }
  if(opt_unicode == OFF) {
    strlcpy(arr + index, print_pass_off, len - index);
    index += sizeof(print_pass_off) - 1;
  } else if(opt_unicode == GENERIC) {
    strlcpy(arr + index, print_pass_generic, len - index);
    index += sizeof(print_pass_generic) - 1;
  } else if(opt_unicode == BRANDED) {
    strlcpy(arr + index, print_pass_branded, len - index);
    index += sizeof(print_pass_branded) - 1;
  }
  if(color) {
    strlcpy(arr + index, print_color_reset, len - index);
    index += sizeof(print_color_reset) - 1;
  }
  if(index < len) {
    arr[index++] = ']';
    arr[index] = 0;
  } else {
    index = -1;
  }
  return index;
}

static unsigned print_fail_indicator(char *arr, int len) {
  static const char print_fail_color[] = "\x1b[31m";
  static const char print_fail_branded[] = "⚑";
  static const char print_fail_generic[] = "✗";
  static const char print_fail_off[] = "F";
  int index = 0;
  arr[index++] = '[';
  if(color) {
    strlcpy(arr + index, print_fail_color, len - index);
    index += sizeof(print_fail_color) - 1;
  }
  if(opt_unicode == OFF) {
    strlcpy(arr + index, print_fail_off, len - index);
    index += sizeof(print_fail_off) - 1;
  } else if(opt_unicode == GENERIC) {
    strlcpy(arr + index, print_fail_generic, len - index);
    index += sizeof(print_fail_generic) - 1;
  } else if(opt_unicode == BRANDED) {
    strlcpy(arr + index, print_fail_branded, len - index);
    index += sizeof(print_fail_branded) - 1;
  }
  if(color) {
    strlcpy(arr + index, print_color_reset, len - index);
    index += sizeof(print_color_reset) - 1;
  }
  if(index < len) {
    arr[index++] = ']';
    arr[index] = 0;
  } else {
    index = -1;
  }
  return index;
}

static unsigned print_unknown_indicator(char *arr, int len) {
  static const char print_unknown_color[] = "\x1b[33m";
  // Padding required in order to replace it with fail indicators easily
  static const char print_unknown_branded[] = "\x1a\x1a?";
  static const char print_unknown_generic[] = "\x1a\x1a?";
  static const char print_unknown_off[] = "?";
  int index = 0;
  arr[index++] = '[';
  if(color) {
    strlcpy(arr + index, print_unknown_color, len - index);
    index += sizeof(print_unknown_color) - 1;
  }
  if(opt_unicode == OFF) {
    strlcpy(arr + index, print_unknown_off, len - index);
    index += sizeof(print_unknown_off) - 1;
  } else if(opt_unicode == GENERIC) {
    strlcpy(arr + index, print_unknown_generic, len - index);
    index += sizeof(print_unknown_generic) - 1;
  } else if(opt_unicode == BRANDED) {
    strlcpy(arr + index, print_unknown_branded, len - index);
    index += sizeof(print_unknown_branded) - 1;
  }
  if(color) {
    strlcpy(arr + index, print_color_reset, len - index);
    index += sizeof(print_color_reset) - 1;
  }
  if(index < len) {
    arr[index++] = ']';
    arr[index] = 0;
  } else {
    index = -1;
  }
  return index;
}

static unsigned print_name(char *buff, unsigned buff_len, const char *name,
                           unsigned len) {
  unsigned index = 0;
  if(index + 3 + len >= buff_len) return index;
  buff[index++] = ' ';
  strlcpy(buff + index, name, index + len);
  index += len;
  buff[index++] = '\n';
  return index;
}

static unsigned print_group_pass(char *buff, unsigned buff_len,
                                 const char *name, unsigned len) {
  unsigned index;
  if(opt_failed == OFF) {
    index = print_pass_indicator(buff, buff_len);
    index += print_name(buff + index, buff_len - index, name, len);
    buff[index] = 0;
    return index;
  } else {
    buff[0] = 0;
    return 0;
  }
}

static unsigned print_group_fail(char *buff, unsigned buff_len,
                                 const char *name, unsigned len) {
  unsigned index;
  index = print_fail_indicator(buff, buff_len);
  index += print_name(buff + index, buff_len - index, name, len);
  return index;
}

static unsigned print_group_unknown(char *buff, unsigned buff_len,
                                    const char *name, unsigned len) {
  unsigned index;
  index = print_unknown_indicator(buff, buff_len);
  index += print_name(buff + index, buff_len - index, name, len);
  buff[index] = 0;
  return index;
}

static unsigned print_test_pass(char *buff, unsigned buff_len, const char *name,
                                unsigned len) {
  unsigned index = 0;
  if(index + 4 > buff_len) return index;
  strlcpy(buff, "    ", buff_len);
  index += sizeof("    ") - 1;
  index += print_pass_indicator(buff + index, buff_len - index);
  index += print_name(buff + index, buff_len - index, name, len);
  buff[index] = 0;
  return index;
}

static unsigned print_test_fail(char *buff, unsigned buff_len, const char *name,
                                unsigned len) {
  unsigned index = 0;
  if(index + 4 > buff_len) return index;
  strlcpy(buff, "    ", buff_len);
  index += sizeof("    ") - 1;
  index += print_fail_indicator(buff + index, buff_len - index);
  index += print_name(buff + index, buff_len - index, name, len);
  buff[index] = 0;
  return index;
}

static unsigned print_test_unknown(char *buff, unsigned buff_len,
                                   const char *name, unsigned len) {
  unsigned index = 0;
  if(index + 4 > buff_len) return index;
  strlcpy(buff, "    ", buff_len);
  index += sizeof("    ") - 1;
  index += print_unknown_indicator(buff + index, buff_len - index);
  index += print_name(buff + index, buff_len - index, name, len);
  buff[index] = 0;
  return index;
}

static unsigned print_test_pass_info(char *buff, unsigned buff_len,
                                     const struct ctf_internal_state *state) {
  unsigned index = 0;
  unsigned spaces = 8;
  if(detail) {
    spaces += strlen(state->file) + 2 + int_length(state->line);
  }
  if(index + spaces > buff_len) return index;
  while(index < spaces) {
    buff[index++] = ' ';
  }
  index += print_pass_indicator(buff + index, buff_len - index);
  index +=
    print_name(buff + index, buff_len - index, state->msg, strlen(state->msg));
  buff[index] = 0;
  return index;
}

static unsigned print_test_fail_info(char *buff, unsigned buff_len,
                                     const struct ctf_internal_state *state) {
  unsigned index = 0;
  if(index + 8 > buff_len) return index;
  while(index < 8) {
    buff[index++] = ' ';
  }
  if(detail) {
    if(index + strlen(state->file) + 2 + int_length(state->line) >= buff_len)
      return index;
    index += snprintf(buff + index, buff_len - index, "[%s|%d|", state->file,
                      state->line);
    if(color) {
      strlcpy(buff + index, "\x1b[31mE\x1b[0m", buff_len - index);
      index += sizeof("\x1b[31mE\x1b[0m") - 1;
    } else {
      buff[index++] = 'E';
    }
    buff[index++] = ']';
    buff[index++] = ' ';
    strlcpy(buff + index, state->msg, buff_len - index);
    index += strlen(state->msg);
    buff[index++] = '\n';
    buff[index] = 0;
  } else {
    index += print_fail_indicator(buff + index, buff_len - index);
    index += print_name(buff + index, buff_len - index, state->msg,
                        strlen(state->msg));
    buff[index] = 0;
  }
  return index;
}

static unsigned print_test_unknown_info(
  char *buff, unsigned buff_len, const struct ctf_internal_state *state) {
  unsigned index = 0;
  if(index + 8 > buff_len) return index;
  while(index < 8) {
    buff[index++] = ' ';
  }
  if(detail) {
    if(index + strlen(state->file) + 2 + int_length(state->line) >= buff_len)
      return index;
    buff[index++] = '[';
    strlcpy(buff + index, state->file, buff_len - index);
    index += strlen(state->file);
    buff[index++] = '|';
    index += print_int(buff + index, buff_len - index, state->line);
    buff[index++] = '|';
    if(color) {
      strlcpy(buff + index, "\x1b[33mW\x1b[0m", buff_len - index);
      index += sizeof("\x1b[33mW\x1b[0m") - 1;
    } else {
      buff[index++] = 'W';
    }
    buff[index++] = ']';
    buff[index++] = ' ';
    strlcpy(buff + index, state->msg, buff_len - index);
    index += strlen(state->msg);
    buff[index++] = '\n';
    buff[index] = 0;
  } else {
    index += print_unknown_indicator(buff + index, buff_len - index);
    index += print_name(buff + index, buff_len - index, state->msg,
                        strlen(state->msg));
    buff[index] = 0;
  }
  return index;
}

static size_t print_arr(struct ctf_internal_state *state, size_t index,
                        const void *data, size_t size, size_t step, int sign,
                        const char *format) {
  struct {
    union {
      const int8_t *i8;
      const int16_t *i16;
      const int32_t *i32;
      const int64_t *i64;
      const uint8_t *u8;
      const uint16_t *u16;
      const uint32_t *u32;
      const uint64_t *u64;
    };
  } iterator;
  iterator.u8 = data;
  switch(step) {
  case 1:
    if(sign) {
      for(size_t i = 0; i < size; i++) {
        index += snprintf(state->msg + index, CTF_CONST_STATE_MSG_SIZE - index,
                          format, (intmax_t)iterator.i8[i]);
      }
    } else {
      for(size_t i = 0; i < size; i++) {
        index += snprintf(state->msg + index, CTF_CONST_STATE_MSG_SIZE - index,
                          format, (uintmax_t)iterator.u8[i]);
      }
    }
    break;
  case 2:
    if(sign) {
      for(size_t i = 0; i < size; i++) {
        index += snprintf(state->msg + index, CTF_CONST_STATE_MSG_SIZE - index,
                          format, (intmax_t)iterator.i16[i]);
      }
    } else {
      for(size_t i = 0; i < size; i++) {
        index += snprintf(state->msg + index, CTF_CONST_STATE_MSG_SIZE - index,
                          format, (uintmax_t)iterator.u16[i]);
      }
    }
    break;
  case 4:
    if(sign) {
      for(size_t i = 0; i < size; i++) {
        index += snprintf(state->msg + index, CTF_CONST_STATE_MSG_SIZE - index,
                          format, (intmax_t)iterator.i32[i]);
      }
    } else {
      for(size_t i = 0; i < size; i++) {
        index += snprintf(state->msg + index, CTF_CONST_STATE_MSG_SIZE - index,
                          format, (uintmax_t)iterator.u32[i]);
      }
    }
    break;
  case 8:
    if(sign) {
      for(size_t i = 0; i < size; i++) {
        index += snprintf(state->msg + index, CTF_CONST_STATE_MSG_SIZE - index,
                          format, (intmax_t)iterator.i64[i]);
      }
    } else {
      for(size_t i = 0; i < size; i++) {
        index += snprintf(state->msg + index, CTF_CONST_STATE_MSG_SIZE - index,
                          format, (uintmax_t)iterator.u64[i]);
      }
    }
    break;
  }
  index -= 2;
  return index;
}

static size_t print_mem(struct ctf_internal_state *state, const void *a,
                        const void *b, size_t la, size_t lb, size_t step,
                        int sign, const char *a_str, const char *b_str,
                        const char *op_str, const char *format, int line,
                        const char *file) {
  size_t index;
  int status = memcmp(a, b, MIN(la, lb) * step);
  index = snprintf(state->msg, CTF_CONST_STATE_MSG_SIZE, "%s %s %s ({", a_str,
                   op_str, b_str);
  index = print_arr(state, index, a, la, step, sign, format);
  index += snprintf(state->msg + index, CTF_CONST_STATE_MSG_SIZE - index,
                    "} %s {", op_str);
  index = print_arr(state, index, a, la, step, sign, format);
  snprintf(state->msg + index, CTF_CONST_STATE_MSG_SIZE - index, "})");
  state->status = status;
  return index;
}

static void group_run_helper(const struct ctf_internal_group *group,
                             char *buff) {
  unsigned buff_index = 0;
  int test_passed;
  int all_passed = 1;
  const char *test_name = group->test_names;
  int test_name_len = 1;
  buff_index +=
    print_group_unknown(buff + buff_index, PRINT_BUFF_SIZE - buff_index,
                        group->name, strlen(group->name));
  for(int i = 0; i < group->length; i++) {
    test_passed = 1;
    if(opt_threads != 1) {
      parallel_states_index[parallel_thread_index] = 0;
    }
    ctf_internal_state_index = 0;
    CTF_INTERNAL_SKIP_SPACE(test_name);
    test_name++; /* skips paren */
    CTF_INTERNAL_SKIP_SPACE(test_name);
    test_name_len = 1;
    CTF_INTERNAL_NEXT_ID(test_name, test_name_len);
    print_test_unknown(buff + buff_index, PRINT_BUFF_SIZE - buff_index,
                       test_name, test_name_len);
    group->tests[i]();
    if(opt_threads != 1) {
      ctf_internal_state_index = parallel_states_index[parallel_thread_index];
    }
    for(int j = 0; j < ctf_internal_state_index; j++) {
      if(ctf_internal_states[j].status == 1) {
        test_passed = 0;
        all_passed = 0;
        break;
      }
    }
    if(!test_passed) {
      buff_index +=
        print_test_fail(buff + buff_index, PRINT_BUFF_SIZE - buff_index,
                        test_name, test_name_len);
      for(int j = 0; j < ctf_internal_state_index; j++) {
        if(ctf_internal_states[j].status == 0) {
          buff_index += print_test_pass_info(buff + buff_index,
                                             PRINT_BUFF_SIZE - buff_index,
                                             ctf_internal_states + j);
        } else if(ctf_internal_states[j].status == 1) {
          buff_index += print_test_fail_info(buff + buff_index,
                                             PRINT_BUFF_SIZE - buff_index,
                                             ctf_internal_states + j);
        }
      }
    } else {
      buff_index +=
        print_test_pass(buff + buff_index, PRINT_BUFF_SIZE - buff_index,
                        test_name, test_name_len);
    }
    test_name += test_name_len;
  }
  if(all_passed) {
    print_group_pass(buff, PRINT_BUFF_SIZE, group->name, strlen(group->name));
  } else {
    print_group_fail(buff, PRINT_BUFF_SIZE, group->name, strlen(group->name));
    ctf_exit_code = 1;
  }
}

static void group_run(const struct ctf_internal_group *group) {
  group_run_helper(group, print_buff[parallel_thread_index]);
  printf("%s", print_buff[parallel_thread_index]);
  fflush(stdout);
}

static void groups_run(int count, va_list args) {
  for(int i = 0; i < count; i++) {
    group_run_helper(va_arg(args, const struct ctf_internal_group *),
                     print_buff[parallel_thread_index]);
    printf("%s", print_buff[parallel_thread_index]);
    fflush(stdout);
  }
}

static void assert_copy(struct ctf_internal_state *state, int line,
                        const char *file) {
  state->line = line;
  strlcpy(state->file, file, CTF_CONST_STATE_FILE_SIZE);
}

static int parallel_get_thread_index(void) {
  const pthread_t thread = pthread_self();
  for(int i = 0; i < opt_threads; i++) {
    if(pthread_equal(parallel_threads[i], thread)) {
      return i;
    }
  }
  return -1;
}

static void *parallel_thread_loop(void *data) {
  (void)data;
  parallel_thread_index = parallel_get_thread_index();
  ctf_internal_states = parallel_states[parallel_thread_index];
  const struct ctf_internal_group *group;
  while(1) {
    pthread_mutex_lock(&parallel_task_queue_mutex);
    if(parallel_task_queue[0] == NULL) {
      if(parallel_threads_waiting == opt_threads - 1)
        pthread_cond_signal(&parallel_threads_waiting_all);
      parallel_threads_waiting++;
      while(parallel_task_queue[0] == NULL && parallel_state) {
        pthread_cond_wait(&parallel_task_queue_non_empty,
                          &parallel_task_queue_mutex);
      }
      parallel_threads_waiting--;
      if(parallel_task_queue[0] == NULL && !parallel_state) {
        pthread_mutex_unlock(&parallel_task_queue_mutex);
        return NULL;
      }
    }
    group = parallel_task_queue[0];
    for(int i = 0; i < TASK_QUEUE_MAX - 1 && parallel_task_queue[i] != NULL;
        i++) {
      parallel_task_queue[i] = parallel_task_queue[i + 1];
    }
    pthread_cond_signal(&parallel_task_queue_non_full);
    pthread_mutex_unlock(&parallel_task_queue_mutex);
    group_run_helper(group, print_buff[parallel_thread_index]);
    pthread_mutex_lock(&parallel_print_mutex);
    printf("%s", print_buff[parallel_thread_index]);
    print_buff[parallel_thread_index][0] = 0;
    fflush(stdout);
    pthread_mutex_unlock(&parallel_print_mutex);
  }
}

static void parallel_group_run(const struct ctf_internal_group *group) {
  pthread_mutex_lock(&parallel_task_queue_mutex);
  while(parallel_task_queue[TASK_QUEUE_MAX - 1] != NULL) {
    pthread_cond_wait(&parallel_task_queue_non_full,
                      &parallel_task_queue_mutex);
  }
  for(int i = 0; i < TASK_QUEUE_MAX; i++) {
    if(parallel_task_queue[i] == NULL) {
      parallel_task_queue[i] = group;
      break;
    }
  }
  pthread_cond_signal(&parallel_task_queue_non_empty);
  pthread_mutex_unlock(&parallel_task_queue_mutex);
}

static void parallel_groups_run(int count, va_list args) {
  const struct ctf_internal_group *group;

  pthread_mutex_lock(&parallel_task_queue_mutex);
  for(int j = 0; j < count; j++) {
    group = va_arg(args, const struct ctf_internal_group *);
    while(parallel_task_queue[TASK_QUEUE_MAX - 1] != NULL) {
      pthread_cond_wait(&parallel_task_queue_non_full,
                        &parallel_task_queue_mutex);
    }
    for(int i = 0; i < TASK_QUEUE_MAX; i++) {
      if(parallel_task_queue[i] == NULL) {
        parallel_task_queue[i] = group;
        break;
      }
    }
  }
  pthread_cond_broadcast(&parallel_task_queue_non_empty);
  pthread_mutex_unlock(&parallel_task_queue_mutex);
}

#define EXPECT_END                                  \
  ctf_internal_state_index++;                       \
  if(opt_threads != 1) {                            \
    parallel_states_index[parallel_thread_index]++; \
  }

#define EXPECT_GEN_PRIMITIVE                  \
  EXPECT_GEN(char_eq, char, ==, "'%c'");      \
  EXPECT_GEN(char_neq, char, !=, "'%c'");     \
  EXPECT_GEN(char_gt, char, >, "'%c'");       \
  EXPECT_GEN(char_lt, char, <, "'%c'");       \
  EXPECT_GEN(char_gte, char, >=, "'%c'");     \
  EXPECT_GEN(char_lte, char, <=, "%c");       \
  EXPECT_GEN(int_eq, intmax_t, ==, "%jd");    \
  EXPECT_GEN(int_neq, intmax_t, !=, "%jd");   \
  EXPECT_GEN(int_gt, intmax_t, >, "%jd");     \
  EXPECT_GEN(int_lt, intmax_t, <, "%jd");     \
  EXPECT_GEN(int_gte, intmax_t, >=, "%jd");   \
  EXPECT_GEN(int_lte, intmax_t, <=, "%jd");   \
  EXPECT_GEN(uint_eq, uintmax_t, ==, "%ju");  \
  EXPECT_GEN(uint_neq, uintmax_t, !=, "%ju"); \
  EXPECT_GEN(uint_gt, uintmax_t, >, "%ju");   \
  EXPECT_GEN(uint_lt, uintmax_t, <, "%ju");   \
  EXPECT_GEN(uint_gte, uintmax_t, >=, "%ju"); \
  EXPECT_GEN(uint_lte, uintmax_t, <=, "%ju");

int ctf_internal_fail(const char *m, int line, const char *file) {
  ctf_internal_states[ctf_internal_state_index].status = 1;
  EXPECT_END;
  assert_copy(ctf_internal_states + ctf_internal_state_index - 1, line, file);
  strlcpy(ctf_internal_states[ctf_internal_state_index - 1].msg, m,
          CTF_CONST_STATE_MSG_SIZE);
  return 0;
}
int ctf_internal_pass(const char *m, int line, const char *file) {
  ctf_internal_states[ctf_internal_state_index].status = 0;
  EXPECT_END;
  assert_copy(ctf_internal_states + ctf_internal_state_index - 1, line, file);
  strlcpy(ctf_internal_states[ctf_internal_state_index - 1].msg, m,
          CTF_CONST_STATE_MSG_SIZE);
  return 1;
}
int ctf_internal_expect_msg(int status, const char *msg, int line,
                            const char *file) {
  ctf_internal_states[ctf_internal_state_index].status = status;
  EXPECT_END;
  assert_copy(ctf_internal_states + ctf_internal_state_index - 1, line, file);
  strlcpy(ctf_internal_states[ctf_internal_state_index - 1].msg, msg,
          CTF_CONST_STATE_MSG_SIZE);
  return status;
}
int ctf_internal_expect_true(int a, const char *a_str, int line,
                             const char *file) {
  ctf_internal_states[ctf_internal_state_index].status = !a;
  EXPECT_END;
  assert_copy(ctf_internal_states + ctf_internal_state_index - 1, line, file);
  snprintf(ctf_internal_states[ctf_internal_state_index - 1].msg,
           CTF_CONST_STATE_MSG_SIZE, "%s == true (%d != 0)", a_str, a);
  return a;
}
int ctf_internal_expect_false(int a, const char *a_str, int line,
                              const char *file) {
  ctf_internal_states[ctf_internal_state_index].status = a;
  EXPECT_END;
  assert_copy(ctf_internal_states + ctf_internal_state_index - 1, line, file);
  snprintf(ctf_internal_states[ctf_internal_state_index - 1].msg,
           CTF_CONST_STATE_MSG_SIZE, "%s == false (%d == 0)", a_str, a);
  return !a;
}
#define EXPECT_GEN(name, type, op, format)                                     \
  int ctf_internal_expect_##name(type a, type b, const char *a_str,            \
                                 const char *b_str, int line,                  \
                                 const char *file) {                           \
    int status;                                                                \
    ctf_internal_states[ctf_internal_state_index].status = 2;                  \
    EXPECT_END;                                                                \
    assert_copy(ctf_internal_states + ctf_internal_state_index - 1, line,      \
                file);                                                         \
    snprintf(ctf_internal_states[ctf_internal_state_index - 1].msg,            \
             CTF_CONST_STATE_MSG_SIZE,                                         \
             "%s " #op " %s (" format " " #op " " format ")", a_str, b_str, a, \
             b);                                                               \
    status = ((a)op(b));                                                       \
    ctf_internal_states[ctf_internal_state_index - 1].status = !status;        \
    return status;                                                             \
  }
EXPECT_GEN_PRIMITIVE
EXPECT_GEN(ptr_eq, const void *, ==, "%p");
EXPECT_GEN(ptr_neq, const void *, !=, "%p");
EXPECT_GEN(ptr_gt, const void *, >, "%p");
EXPECT_GEN(ptr_lt, const void *, <, "%p");
EXPECT_GEN(ptr_gte, const void *, >=, "%p");
EXPECT_GEN(ptr_lte, const void *, <=, "%p");
#undef EXPECT_GEN
#define EXPECT_GEN(name, type, op, format, f)                                  \
  int ctf_internal_expect_##name(type a, type b, const char *a_str,            \
                                 const char *b_str, int line,                  \
                                 const char *file) {                           \
    int status;                                                                \
    ctf_internal_states[ctf_internal_state_index].status = 2;                  \
    EXPECT_END;                                                                \
    assert_copy(ctf_internal_states + ctf_internal_state_index - 1, line,      \
                file);                                                         \
    snprintf(ctf_internal_states[ctf_internal_state_index - 1].msg,            \
             CTF_CONST_STATE_MSG_SIZE,                                         \
             "%s " #op " %s (" format " " #op " " format ")", a_str, b_str, a, \
             b);                                                               \
    status = ((f)(a, b)op(0));                                                 \
    ctf_internal_states[ctf_internal_state_index - 1].status = !status;        \
    return status;                                                             \
  }
EXPECT_GEN(string_eq, const char *, ==, "\"%s\"", strcmp);
EXPECT_GEN(string_neq, const char *, !=, "\"%s\"", strcmp);
EXPECT_GEN(string_gt, const char *, >, "\"%s\"", strcmp);
EXPECT_GEN(string_lt, const char *, <, "\"%s\"", strcmp);
EXPECT_GEN(string_gte, const char *, >=, "\"%s\"", strcmp);
EXPECT_GEN(string_lte, const char *, <=, "\"%s\"", strcmp);
#undef EXPECT_GEN
_Pragma("GCC diagnostic ignored \"-Wtype-limits\"");
#define EXPECT_GEN(name, type, op, format)                                    \
  int ctf_internal_expect_memory_##name(                                      \
    const void *(a), const void *(b), size_t l, size_t step, int sign,        \
    const char *a_str, const char *b_str, int line, const char *file) {       \
    int status;                                                               \
    ctf_internal_states[ctf_internal_state_index].status = 2;                 \
    EXPECT_END;                                                               \
    assert_copy(ctf_internal_states + ctf_internal_state_index - 1, line,     \
                file);                                                        \
    print_mem(ctf_internal_states + ctf_internal_state_index - 1, a, b, l, l, \
              step, sign, a_str, b_str, #op, format ", ", line, file);        \
    status = ctf_internal_states[ctf_internal_state_index - 1].status op 0;   \
    ctf_internal_states[ctf_internal_state_index - 1].status = !status;       \
    return status;                                                            \
  }
EXPECT_GEN_PRIMITIVE
#undef EXPECT_GEN
#define EXPECT_GEN(name, type, op, format)                                    \
  int ctf_internal_expect_memory_##name(                                      \
    const void *const *(a), const void *const *(b), size_t l, size_t step,    \
    int sign, const char *a_str, const char *b_str, int line,                 \
    const char *file) {                                                       \
    int status;                                                               \
    ctf_internal_states[ctf_internal_state_index].status = 2;                 \
    EXPECT_END;                                                               \
    assert_copy(ctf_internal_states + ctf_internal_state_index - 1, line,     \
                file);                                                        \
    print_mem(ctf_internal_states + ctf_internal_state_index - 1, a, b, l, l, \
              step, sign, a_str, b_str, #op, format ", ", line, file);        \
    status = ctf_internal_states[ctf_internal_state_index - 1].status op 0;   \
    ctf_internal_states[ctf_internal_state_index - 1].status = !status;       \
    return status;                                                            \
  }
EXPECT_GEN(ptr_eq, const void *, ==, "%p");
EXPECT_GEN(ptr_neq, const void *, !=, "%p");
EXPECT_GEN(ptr_gt, const void *, >, "%p");
EXPECT_GEN(ptr_lt, const void *, <, "%p");
EXPECT_GEN(ptr_gte, const void *, >=, "%p");
EXPECT_GEN(ptr_lte, const void *, <=, "%p");
#undef EXPECT_GEN
#define EXPECT_GEN(name, type, op, format)                                  \
  int ctf_internal_expect_array_##name(                                     \
    const void *(a), const void *(b), size_t la, size_t lb, size_t step,    \
    int sign, const char *a_str, const char *b_str, int line,               \
    const char *file) {                                                     \
    int status;                                                             \
    ctf_internal_states[ctf_internal_state_index].status = 2;               \
    EXPECT_END;                                                             \
    assert_copy(ctf_internal_states + ctf_internal_state_index - 1, line,   \
                file);                                                      \
    print_mem(ctf_internal_states + ctf_internal_state_index - 1, a, b, la, \
              lb, step, sign, a_str, b_str, #op, format ", ", line, file);  \
    if(ctf_internal_states[ctf_internal_state_index - 1].status == 0) {     \
      status = (la op lb);                                                  \
    } else {                                                                \
      status =                                                              \
        (ctf_internal_states[ctf_internal_state_index - 1].status op 0);    \
    }                                                                       \
    ctf_internal_states[ctf_internal_state_index - 1].status = !status;     \
    return status;                                                          \
  }
EXPECT_GEN_PRIMITIVE
#undef EXPECT_GEN
#define EXPECT_GEN(name, type, op, format)                                  \
  int ctf_internal_expect_array_##name(                                     \
    const void *const *(a), const void *const *(b), size_t la, size_t lb,   \
    size_t step, int sign, const char *a_str, const char *b_str, int line,  \
    const char *file) {                                                     \
    int status;                                                             \
    ctf_internal_states[ctf_internal_state_index].status = 2;               \
    EXPECT_END;                                                             \
    assert_copy(ctf_internal_states + ctf_internal_state_index - 1, line,   \
                file);                                                      \
    print_mem(ctf_internal_states + ctf_internal_state_index - 1, a, b, la, \
              lb, step, sign, a_str, b_str, #op, format ", ", line, file);  \
    if(ctf_internal_states[ctf_internal_state_index - 1].status == 0) {     \
      status = (la op lb);                                                  \
    } else {                                                                \
      status =                                                              \
        (ctf_internal_states[ctf_internal_state_index - 1].status op 0);    \
    }                                                                       \
    ctf_internal_states[ctf_internal_state_index - 1].status = !status;     \
    return status;                                                          \
  }
EXPECT_GEN(ptr_eq, const void *, ==, "%p");
EXPECT_GEN(ptr_neq, const void *, !=, "%p");
EXPECT_GEN(ptr_gt, const void *, >, "%p");
EXPECT_GEN(ptr_lt, const void *, <, "%p");
EXPECT_GEN(ptr_gte, const void *, >=, "%p");
EXPECT_GEN(ptr_lte, const void *, <=, "%p");
#undef EXPECT_END
_Pragma("GCC diagnostic pop")
  __attribute__((constructor)) void ctf_internal_premain(int argc,
                                                         char *argv[]) {
  for(int i = 1; i < argc; i++) {
    if(argv[i][0] != '-') err();
    if(!strcmp(argv[i] + 1, "h") || !strcmp(argv[i] + 1, "-help")) {
      err();
    } else if(!strcmp(argv[i] + 1, "u") || !strcmp(argv[i] + 1, "-unicode")) {
      if(i == argc) err();
      i++;
      if(!strcmp(argv[i], "off")) {
        opt_unicode = OFF;
      } else if(!strcmp(argv[i], "generic")) {
        opt_unicode = GENERIC;
      } else if(!strcmp(argv[i], "branded")) {
        opt_unicode = BRANDED;
      }
    } else if(!strcmp(argv[i] + 1, "c") || !strcmp(argv[i] + 1, "-color")) {
      if(i == argc) err();
      i++;
      opt_color = get_value(argv[i]);
    } else if(!strcmp(argv[i] + 1, "d") || !strcmp(argv[i] + 1, "-detail")) {
      if(i == argc) err();
      i++;
      opt_detail = get_value(argv[i]);
    } else if(!strcmp(argv[i] + 1, "f") || !strcmp(argv[i] + 1, "-failed")) {
      opt_failed = 1;
    } else if(!strcmp(argv[i] + 1, "j") || !strcmp(argv[i] + 1, "-jobs")) {
      if(i == argc) err();
      i++;
      sscanf(argv[i], "%u", &opt_threads);
      if(opt_threads > MAX_THREADS) opt_threads = MAX_THREADS;
      if(opt_threads < 1) opt_threads = 1;
    }
  }
  tty_present = !IS_TTY;
  if(opt_color == AUTO) {
    color = tty_present;
  } else {
    color = opt_color;
  }
  if(opt_detail == AUTO) {
    detail = !tty_present;
  } else {
    detail = opt_detail;
  }
#ifndef CTF_NO_SIGNAL
  sigaction(SIGSEGV,
            &(struct sigaction){.sa_handler = ctf_sigsegv_handler,
                                .sa_flags = SA_ONSTACK | SA_RESETHAND},
            NULL);
  sigaltstack(&(stack_t){.ss_sp = signal_stack, .ss_size = SIGNAL_STACK_SIZE},
              NULL);
#endif
}

void ctf_parallel_sync(void) {
  if(opt_threads == 1) return;
  if(!parallel_state) return;
  pthread_mutex_lock(&parallel_task_queue_mutex);
  while(parallel_threads_waiting != opt_threads ||
        parallel_task_queue[0] != NULL) {
    pthread_cond_wait(&parallel_threads_waiting_all,
                      &parallel_task_queue_mutex);
  }
  fflush(stdout);
  pthread_mutex_unlock(&parallel_task_queue_mutex);
}
void ctf_parallel_start(void) {
  if(opt_threads == 1) return;
  parallel_state = 1;
  for(int i = 0; i < opt_threads; i++) {
    pthread_create(parallel_threads + i, NULL, parallel_thread_loop, NULL);
  }
}
void ctf_parallel_stop(void) {
  if(opt_threads == 1) return;
  parallel_state = 0;
  pthread_mutex_lock(&parallel_task_queue_mutex);
  pthread_cond_broadcast(&parallel_task_queue_non_empty);
  pthread_mutex_unlock(&parallel_task_queue_mutex);
  for(int i = 0; i < opt_threads; i++) {
    pthread_join(parallel_threads[i], NULL);
  }
}
void ctf_group_run(const struct ctf_internal_group *group) {
  if(parallel_state) {
    parallel_group_run(group);
  } else {
    group_run(group);
  }
}
void ctf_internal_groups_run(int count, ...) {
  va_list args;
  va_start(args, count);
  if(parallel_state) {
    parallel_groups_run(count, args);
  } else {
    groups_run(count, args);
  }
  va_end(args);
}
void ctf_sigsegv_handler(int unused) {
  (void)unused;
  const char err_color[] = "\x1b[33m";
  size_t buff_index = 0;
  const char premsg[] =
    "----------------------------------------\n"
    "                SIGSEGV\n"
    "----------------------------------------\n"
    "             BUFFER STATE\n"
    "----------------------------------------\n";
  const char postmsg[] =
    "----------------------------------------\n"
    "             BUFFER STATE\n"
    "----------------------------------------\n";
  _Pragma("GCC diagnostic ignored \"-Wunused-result\"");
  if(color) {
    write(STDOUT_FILENO, err_color, sizeof(err_color));
  }
  write(STDOUT_FILENO, premsg, sizeof(premsg));
  if(color) write(STDOUT_FILENO, print_color_reset, sizeof(print_color_reset));
  write(STDOUT_FILENO, print_buff[parallel_thread_index],
        strlen(print_buff[parallel_thread_index]));
  for(int i = 0; i < ctf_internal_state_index; i++) {
    if(ctf_internal_states[i].status == 0) {
      buff_index += print_test_pass_info(
        print_buff[parallel_thread_index] + buff_index,
        PRINT_BUFF_SIZE - buff_index, ctf_internal_states + i);
    } else if(ctf_internal_states[i].status == 1) {
      buff_index += print_test_fail_info(
        print_buff[parallel_thread_index] + buff_index,
        PRINT_BUFF_SIZE - buff_index, ctf_internal_states + i);
    } else if(ctf_internal_states[i].status == 2) {
      buff_index += print_test_unknown_info(
        print_buff[parallel_thread_index] + buff_index,
        PRINT_BUFF_SIZE - buff_index, ctf_internal_states + i);
    }
  }
  write(STDOUT_FILENO, print_buff[parallel_thread_index],
        strlen(print_buff[parallel_thread_index]));
  if(color) write(STDOUT_FILENO, err_color, sizeof(err_color));
  write(STDOUT_FILENO, postmsg, sizeof(postmsg));
  if(color) write(STDOUT_FILENO, print_color_reset, sizeof(print_color_reset));
  _Pragma("GCC diagnostic pop") fflush(stdout);
}
