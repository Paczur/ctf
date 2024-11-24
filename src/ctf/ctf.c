#include "ctf.h"

#include <signal.h>
#include <stdarg.h>
#include <stdlib.h>
#include <unistd.h>

include(`base.m4')

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
  "-j, --jobs     Number of parallel threads to run (default 1)\n"           \
  "-s, --sigsegv  Don't register SIGSEGV handler\n"

#define OFF 0
#define ON 1
#define AUTO 2
#define GENERIC 1
#define BRANDED 2

#define PRINT_BUFF_SIZE 65536
#define TASK_QUEUE_MAX 64

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

char ctf_signal_altstack[CTF_CONST_SIGNAL_STACK_SIZE];
static mtx_t parallel_print_mutex;
static thrd_t parallel_threads[CTF_CONST_MAX_THREADS];
static int parallel_threads_waiting = 0;
static struct ctf_internal_group parallel_task_queue[TASK_QUEUE_MAX] = {0};
static mtx_t parallel_task_queue_mutex;
static cnd_t parallel_threads_waiting_all;
static cnd_t parallel_task_queue_non_empty;
static cnd_t parallel_task_queue_non_full;
static int parallel_state = 0;
static struct ctf_internal_state parallel_states[CTF_CONST_MAX_THREADS]
                                                [CTF_CONST_STATES_PER_THREAD];
static int parallel_states_index[CTF_CONST_MAX_THREADS];
static struct ctf_internal_state states[CTF_CONST_STATES_PER_THREAD];
static char print_buff[CTF_CONST_MAX_THREADS][PRINT_BUFF_SIZE];

thread_local int ctf_internal_parallel_thread_index;
thread_local struct ctf_internal_mock_state *ctf_internal_mock_reset_queue[CTF_CONST_MOCKS_PER_TEST];
thread_local int ctf_internal_mock_reset_count;
thread_local struct ctf_internal_state *ctf_internal_states = states;
thread_local int ctf_internal_state_index = 0;

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
  char *buff, unsigned buff_len, struct ctf_internal_state *state) {
  unsigned index = 0;
  if(index + 8 > buff_len) return index;
  while(index < 8) {
    buff[index++] = ' ';
  }
  if(detail) {
    if(index + strlen(state->file) + 2 + int_length(state->line) >= buff_len)
      return index;
    buff[index++] = '[';
    state->file = buff + index;
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
                        const char *op_str, const char *format) {
  size_t index;
  int status = memcmp(a, b, MIN(la, lb) * step);
  index = snprintf(state->msg, CTF_CONST_STATE_MSG_SIZE, "%s %s %s ({", a_str,
                   op_str, b_str);
  index = print_arr(state, index, a, la, step, sign, format);
  index += snprintf(state->msg + index, CTF_CONST_STATE_MSG_SIZE - index,
                    "} %s {", op_str);
  index = print_arr(state, index, b, lb, step, sign, format);
  snprintf(state->msg + index, CTF_CONST_STATE_MSG_SIZE - index, "})");
  state->status = status;
  return index;
}

static void test_cleanup(void) {
  for(int i = 0; i < ctf_internal_mock_reset_count; i++) {
    ctf_internal_mock_reset_queue[i]->call_count = 0;
    ctf_internal_mock_reset_queue[i]->mock_f = NULL;
    ctf_internal_mock_reset_queue[i]->return_override = 0;
    ctf_internal_mock_reset_queue[i]->check_count = 0;
  }
  ctf_internal_mock_reset_count = 0;
}

static void group_run_helper(struct ctf_internal_group group, char *buff) {
  unsigned buff_index = 0;
  int test_status;
  int group_status = 1;
  buff_index +=
    print_group_unknown(buff + buff_index, PRINT_BUFF_SIZE - buff_index,
                        group.name, strlen(group.name));
  for(int i = 0; i < CTF_CONST_GROUP_SIZE && group.tests[i].f; i++) {
    test_status = 1;
    ctf_internal_state_index = 0;
    if(opt_threads != 1) {
      parallel_states_index[ctf_internal_parallel_thread_index] = 0;
    }
    print_test_unknown(buff + buff_index, PRINT_BUFF_SIZE - buff_index,
                       group.tests[i].name, strlen(group.tests[i].name));
    group.tests[i].f();
    test_cleanup();
    if(opt_threads != 1) {
      ctf_internal_state_index =
        parallel_states_index[ctf_internal_parallel_thread_index];
    }
    for(int j = 0; j < ctf_internal_state_index; j++) {
      if(ctf_internal_states[j].status == 1) {
        test_status = 0;
        group_status = 0;
        break;
      }
    }
    if(!test_status) {
      buff_index +=
        print_test_fail(buff + buff_index, PRINT_BUFF_SIZE - buff_index,
                        group.tests[i].name, strlen(group.tests[i].name));
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
                        group.tests[i].name, strlen(group.tests[i].name));
    }
  }
  if(group_status) {
    print_group_pass(buff, PRINT_BUFF_SIZE, group.name, strlen(group.name));
  } else {
    print_group_fail(buff, PRINT_BUFF_SIZE, group.name, strlen(group.name));
    ctf_exit_code = 1;
  }
}

static void group_run(struct ctf_internal_group group) {
  group_run_helper(group, print_buff[ctf_internal_parallel_thread_index]);
  printf("%s", print_buff[ctf_internal_parallel_thread_index]);
  fflush(stdout);
}

static void groups_run(int count, va_list args) {
  struct ctf_internal_group group;
  for(int i = 0; i < count; i++) {
    group = va_arg(args, struct ctf_internal_group);
    group_run_helper(group, print_buff[ctf_internal_parallel_thread_index]);
    printf("%s", print_buff[ctf_internal_parallel_thread_index]);
    fflush(stdout);
  }
}

static void assert_copy(struct ctf_internal_state *state, int line,
                        const char *file) {
  state->line = line;
  state->file = file;
}

static int parallel_get_thread_index(void) {
  const thrd_t thread = thrd_current();
  for(int i = 0; i < opt_threads; i++) {
    if(thrd_equal(parallel_threads[i], thread)) {
      return i;
    }
  }
  return -1;
}

static int parallel_thread_loop(void *data) {
  (void)data;
  struct ctf_internal_group group;
  ctf_internal_parallel_thread_index = parallel_get_thread_index();
  ctf_internal_states = parallel_states[ctf_internal_parallel_thread_index];
  while(1) {
    mtx_lock(&parallel_task_queue_mutex);
    if(parallel_task_queue[0].tests == NULL) {
      if(parallel_threads_waiting == opt_threads - 1)
        cnd_signal(&parallel_threads_waiting_all);
      parallel_threads_waiting++;
      while(parallel_task_queue[0].tests == NULL && parallel_state) {
        cnd_wait(&parallel_task_queue_non_empty, &parallel_task_queue_mutex);
      }
      parallel_threads_waiting--;
      if(parallel_task_queue[0].tests == NULL && !parallel_state) {
        mtx_unlock(&parallel_task_queue_mutex);
        return 0;
      }
    }
    group = parallel_task_queue[0];
    for(int i = 0;
        i < TASK_QUEUE_MAX - 1 && parallel_task_queue[i].tests != NULL; i++) {
      parallel_task_queue[i] = parallel_task_queue[i + 1];
    }
    cnd_signal(&parallel_task_queue_non_full);
    mtx_unlock(&parallel_task_queue_mutex);
    group_run_helper(group, print_buff[ctf_internal_parallel_thread_index]);
    mtx_lock(&parallel_print_mutex);
    printf("%s", print_buff[ctf_internal_parallel_thread_index]);
    print_buff[ctf_internal_parallel_thread_index][0] = 0;
    fflush(stdout);
    mtx_unlock(&parallel_print_mutex);
  }
}

static void parallel_group_run(struct ctf_internal_group group) {
  mtx_lock(&parallel_task_queue_mutex);
  while(parallel_task_queue[TASK_QUEUE_MAX - 1].tests != NULL) {
    cnd_wait(&parallel_task_queue_non_full, &parallel_task_queue_mutex);
  }
  for(int i = 0; i < TASK_QUEUE_MAX; i++) {
    if(parallel_task_queue[i].tests == NULL) {
      parallel_task_queue[i] = group;
      break;
    }
  }
  cnd_signal(&parallel_task_queue_non_empty);
  mtx_unlock(&parallel_task_queue_mutex);
}

static void parallel_groups_run(int count, va_list args) {
  struct ctf_internal_group group;

  mtx_lock(&parallel_task_queue_mutex);
  for(int j = 0; j < count; j++) {
    group = va_arg(args, struct ctf_internal_group);
    while(parallel_task_queue[TASK_QUEUE_MAX - 1].tests != NULL) {
      cnd_wait(&parallel_task_queue_non_full, &parallel_task_queue_mutex);
    }
    for(int i = 0; i < TASK_QUEUE_MAX; i++) {
      if(parallel_task_queue[i].tests == NULL) {
        parallel_task_queue[i] = group;
        break;
      }
    }
  }
  cnd_broadcast(&parallel_task_queue_non_empty);
  mtx_unlock(&parallel_task_queue_mutex);
}

#define EXPECT_START                                                    \
  if(ctf_internal_state_index >= CTF_CONST_STATES_PER_THREAD) return 0; \
  ctf_internal_state_index++;                                           \
  if(opt_threads != 1) {                                                \
    parallel_states_index[ctf_internal_parallel_thread_index]++;        \
  }

int ctf_internal_fail(const char *m, int line, const char *file) {
  ctf_internal_states[ctf_internal_state_index].status = 1;
  EXPECT_START;
  assert_copy(ctf_internal_states + ctf_internal_state_index - 1, line, file);
  strlcpy(ctf_internal_states[ctf_internal_state_index - 1].msg, m,
          CTF_CONST_STATE_MSG_SIZE);
  return 0;
}
int ctf_internal_pass(const char *m, int line, const char *file) {
  ctf_internal_states[ctf_internal_state_index].status = 0;
  EXPECT_START;
  assert_copy(ctf_internal_states + ctf_internal_state_index - 1, line, file);
  strlcpy(ctf_internal_states[ctf_internal_state_index - 1].msg, m,
          CTF_CONST_STATE_MSG_SIZE);
  return 1;
}
int ctf_internal_expect_msg(int status, const char *msg, int line,
                            const char *file) {
  ctf_internal_states[ctf_internal_state_index].status = status;
  EXPECT_START;
  assert_copy(ctf_internal_states + ctf_internal_state_index - 1, line, file);
  strlcpy(ctf_internal_states[ctf_internal_state_index - 1].msg, msg,
          CTF_CONST_STATE_MSG_SIZE);
  return status;
}
void ctf_internal_mock_check_base(struct ctf_internal_mock_state *state,
                                  const char *v, int ret, int memory) {
  int removed = 0;
  for(int i = 0; i < state->check_count; i++) {
    if(!(state->check[i].type & CTF_INTERNAL_MOCK_TYPE_MEMORY) == memory)
      continue;
    if(strcmp(state->check[i].var, v)) continue;
    if(state->check[i].type & CTF_INTERNAL_MOCK_TYPE_ONCE) {
      state->check[i].f.i = NULL;
      removed++;
    }
    if(!ret && state->check[i].type & CTF_INTERNAL_MOCK_TYPE_ASSERT) {
      longjmp(state->assert_jump, 1);
    }
  }
  for(int i = 0; i < state->check_count; i++) {
    if(state->check[i].f.i == NULL) {
      for(int j = i + 1; j < state->check_count; j++) {
        if(state->check[j].f.i != NULL) {
          state->check[i] = state->check[j];
          state->check[j].f.i = NULL;
          break;
        }
      }
    }
  }
  state->check_count -= removed;                                        \
}
static void mock_base(struct ctf_internal_mock_state *state, int type,
                      int line, const char *file, const char *print_var,
                      void *f, const char *var) {
  struct ctf_internal_check *const check = state->check + state->check_count;
  check->f.i = f;
  check->var = var;
  check->type = type;
  check->line = line;
  check->file = file;
  check->print_var = print_var;
  state->check_count++;
}
// clang-format off
/*
define(`EXPECT_HELPER',
`int ctf_internal_expect_$1_$2($3 a, $3 b, const char *a_str,
const char *b_str, int line, const char *file) {
int status;
EXPECT_START;
ctf_internal_states[ctf_internal_state_index-1].status = 2;
assert_copy(ctf_internal_states + ctf_internal_state_index - 1, line,
            file);
snprintf(ctf_internal_states[ctf_internal_state_index - 1].msg,
         CTF_CONST_STATE_MSG_SIZE,
         "%s $4 %s ( "$5" $4 "$5" )", a_str, b_str, a,
         b);
status = $6 $4 $7;
ctf_internal_states[ctf_internal_state_index - 1].status = !status;
return status;
}')
define(`EXPECT_MEMORY_HELPER',
`int ctf_internal_expect_memory_$1_$2(                                      \
$3(a), $3(b), size_t l, size_t step, int sign,        \
const char *a_str, const char *b_str, int line, const char *file) {       \
    int status;                                                               \
    EXPECT_START;                                                               \
    ctf_internal_states[ctf_internal_state_index-1].status = 2;                 \
    assert_copy(ctf_internal_states + ctf_internal_state_index - 1, line,     \
                file);                                                        \
    print_mem(ctf_internal_states + ctf_internal_state_index - 1, a, b, l, l, \
              step, sign, a_str, b_str, "$4", $5 ", ");        \
    status = ctf_internal_states[ctf_internal_state_index - 1].status $4 0;   \
    ctf_internal_states[ctf_internal_state_index - 1].status = !status;       \
    return status;                                                            \
  }')
define(`EXPECT_ARRAY_HELPER',
`int ctf_internal_expect_array_$1_$2(                                     \
  $3(a), $3(b), size_t la, size_t lb, size_t step,    \
  int sign, const char *a_str, const char *b_str, int line,               \
  const char *file) {                                                     \
  int status;                                                             \
  EXPECT_START;                                                             \
  ctf_internal_states[ctf_internal_state_index-1].status = 2;               \
  assert_copy(ctf_internal_states + ctf_internal_state_index - 1, line,   \
              file);                                                      \
  print_mem(ctf_internal_states + ctf_internal_state_index - 1, a, b, la, \
            lb, step, sign, a_str, b_str, "$4", $5 ", ");  \
  if(ctf_internal_states[ctf_internal_state_index - 1].status == 0) {     \
    status = (la $4 lb);                                                  \
  } else {                                                                \
    status =                                                              \
      (ctf_internal_states[ctf_internal_state_index - 1].status $4 0);    \
  }                                                                       \
  ctf_internal_states[ctf_internal_state_index - 1].status = !status;     \
  return status;                                                          \
}')
define(`MOCK_HELPER',
`void ctf_internal_mock_$1(struct ctf_internal_mock *mock, int type,
                            int line, const char *file, const char *print_var,
                            void *f, const char *var, $2 val) {
  struct ctf_internal_mock_state *const state =
    mock->state + ctf_internal_parallel_thread_index;
  struct ctf_internal_check *const check = state->check + state->check_count;
  check->val.$3 = val;
  mock_base(state, type | $4, line, file, print_var, f, var);
}')
define(`MOCK_STR',
`void ctf_internal_mock_str(struct ctf_internal_mock *mock, int type,
                            int line, const char *file, const char *print_var,
                            void *f, const char *var, const char *val) {
  struct ctf_internal_mock_state *const state =
    mock->state + ctf_internal_parallel_thread_index;
  struct ctf_internal_check *const check = state->check + state->check_count;
  check->val.p = val;
  mock_base(state, type | CTF_INTERNAL_MOCK_TYPE_MEMORY, line, file, print_var, f, var);
}')
define(`MOCK_MEMORY_HELPER',
`void ctf_internal_mock_$1(struct ctf_internal_mock *mock, int type,
                            int line, const char *file, const char *print_var,
                            void *f, const char *var, $2 val, size_t l) {
  struct ctf_internal_mock_state *const state =
    mock->state + ctf_internal_parallel_thread_index;
  struct ctf_internal_check *const check = state->check + state->check_count;
  check->val.$3 = val;
  check->length = l;
  mock_base(state, type | CTF_INTERNAL_MOCK_TYPE_MEMORY, line, file, print_var, f, var);
}')
define(`MOCK_CHECK_HELPER',
`void ctf_internal_mock_check_$1(struct ctf_internal_mock_state *state, $2 v,
const char *v_print) {
  int ret = 1;
  for(int i=0; i<state->check_count; i++) {
    if(state->check[i].type & CTF_INTERNAL_MOCK_TYPE_MEMORY) continue;
    if(strcmp(state->check[i].var, v_print)) continue;
    ret = state->check[i].f.$3(
      state->check[i].val.$3, v,
      state->check[i].print_var, v_print, state->check[i].line,
        state->check[i].file);
  }
  ctf_internal_mock_check_base(state, v_print, ret, 0);
}')
define(`MOCK_CHECK_STR',
       `void ctf_internal_mock_check_str(struct ctf_internal_mock_state *state,
                                         const char *v, const char *v_print) {
  int ret = 1;
  for(int i=0; i<state->check_count; i++) {
    if(!(state->check[i].type & CTF_INTERNAL_MOCK_TYPE_MEMORY)) continue;
    if(strcmp(state->check[i].var, v_print)) continue;
    ret = state->check[i].f.p(
      state->check[i].val.p, v,
      state->check[i].print_var, v_print, state->check[i].line,
        state->check[i].file);
  }
  ctf_internal_mock_check_base(state, v_print, ret, 1);
}')
define(`MOCK_CHECK_MEMORY_HELPER',
       `void ctf_internal_mock_check_memory_$1(struct ctf_internal_mock_state *state, const void * v,
                                               const char *v_print, size_t step, int sign) {
  int ret = 1;
  for(int i=0; i<state->check_count; i++) {
    if(!(state->check[i].type & CTF_INTERNAL_MOCK_TYPE_MEMORY)) continue;
    if(strcmp(state->check[i].var, v_print)) continue;
    ret = state->check[i].f.m( state->check[i].val.p, v, state->check[i].length, step, sign,
      state->check[i].print_var, v_print, state->check[i].line,
        state->check[i].file);
  }
  ctf_internal_mock_check_base(state, v_print, ret, 1);
}')
define(`MOCK', `MOCK_HELPER(`$1', TYPE(`$1'), SHORT(`$1'), 0)')
define(`MOCK_MEMORY', `MOCK_MEMORY_HELPER(`$1', `const void *', `p')')
define(`EXPECT_PRIMITIVE', `EXPECT_HELPER(`$1',`$2',TYPE(`$1'),CMP_SYMBOL(`$2'),FORMAT(`$1'),a,b)')
define(`EXPECT_STRING', `EXPECT_HELPER(`$1',`$2',TYPE(`$1'),CMP_SYMBOL(`$2'),FORMAT(`$1'),strcmp(a,b),0)')
define(`EXPECT_MEMORY', `EXPECT_MEMORY_HELPER(`$1',`$2',`$3',CMP_SYMBOL(`$2'),FORMAT(`$1'))')
define(`EXPECT_ARRAY', `EXPECT_ARRAY_HELPER(`$1',`$2',`$3',CMP_SYMBOL(`$2'),FORMAT(`$1'))')
define(`MOCK_CHECK', `MOCK_CHECK_HELPER(`$1', TYPE(`$1'), SHORT(`$1'))')
define(`MOCK_CHECK_MEMORY', `MOCK_CHECK_MEMORY_HELPER(`$1')')
*/
COMB2(`EXPECT_PRIMITIVE', `(PRIMITIVE_TYPES)', `(CMPS)')
COMB2(`EXPECT_STRING', `(str)', `(CMPS)')
COMB3(`EXPECT_MEMORY', `(char, int, uint)', `(CMPS)', `(const void *)')
COMB3(`EXPECT_MEMORY', `(ptr)', `(CMPS)', `(const void *)')
COMB3(`EXPECT_ARRAY', `(char, int, uint)', `(CMPS)', `(const void *)')
COMB3(`EXPECT_ARRAY', `(ptr)', `(CMPS)', `(const void *)')
COMB(`MOCK', `(PRIMITIVE_TYPES)')
MOCK_STR
COMB(`MOCK_MEMORY', `(memory)')
COMB(`MOCK_CHECK', `(PRIMITIVE_TYPES)')
MOCK_CHECK_STR
COMB(`MOCK_CHECK_MEMORY', `(PRIMITIVE_TYPES)')
// clang-format on
__attribute__((constructor)) void ctf_internal_premain(int argc,
                                                         char *argv[]) {
  int handle_sigsegv = 1;
  mtx_init(&parallel_print_mutex, mtx_plain);
  mtx_init(&parallel_task_queue_mutex, mtx_plain);
  cnd_init(&parallel_threads_waiting_all);
  cnd_init(&parallel_task_queue_non_empty);
  cnd_init(&parallel_task_queue_non_full);
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
      if(opt_threads > CTF_CONST_MAX_THREADS)
        opt_threads = CTF_CONST_MAX_THREADS;
      if(opt_threads < 1) opt_threads = 1;
    } else if(!strcmp(argv[i] + 1, "s") || !strcmp(argv[i] + 1, "-sigsegv")) {
      handle_sigsegv = 0;
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
  if(handle_sigsegv) {
    sigaction(SIGSEGV,
              &(struct sigaction){.sa_handler = ctf_sigsegv_handler,
                                  .sa_flags = SA_ONSTACK | SA_RESETHAND},
              NULL);
    sigaltstack(&(stack_t){.ss_sp = ctf_signal_altstack,
                           .ss_size = CTF_CONST_SIGNAL_STACK_SIZE},
                NULL);
  }
}

void ctf_parallel_sync(void) {
  if(opt_threads == 1) return;
  if(!parallel_state) return;
  mtx_lock(&parallel_task_queue_mutex);
  while(parallel_threads_waiting != opt_threads ||
        parallel_task_queue[0].tests != NULL) {
    cnd_wait(&parallel_threads_waiting_all, &parallel_task_queue_mutex);
  }
  fflush(stdout);
  mtx_unlock(&parallel_task_queue_mutex);
}
void ctf_parallel_start(void) {
  if(opt_threads == 1) return;
  parallel_state = 1;
  for(int i = 0; i < opt_threads; i++) {
    thrd_create(parallel_threads + i, parallel_thread_loop, NULL);
  }
}
void ctf_parallel_stop(void) {
  if(opt_threads == 1) return;
  parallel_state = 0;
  mtx_lock(&parallel_task_queue_mutex);
  cnd_broadcast(&parallel_task_queue_non_empty);
  mtx_unlock(&parallel_task_queue_mutex);
  for(int i = 0; i < opt_threads; i++) {
    thrd_join(parallel_threads[i], NULL);
  }
}
void ctf_internal_group_run(const struct ctf_internal_group group) {
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
  write(STDOUT_FILENO, print_buff[ctf_internal_parallel_thread_index],
        strlen(print_buff[ctf_internal_parallel_thread_index]));
  for(int i = 0; i < ctf_internal_state_index; i++) {
    if(ctf_internal_states[i].status == 0) {
      buff_index += print_test_pass_info(
        print_buff[ctf_internal_parallel_thread_index] + buff_index,
        PRINT_BUFF_SIZE - buff_index, ctf_internal_states + i);
    } else if(ctf_internal_states[i].status == 1) {
      buff_index += print_test_fail_info(
        print_buff[ctf_internal_parallel_thread_index] + buff_index,
        PRINT_BUFF_SIZE - buff_index, ctf_internal_states + i);
    } else if(ctf_internal_states[i].status == 2) {
      buff_index += print_test_unknown_info(
        print_buff[ctf_internal_parallel_thread_index] + buff_index,
        PRINT_BUFF_SIZE - buff_index, ctf_internal_states + i);
    }
  }
  write(STDOUT_FILENO, print_buff[ctf_internal_parallel_thread_index],
        strlen(print_buff[ctf_internal_parallel_thread_index]));
  if(color) write(STDOUT_FILENO, err_color, sizeof(err_color));
  write(STDOUT_FILENO, postmsg, sizeof(postmsg));
  if(color) write(STDOUT_FILENO, print_color_reset, sizeof(print_color_reset));
  _Pragma("GCC diagnostic pop") fflush(stdout);
}
