#include "ctf.h"

#include <setjmp.h>
#include <signal.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
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
  "-p, --passed   (off|on|auto*) printing of passed groups\n"                \
  "-j, --jobs     Number of parallel threads to run (default 1)\n"           \
  "-s, --sigsegv  Don't register SIGSEGV handler\n"                          \
  "--,            Stop parsing arguments\n"
#define OFF 0
#define ON 1
#define AUTO 2
#define GENERIC 1
#define BRANDED 2

#define TASK_QUEUE_MAX 64
#define MIN_DIGITS_FOR_LINE 4

#define DEFAULT_PRINT_BUFF_SIZE 1024
#define DEFAULT_STATE_MSG_CAPACITY 128
#define DEFAULT_THREAD_DATA_STATES_CAPACITY 16
#define DEFAULT_THREAD_DATA_MOCK_RESET_STACK_CAPACITY 8

#define MSG_SPRINTF_APPEND(msg, ...)                                        \
  do {                                                                      \
    const uintmax_t printf_size = snprintf(NULL, 0, __VA_ARGS__) + 1;       \
    uintmax_t mul = printf_size / DEFAULT_STATE_MSG_CAPACITY + 1;           \
    mul = mul % 2 == 0 ? mul : mul + 1;                                     \
    if(msg == NULL) {                                                       \
      msg##_capacity = DEFAULT_STATE_MSG_CAPACITY * mul;                    \
      msg = malloc(msg##_capacity);                                         \
    } else if(printf_size >= msg##_capacity) {                              \
      msg##_capacity = DEFAULT_STATE_MSG_CAPACITY * mul;                    \
      msg = realloc(msg, msg##_capacity);                                   \
    }                                                                       \
    msg##_size +=                                                           \
      snprintf(msg + msg##_size, msg##_capacity - msg##_size, __VA_ARGS__); \
  } while(0)
#define MSG_SPRINTF(msg, ...)                                         \
  do {                                                                \
    const uintmax_t printf_size = snprintf(NULL, 0, __VA_ARGS__) + 1; \
    uintmax_t mul = printf_size / DEFAULT_STATE_MSG_CAPACITY + 1;     \
    mul = mul % 2 == 0 ? mul : mul + 1;                               \
    if(msg == NULL) {                                                 \
      msg##_capacity = DEFAULT_STATE_MSG_CAPACITY * mul;              \
      msg = malloc(msg##_capacity);                                   \
    } else if(printf_size >= msg##_capacity) {                        \
      msg##_capacity = DEFAULT_STATE_MSG_CAPACITY * mul;              \
      msg = realloc(msg, msg##_capacity);                             \
    }                                                                 \
    msg##_size = snprintf(msg, msg##_capacity, __VA_ARGS__);          \
  } while(0)
#define MSG_VSPRINTF(msg, f, v)                                   \
  do {                                                            \
    va_list vc;                                                   \
    va_copy(vc, v);                                               \
    const uintmax_t printf_size = vsnprintf(NULL, 0, f, v) + 1;   \
    uintmax_t mul = printf_size / DEFAULT_STATE_MSG_CAPACITY + 1; \
    mul = mul % 2 == 0 ? mul : mul + 1;                           \
    if(msg == NULL) {                                             \
      msg##_capacity = DEFAULT_STATE_MSG_CAPACITY * mul;          \
      msg = malloc(msg##_capacity);                               \
    } else if(printf_size >= msg##_capacity) {                    \
      msg##_capacity = DEFAULT_STATE_MSG_CAPACITY * mul;          \
      msg = realloc(msg, msg##_capacity);                         \
    }                                                             \
    msg##_size = vsnprintf(msg, msg##_capacity, f, vc);           \
  } while(0)
#define ISSPACE(c) (((c) >= '\t' && (c) <= '\r') || (c) == ' ')
#define SKIP_SPACE(c)      \
  do {                     \
    while(ISSPACE(*(c))) { \
      (c)++;               \
    }                      \
  } while(0)
#define NEXT_ID(c, i)                                           \
  do {                                                          \
    while(!ISSPACE((c)[i]) && (c)[i] != ',' && (c)[i] != ')') { \
      (i)++;                                                    \
    }                                                           \
  } while(0)
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define STRINGIFY2(x) #x
#define STRINGIFY(x) STRINGIFY2(x)

struct buff {
  char *buff;
  uintmax_t size;
  uintmax_t capacity;
};

static int opt_unicode = BRANDED;
static int opt_color = AUTO;
static int opt_detail = AUTO;
static int opt_passed = AUTO;
static int opt_threads = 1;
static int tty_present = 0;
static const char print_color_reset[] = "\x1b[0m";
static int color = 0;
static int detail = 1;

static pthread_mutex_t parallel_print_mutex;
static pthread_t parallel_threads[CTF_CONST_MAX_THREADS];
static int parallel_threads_waiting = 0;
static struct ctf__group parallel_task_queue[TASK_QUEUE_MAX] = {0};
static pthread_mutex_t parallel_task_queue_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t parallel_threads_waiting_all = PTHREAD_COND_INITIALIZER;
static pthread_cond_t parallel_task_queue_non_empty = PTHREAD_COND_INITIALIZER;
static pthread_cond_t parallel_task_queue_non_full = PTHREAD_COND_INITIALIZER;
static uintmax_t parallel_state = 0;
static struct buff print_buff[CTF_CONST_MAX_THREADS];
static jmp_buf ctf__assert_jmp_buff[CTF_CONST_MAX_THREADS];

char ctf_signal_altstack[CTF_CONST_SIGNAL_STACK_SIZE];
pthread_key_t ctf__thread_index;
int ctf_exit_code = 0;
struct ctf__thread_data ctf__thread_data[CTF_CONST_MAX_THREADS] = {0};

static void buff_resize(struct buff *buff, uintmax_t cap) {
  uintmax_t mul = (cap + 1) / DEFAULT_PRINT_BUFF_SIZE + 1;
  mul = mul % 2 == 0 ? mul : mul + 1;
  buff->capacity = DEFAULT_PRINT_BUFF_SIZE * mul;
  buff->buff = realloc(buff->buff, buff->capacity);
}

static void state_init(struct ctf__state *state) {
  state->msg = NULL;
}

static void mock_state_init(struct ctf__mock_state *state) {
  memset(state->return_overrides, 0, sizeof(state->return_overrides));
  memset(state->check, 0, sizeof(state->check));
}

static void thread_data_states_increment(struct ctf__thread_data *data) {
  data->states_size++;
  if(data->states_size >= data->states_capacity) {
    const uintmax_t newcap = data->states_capacity * 2;
    data->states = realloc(data->states, newcap * sizeof(*data->states));
    for(uintmax_t i = data->states_capacity; i < newcap; i++)
      state_init(data->states + i);
    data->states_capacity = newcap;
  }
}

static void thread_data_init(struct ctf__thread_data *data) {
  data->states_size = 0;
  data->mock_reset_stack_size = 0;
  data->states_capacity = DEFAULT_THREAD_DATA_STATES_CAPACITY;
  data->mock_reset_stack_capacity =
    DEFAULT_THREAD_DATA_MOCK_RESET_STACK_CAPACITY;
  data->states = malloc(data->states_capacity * sizeof(*data->states));
  data->mock_reset_stack =
    malloc(data->mock_reset_stack_capacity * sizeof(*data->mock_reset_stack));
  for(uintmax_t k = 0; k < data->states_capacity; k++)
    state_init(data->states + k);
}

static void thread_data_deinit(struct ctf__thread_data *data) {
  free(data->states);
}

static void pthread_key_destr(void *v) {
  (void)v;
}

static int get_value(const char *opt) {
  if(!strcmp(opt, "on")) return ON;
  if(!strcmp(opt, "off")) return OFF;
  if(!strcmp(opt, "auto")) return AUTO;
  puts(HELP_MSG);
  exit(0);
  return 0;
}

static void err(void) {
  puts(HELP_MSG);
  exit(0);
}

static uintmax_t print_pass_indicator(struct buff *buff) {
  uintmax_t full_size = 0;
  static const char print_pass_color[] = "\x1b[32m";
  static const char print_pass_branded[] = "✓";
  static const char print_pass_generic[] = "✓";
  static const char print_pass_off[] = "P";
  if(buff == NULL) {
    full_size++;  // '[' char
    if(color) {
      full_size += sizeof(print_pass_color) - 1;
      full_size += sizeof(print_color_reset) - 1;
    }
    switch(opt_unicode) {
    case OFF:
      full_size += sizeof(print_pass_off) - 1;
      break;
    case GENERIC:
      full_size += sizeof(print_pass_generic) - 1;
      break;
    case BRANDED:
      full_size += sizeof(print_pass_branded) - 1;
      break;
    }
    full_size++;  // ']' char
    return full_size;
  }
  buff->buff[buff->size++] = '[';
  if(color) {
    strcpy(buff->buff + buff->size, print_pass_color);
    buff->size += sizeof(print_pass_color) - 1;
  }
  switch(opt_unicode) {
  case OFF:
    strcpy(buff->buff + buff->size, print_pass_off);
    buff->size += sizeof(print_pass_off) - 1;
    break;
  case GENERIC:
    strcpy(buff->buff + buff->size, print_pass_generic);
    buff->size += sizeof(print_pass_generic) - 1;
    break;
  case BRANDED:
    strcpy(buff->buff + buff->size, print_pass_branded);
    buff->size += sizeof(print_pass_branded) - 1;
    break;
  }
  if(color) {
    strcpy(buff->buff + buff->size, print_color_reset);
    buff->size += sizeof(print_color_reset) - 1;
  }
  buff->buff[buff->size++] = ']';
  return 0;
}

static uintmax_t print_fail_indicator(struct buff *buff) {
  uintmax_t full_size = 0;
  static const char print_fail_color[] = "\x1b[31m";
  static const char print_fail_branded[] = "⚑";
  static const char print_fail_generic[] = "✗";
  static const char print_fail_off[] = "F";
  if(buff == NULL) {
    full_size++;  // '[' char
    if(color) {
      full_size += sizeof(print_fail_color) - 1;
      full_size += sizeof(print_color_reset) - 1;
    }
    switch(opt_unicode) {
    case OFF:
      full_size += sizeof(print_fail_off) - 1;
      break;
    case GENERIC:
      full_size += sizeof(print_fail_generic) - 1;
      break;
    case BRANDED:
      full_size += sizeof(print_fail_branded) - 1;
      break;
    }
    full_size++;  // ']' char
    return full_size;
  }
  buff->buff[buff->size++] = '[';
  if(color) {
    strcpy(buff->buff + buff->size, print_fail_color);
    buff->size += sizeof(print_fail_color) - 1;
  }
  switch(opt_unicode) {
  case OFF:
    strcpy(buff->buff + buff->size, print_fail_off);
    buff->size += sizeof(print_fail_off) - 1;
    break;
  case GENERIC:
    strcpy(buff->buff + buff->size, print_fail_generic);
    buff->size += sizeof(print_fail_generic) - 1;
    break;
  case BRANDED:
    strcpy(buff->buff + buff->size, print_fail_branded);
    buff->size += sizeof(print_fail_branded) - 1;
    break;
  }
  if(color) {
    strcpy(buff->buff + buff->size, print_color_reset);
    buff->size += sizeof(print_color_reset) - 1;
  }
  buff->buff[buff->size++] = ']';
  return 0;
}

static uintmax_t print_unknown_indicator(struct buff *buff) {
  uintmax_t full_size = 0;
  static const char print_unknown_color[] = "\x1b[33m";
  // Padding required in order to replace it with fail indicators easily
  static const char print_unknown_branded[] = "\x1a\x1a?";
  static const char print_unknown_generic[] = "\x1a\x1a?";
  static const char print_unknown_off[] = "?";
  if(buff == NULL) {
    full_size++;  // '[' char
    if(color) {
      full_size += sizeof(print_unknown_color) - 1;
      full_size += sizeof(print_color_reset) - 1;
    }
    switch(opt_unicode) {
    case OFF:
      full_size += sizeof(print_unknown_off) - 1;
      break;
    case GENERIC:
      full_size += sizeof(print_unknown_generic) - 1;
      break;
    case BRANDED:
      full_size += sizeof(print_unknown_branded) - 1;
      break;
    }
    full_size++;  // ']' char
    return full_size;
  }
  buff->buff[buff->size++] = '[';
  if(color) {
    strcpy(buff->buff + buff->size, print_unknown_color);
    buff->size += sizeof(print_unknown_color) - 1;
  }
  switch(opt_unicode) {
  case OFF:
    strcpy(buff->buff + buff->size, print_unknown_off);
    buff->size += sizeof(print_unknown_off) - 1;
    break;
  case GENERIC:
    strcpy(buff->buff + buff->size, print_unknown_generic);
    buff->size += sizeof(print_unknown_generic) - 1;
    break;
  case BRANDED:
    strcpy(buff->buff + buff->size, print_unknown_branded);
    buff->size += sizeof(print_unknown_branded) - 1;
    break;
  }
  if(color) {
    strcpy(buff->buff + buff->size, print_color_reset);
    buff->size += sizeof(print_color_reset) - 1;
  }
  buff->buff[buff->size++] = ']';
  return 0;
}

static uintmax_t print_group_pass(struct buff *buff,
                                 const char *name, uintmax_t name_len) {
  uintmax_t full_size = 0;
  if(buff == NULL) {
    if(opt_passed != OFF) {
      full_size += print_pass_indicator(NULL);
      full_size += 2;  // ' ' '\n' chars
      full_size += name_len;
    }
    return full_size;
  }
  if(opt_passed != OFF) {
    print_pass_indicator(buff);
    buff->size += sprintf(buff->buff + buff->size, " %s", name);
    buff->buff[buff->size++] = '\n';
  }
  return 0;
}

static uintmax_t print_group_fail(struct buff *buff,
                                 const char *name, uintmax_t name_len) {
  uintmax_t full_size = 0;
  if(buff == NULL) {
    full_size += print_fail_indicator(NULL);
    full_size += 2;  // ' ' '\n' chars
    full_size += name_len;
    return full_size;
  }
  print_fail_indicator(buff);
  buff->size += sprintf(buff->buff + buff->size, " %s", name);
  buff->buff[buff->size++] = '\n';
  return 0;
}

static uintmax_t print_group_unknown(struct buff *buff,
                                    const char *name, uintmax_t name_len) {
  uintmax_t full_size = 0;
  if(buff == NULL) {
    full_size += print_unknown_indicator(NULL);
    full_size += 2;  // ' ' '\n' chars
    full_size += name_len;
    return full_size;
  }
  print_unknown_indicator(buff);
  buff->size += sprintf(buff->buff + buff->size, " %s\n", name);
  return 0;
}

static uintmax_t print_test_pass(struct buff *buff, const char *name,
                                uintmax_t name_len) {
  uintmax_t full_size = 0;
  if(buff == NULL) {
    full_size += 4;  // indent
    full_size += print_pass_indicator(NULL);
    full_size += 2;  // ' ' and '\n' chars
    full_size += name_len;
    return full_size;
  }
  memset(buff->buff + buff->size, ' ', 4);
  buff->size += 4;
  print_pass_indicator(buff);
  buff->size += sprintf(buff->buff + buff->size, " %s\n", name);
  return 0;
}

static uintmax_t print_test_fail(struct buff *buff, const char *name, uintmax_t name_len) {
  uintmax_t full_size = 0;
  if(buff == NULL) {
    full_size += 4;  // indent
    full_size += print_fail_indicator(NULL);
    full_size += 2;  // ' ' and '\n' chars
    full_size += name_len;
    return full_size;
  }
  memset(buff->buff + buff->size, ' ', 4);
  buff->size += 4;
  print_fail_indicator(buff);
  buff->size += sprintf(buff->buff + buff->size, " %s\n", name);
  return 0;
}

static uintmax_t print_test_unknown(struct buff *buff,
                                     const char *name, uintmax_t name_len) {
  uintmax_t full_size = 0;
  if(buff == NULL) {
    full_size += 4;  // indent
    full_size += print_unknown_indicator(buff);
    full_size += 2;  // ' ' '\n' chars
    full_size += name_len;
    return full_size;
  }
  memset(buff->buff + buff->size, ' ', 4);
  buff->size += 4;
  print_unknown_indicator(buff);
  buff->size += sprintf(buff->buff + buff->size, " %s\n", name);
  return 0;
}

static uintmax_t print_test_pass_info(struct buff *buff,
                                     const struct ctf__state *state) {
  uintmax_t spaces = 8;
  uintmax_t full_size = 0;
  if(buff == NULL) {
    if(detail) {
      full_size +=
        snprintf(NULL, 0, "[%s|%0" STRINGIFY(MIN_DIGITS_FOR_LINE) "d",
                 state->file, state->line);
    }
    full_size += print_pass_indicator(NULL);
    full_size += 2;  // ' ' '\n' chars
    full_size += state->msg_size;
    return full_size;
  }
  if(detail) {
    spaces += snprintf(NULL, 0, "[%s|%0" STRINGIFY(MIN_DIGITS_FOR_LINE) "d",
                       state->file, state->line);
  }
  memset(buff->buff + buff->size, ' ', spaces);
  buff->size += spaces;
  print_pass_indicator(buff);
  buff->size += sprintf(buff->buff + buff->size, " %s\n", state->msg);
  return 0;
}

static uintmax_t print_test_fail_info(struct buff *buff,
                                     const struct ctf__state *state) {
  uintmax_t full_size = 0;
  if(buff == NULL) {
    full_size += 8;  // indent
    if(detail) {
      full_size +=
        snprintf(NULL, 0, "[%s|%0" STRINGIFY(MIN_DIGITS_FOR_LINE) "d|",
                 state->file, state->line);
      if(color) {
        full_size += sizeof("\x1b[31mE\x1b[0m");
      } else {
        full_size++;  // 'E' char
      }

      full_size += 2;  // "] " string
      full_size += state->msg_size;
      full_size += 1;  // '\n' char
    } else {
      full_size += print_fail_indicator(NULL);
      full_size += 2;  // ' ' '\n' chars
      full_size += state->msg_size;
    }
    return full_size;
  }
  memset(buff->buff + buff->size, ' ', 8);
  buff->size += 8;
  if(detail) {
    buff->size += sprintf(buff->buff + buff->size,
                          "[%s|%0" STRINGIFY(MIN_DIGITS_FOR_LINE) "d|",
                          state->file, state->line);
    if(color) {
      strcpy(buff->buff + buff->size, "\x1b[31mE\x1b[0m");
      buff->size += sizeof("\x1b[31mE\x1b[0m") - 1;
    } else {
      buff->buff[buff->size++] = 'E';
    }
    buff->size += sprintf(buff->buff + buff->size, "] %s\n", state->msg);
  } else {
    print_fail_indicator(buff);
    buff->size += sprintf(buff->buff + buff->size, " %s\n", state->msg);
    buff->buff[buff->size] = 0;
  }
  return 0;
}

static uintmax_t print_test_unknown_info(
  struct buff *buff, struct ctf__state *state) {
  uintmax_t full_size = 0;
  if(buff == NULL) {
    full_size += 8;  // indent
    if(detail) {
      full_size +=
        snprintf(NULL, 0, "[%s|%0" STRINGIFY(MIN_DIGITS_FOR_LINE) "d|",
                 state->file, state->line);
      if(color) {
        full_size += sizeof("\x1b[33mW\x1b[0m");
        ;
      } else {
        full_size++;  // 'W' char
      }
      full_size += 2;  // "] " chars
      full_size += state->msg_size;
      full_size += 1;  // '\n' char
    } else {
      full_size += print_unknown_indicator(NULL);
      full_size += 2;  // ' ' '\n' chars
      full_size += state->msg_size;
    }
    return full_size;
  }
  memset(buff->buff + buff->size, ' ', 8);
  buff->size += 8;
  if(detail) {
    buff->size += sprintf(buff->buff + buff->size,
                          "[%s|%0" STRINGIFY(MIN_DIGITS_FOR_LINE) "d|",
                          state->file, state->line);
    if(color) {
      strcpy(buff->buff + buff->size, "\x1b[33mW\x1b[0m");
      buff->size += sizeof("\x1b[33mW\x1b[0m") - 1;
    } else {
      buff->buff[buff->size++] = 'W';
    }
    buff->size += sprintf(buff->buff + buff->size, "] %s\n", state->msg);
  } else {
    print_unknown_indicator(buff);
    buff->size += sprintf(buff->buff + buff->size, " %s\n", state->msg);
    buff->buff[buff->size] = 0;
  }
  return 0;
}

static void print_arr(struct ctf__state *state,
                        const void *data, uintmax_t size, uintmax_t step, int sign,
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
      for(uintmax_t i = 0; i < size; i++)
        MSG_SPRINTF_APPEND(state->msg, format, (intmax_t)iterator.i8[i]);
    } else {
      for(uintmax_t i = 0; i < size; i++)
        MSG_SPRINTF_APPEND(state->msg, format, (uintmax_t)iterator.u8[i]);
    }
    break;
  case 2:
    if(sign) {
      for(uintmax_t i = 0; i < size; i++)
        MSG_SPRINTF_APPEND(state->msg, format, (intmax_t)iterator.i16[i]);
    } else {
      for(uintmax_t i = 0; i < size; i++)
        MSG_SPRINTF_APPEND(state->msg, format, (uintmax_t)iterator.u16[i]);
    }
    break;
  case 4:
    if(sign) {
      for(uintmax_t i = 0; i < size; i++)
        MSG_SPRINTF_APPEND(state->msg, format, (intmax_t)iterator.i32[i]);
    } else {
      for(uintmax_t i = 0; i < size; i++)
        MSG_SPRINTF_APPEND(state->msg, format, (uintmax_t)iterator.u32[i]);
    }
    break;
  case 8:
    if(sign) {
      for(uintmax_t i = 0; i < size; i++)
        MSG_SPRINTF_APPEND(state->msg, format, (intmax_t)iterator.i64[i]);
    } else {
      for(uintmax_t i = 0; i < size; i++)
        MSG_SPRINTF_APPEND(state->msg, format, (uintmax_t)iterator.u64[i]);
    }
    break;
  }
  state->msg_size -= 2;
}

static void print_mem(struct ctf__state *state, const void *a,
                        const void *b, uintmax_t la, uintmax_t lb, uintmax_t step,
                        int sign, const char *a_str, const char *b_str,
                        const char *op_str, const char *format) {
  int status = memcmp(a, b, MIN(la, lb) * step);
  MSG_SPRINTF(state->msg, "%s %s %s ({", a_str, op_str, b_str);
  print_arr(state, a, la, step, sign, format);
  MSG_SPRINTF_APPEND(state->msg, "} %s {", op_str);
  print_arr(state, b, lb, step, sign, format);
  MSG_SPRINTF_APPEND(state->msg, "})");
  state->status = status;
}

static void test_cleanup(void) {
  intptr_t thread_index = (intptr_t)pthread_getspecific(ctf__thread_index);
  struct ctf__thread_data *thread_data = ctf__thread_data + thread_index;
  for(uintmax_t i = 0; i < thread_data->mock_reset_stack_size; i++) {
    thread_data->mock_reset_stack[i]->call_count = 0;
    thread_data->mock_reset_stack[i]->mock_f = NULL;
    memset(thread_data->mock_reset_stack[i]->return_overrides, 0,
           sizeof(thread_data->mock_reset_stack[i]->return_overrides));
    thread_data->mock_reset_stack[i]->check_count = 0;
  }
  thread_data->mock_reset_stack_size = 0;
}

static void group_run_helper(struct ctf__group group, struct buff *buff) {
  intptr_t thread_index = (intptr_t)pthread_getspecific(ctf__thread_index);
  struct ctf__thread_data *thread_data = ctf__thread_data + thread_index;
  uintmax_t temp_size;
  uintmax_t test_name_len;
  const uintmax_t group_name_len = strlen(group.name);
  int test_status;
  volatile int group_status = 1;
  buff->size = 0;

  temp_size = print_group_unknown(NULL, group.name, group_name_len);
  if(buff->size + temp_size >= buff->capacity)
    buff_resize(buff, buff->size + temp_size);
  print_group_unknown(buff, group.name, group_name_len);

  group.setup();
  for(int i = 0; i < CTF_CONST_GROUP_SIZE && group.tests[i].f; i++) {
    test_name_len = strlen(group.tests[i].name);
    thread_data->states_size = 0;

    temp_size = print_test_unknown(NULL, group.tests[i].name, test_name_len);
    if(buff->size + temp_size >= buff->capacity)
      buff_resize(buff, buff->size + temp_size);
    print_test_unknown(buff, group.tests[i].name, test_name_len);

    group.test_setup();
    if(!setjmp(ctf__assert_jmp_buff[thread_index])) group.tests[i].f();
    group.test_teardown();
    test_cleanup();
    test_status = 1;
    for(uintmax_t j = 0; j < thread_data->states_size; j++) {
      if(thread_data->states[j].status == 1) {
        test_status = 0;
        group_status = 0;
        break;
      }
    }
    buff->size -= temp_size;
    if(!test_status) {
      temp_size = print_test_fail(NULL, group.tests[i].name, test_name_len);
      for(uintmax_t j = 0; j < thread_data->states_size; j++) {
        if(thread_data->states[j].status == 0) {
          temp_size += print_test_pass_info(NULL, thread_data->states + j);
        } else if(thread_data->states[j].status == 1) {
          temp_size += print_test_fail_info(NULL, thread_data->states + j);
        }
      }
      if(buff->size + temp_size >= buff->capacity)
        buff_resize(buff, buff->size + temp_size);

      print_test_fail(buff, group.tests[i].name, test_name_len);
      for(uintmax_t j = 0; j < thread_data->states_size; j++) {
        if(thread_data->states[j].status == 0) {
          print_test_pass_info(buff, thread_data->states + j);
        } else if(thread_data->states[j].status == 1) {
          print_test_fail_info(buff, thread_data->states + j);
        }
      }
    } else {
      temp_size = print_test_pass(NULL, group.tests[i].name, test_name_len);
      if(buff->size + temp_size >= buff->capacity)
        buff_resize(buff, buff->size + temp_size);
      print_test_pass(buff, group.tests[i].name, test_name_len);
    }
  }
  group.teardown();
  temp_size = buff->size;
  buff->size = 0;
  if(group_status) {
    print_group_pass(buff, group.name, group_name_len);
    if(opt_passed == ON) buff->size = temp_size;
  } else {
    print_group_fail(buff, group.name, group_name_len);
    buff->size = temp_size;
    ctf_exit_code = 1;
  }
}

static void group_run(struct ctf__group group) {
  intptr_t thread_index = (intptr_t)pthread_getspecific(ctf__thread_index);
  group_run_helper(group, print_buff + thread_index);
  write(STDOUT_FILENO, print_buff[thread_index].buff,
        print_buff[thread_index].size);
}

static void groups_run(uintmax_t count, va_list args) {
  struct ctf__group group;
  intptr_t thread_index = (intptr_t)pthread_getspecific(ctf__thread_index);
  for(uintmax_t i = 0; i < count; i++) {
    group = va_arg(args, struct ctf__group);
    group_run_helper(group, print_buff + thread_index);
    write(STDOUT_FILENO, print_buff[thread_index].buff,
          print_buff[thread_index].size);
  }
}

static void assert_copy(struct ctf__state *state, int line,
                        const char *file) {
  state->line = line;
  state->file = file;
}

static uintmax_t parallel_get_thread_index(void) {
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
  struct ctf__group group;
  intptr_t thread_index = (intptr_t)data;
  pthread_setspecific(ctf__thread_index, (void *)thread_index);
  while(1) {
    pthread_mutex_lock(&parallel_task_queue_mutex);
    if(parallel_task_queue[0].tests == NULL) {
      if(parallel_threads_waiting == opt_threads - 1)
        pthread_cond_signal(&parallel_threads_waiting_all);
      parallel_threads_waiting++;
      while(parallel_task_queue[0].tests == NULL && parallel_state) {
        pthread_cond_wait(&parallel_task_queue_non_empty,
                          &parallel_task_queue_mutex);
      }
      parallel_threads_waiting--;
      if(parallel_task_queue[0].tests == NULL && !parallel_state) {
        pthread_mutex_unlock(&parallel_task_queue_mutex);
        return 0;
      }
    }
    group = parallel_task_queue[0];
    for(int i = 0;
        i < TASK_QUEUE_MAX - 1 && parallel_task_queue[i].tests != NULL; i++) {
      parallel_task_queue[i] = parallel_task_queue[i + 1];
    }
    pthread_cond_signal(&parallel_task_queue_non_full);
    pthread_mutex_unlock(&parallel_task_queue_mutex);
    group_run_helper(group, print_buff + thread_index);
    pthread_mutex_lock(&parallel_print_mutex);
    write(STDOUT_FILENO, print_buff[thread_index].buff,
          print_buff[thread_index].size);
    print_buff[thread_index].size = 0;
    pthread_mutex_unlock(&parallel_print_mutex);
  }
}

static void parallel_group_run(struct ctf__group group) {
  pthread_mutex_lock(&parallel_task_queue_mutex);
  while(parallel_task_queue[TASK_QUEUE_MAX - 1].tests != NULL) {
    pthread_cond_wait(&parallel_task_queue_non_full,
                      &parallel_task_queue_mutex);
  }
  for(int i = 0; i < TASK_QUEUE_MAX; i++) {
    if(parallel_task_queue[i].tests == NULL) {
      parallel_task_queue[i] = group;
      break;
    }
  }
  pthread_cond_signal(&parallel_task_queue_non_empty);
  pthread_mutex_unlock(&parallel_task_queue_mutex);
}

static void parallel_groups_run(int count, va_list args) {
  struct ctf__group group;

  pthread_mutex_lock(&parallel_task_queue_mutex);
  for(int j = 0; j < count; j++) {
    group = va_arg(args, struct ctf__group);
    while(parallel_task_queue[TASK_QUEUE_MAX - 1].tests != NULL) {
      pthread_cond_wait(&parallel_task_queue_non_full,
                        &parallel_task_queue_mutex);
    }
    for(int i = 0; i < TASK_QUEUE_MAX; i++) {
      if(parallel_task_queue[i].tests == NULL) {
        parallel_task_queue[i] = group;
        break;
      }
    }
  }
  pthread_cond_broadcast(&parallel_task_queue_non_empty);
  pthread_mutex_unlock(&parallel_task_queue_mutex);
}

static void mock_reset(struct ctf__mock_state *state) {
  state->call_count = 0;
  memset(state->return_overrides, 0, sizeof(state->return_overrides));
  state->check_count = 0;                                              \
}

int main(int argc, char *argv[]) {
  int i;
  int handle_sigsegv = 1;
  for(i = 1; i < argc; i++) {
    if(argv[i][0] != '-') err();
    if(!strcmp(argv[i] + 1, "h") || !strcmp(argv[i] + 1, "-help")) {
      err();
    } else if(!strcmp(argv[i] + 1, "u") || !strcmp(argv[i] + 1, "-unicode")) {
      i++;
      if(i >= argc) err();
      if(!strcmp(argv[i], "off")) {
        opt_unicode = OFF;
      } else if(!strcmp(argv[i], "generic")) {
        opt_unicode = GENERIC;
      } else if(!strcmp(argv[i], "branded")) {
        opt_unicode = BRANDED;
      }
    } else if(!strcmp(argv[i] + 1, "c") || !strcmp(argv[i] + 1, "-color")) {
      i++;
      if(i >= argc) err();
      opt_color = get_value(argv[i]);
    } else if(!strcmp(argv[i] + 1, "d") || !strcmp(argv[i] + 1, "-detail")) {
      i++;
      if(i >= argc) err();
      opt_detail = get_value(argv[i]);
    } else if(!strcmp(argv[i] + 1, "p") || !strcmp(argv[i] + 1, "-passed")) {
      i++;
      if(i >= argc) err();
      opt_passed = get_value(argv[i]);
    } else if(!strcmp(argv[i] + 1, "j") || !strcmp(argv[i] + 1, "-jobs")) {
      i++;
      if(i >= argc) err();
      sscanf(argv[i], "%u", &opt_threads);
      if(opt_threads > CTF_CONST_MAX_THREADS)
        opt_threads = CTF_CONST_MAX_THREADS;
      if(opt_threads < 1) opt_threads = 1;
    } else if(!strcmp(argv[i] + 1, "s") || !strcmp(argv[i] + 1, "-sigsegv")) {
      handle_sigsegv = 0;
    } else if(!strcmp(argv[i] + 1, "-")) {
      i++;
      break;
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
  pthread_key_create(&ctf__thread_index, pthread_key_destr);
  pthread_setspecific(ctf__thread_index, (void *)0);
  for(int j = 0; j < opt_threads; j++) {
    thread_data_init(ctf__thread_data + j);
    print_buff[j].buff = malloc(DEFAULT_PRINT_BUFF_SIZE);
    print_buff[j].size = 0;
    print_buff[j].capacity = DEFAULT_PRINT_BUFF_SIZE;
  }
  ctf_main(argc - i, argv + i);
  pthread_key_delete(ctf__thread_index);
  return ctf_exit_code;
}
void ctf_sigsegv_handler(int unused) {
  (void)unused;
  intptr_t thread_index = (intptr_t)pthread_getspecific(ctf__thread_index);
  struct ctf__thread_data *thread_data = ctf__thread_data + thread_index;
  const char err_color[] = "\x1b[33m";
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
#pragma GCC diagnostic ignored "-Wunused-result"
  if(color) {
    write(STDOUT_FILENO, err_color, sizeof(err_color));
  }
  write(STDOUT_FILENO, premsg, sizeof(premsg));
  if(color) write(STDOUT_FILENO, print_color_reset, sizeof(print_color_reset));
  write(STDOUT_FILENO, print_buff[thread_index].buff,
        print_buff[thread_index].size);
  for(uintmax_t i = 0; i < thread_data->states_size; i++) {
    if(thread_data->states[i].status == 0) {
      print_test_pass_info(print_buff + thread_index, thread_data->states + i);
    } else if(thread_data->states[i].status == 1) {
      print_test_fail_info(print_buff + thread_index, thread_data->states + i);
    } else if(thread_data->states[i].status == 2) {
      print_test_unknown_info(print_buff + thread_index,
                              thread_data->states + i);
    }
  }
  if(color) write(STDOUT_FILENO, err_color, sizeof(err_color));
  write(STDOUT_FILENO, postmsg, sizeof(postmsg));
  if(color) write(STDOUT_FILENO, print_color_reset, sizeof(print_color_reset));
#pragma GCC diagnostic pop
  fflush(stdout);
}

void ctf_parallel_sync(void) {
  if(opt_threads == 1) return;
  if(!parallel_state) return;
  pthread_mutex_lock(&parallel_task_queue_mutex);
  while(parallel_threads_waiting != opt_threads ||
        parallel_task_queue[0].tests != NULL) {
    pthread_cond_wait(&parallel_threads_waiting_all,
                      &parallel_task_queue_mutex);
  }
  fflush(stdout);
  pthread_mutex_unlock(&parallel_task_queue_mutex);
}
void ctf_parallel_start(void) {
  if(opt_threads == 1) return;
  parallel_state = 1;
  for(intptr_t i = 0; i < opt_threads; i++) {
    pthread_create(parallel_threads + i, NULL, parallel_thread_loop, (void *)i);
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

void ctf_group_run(const struct ctf__group group) {
  if(parallel_state) {
    parallel_group_run(group);
  } else {
    group_run(group);
  }
}
void ctf__groups_run(int count, ...) {
  va_list args;
  va_start(args, count);
  if(parallel_state) {
    parallel_groups_run(count, args);
  } else {
    groups_run(count, args);
  }
  va_end(args);
}
void ctf_assert_barrier(void) {
  intptr_t thread_index = (intptr_t)pthread_getspecific(ctf__thread_index);
  struct ctf__thread_data *thread_data = ctf__thread_data + thread_index;
  for(uintmax_t i = 0; i < thread_data->states_size; i++) {
    if(thread_data->states[i].status) {
      longjmp(ctf__assert_jmp_buff[thread_index], 1);
    }
  }
}
void ctf__assert_fold(uintmax_t count, const char *msg, int line, const char *file) {
  ctf_assert_hide(count);
  ctf__pass(msg, line, file);
}
void ctf_assert_hide(uintmax_t count) {
  intptr_t thread_index = (intptr_t)pthread_getspecific(ctf__thread_index);
  struct ctf__thread_data *thread_data = ctf__thread_data + thread_index;
  if(thread_data->states_size <= count) {
    for(uintmax_t i = 0; i < thread_data->states_size; i++) {
      if(thread_data->states[i].status) {
        longjmp(ctf__assert_jmp_buff[thread_index], 1);
      }
    }
    thread_data->states_size = 0;
  } else {
    for(uintmax_t i = thread_data->states_size - count;
        i < thread_data->states_size; i++) {
      if(thread_data->states[i].status) {
        longjmp(ctf__assert_jmp_buff[thread_index], 1);
      }
    }
    thread_data->states_size -= count;
  }
}

void ctf_unmock(void) {
  intptr_t thread_index = (intptr_t)pthread_getspecific(ctf__thread_index);
  struct ctf__thread_data *thread_data = ctf__thread_data + thread_index;
  prereq_assert(thread_data->mock_reset_stack_size != 0, "No mocks to unmock");
  struct ctf__mock_state *state =
    thread_data->mock_reset_stack[--thread_data->mock_reset_stack_size];
  state->mock_f = NULL;
  mock_reset(state);
}
void ctf__mock_group(struct ctf__mock_bind *bind) {
  intptr_t thread_index = (intptr_t)pthread_getspecific(ctf__thread_index);
  struct ctf__thread_data *thread_data = ctf__thread_data + thread_index;
  struct ctf__mock_state *state;
  for(uintmax_t i = 0; i < CTF_CONST_MOCK_GROUP_SIZE && bind[i].f; i++) {
    state = bind[i].mock->state + thread_index;
    thread_data->mock_reset_stack[thread_data->mock_reset_stack_size++] = state;
    state->mock_f = (void (*)(void))bind[i].f;
    mock_reset(state);
  }                                                                         \
}
void ctf__mock_global(struct ctf__mock_state *state, void(*f)(void)) {
  intptr_t thread_index = (intptr_t)pthread_getspecific(ctf__thread_index);
  struct ctf__thread_data *thread_data = ctf__thread_data + thread_index;
  state += thread_index;
  state->mock_f = f;
  thread_data->mock_reset_stack[thread_data->mock_reset_stack_size++] = state;
  mock_reset(state);                                                   \
}
void ctf__unmock_group(struct ctf__mock_bind *bind) {
  intptr_t thread_index = (intptr_t)pthread_getspecific(ctf__thread_index);
  struct ctf__mock_state *state;
  for(uintmax_t i = 0; i < CTF_CONST_MOCK_GROUP_SIZE && bind[i].f; i++) {
    state = bind[i].mock->state + thread_index;
    state->mock_f = (void (*)(void))NULL;
    mock_reset(state);
  }
}

uintmax_t ctf__fail(const char *m, int line, const char *file, ...) {
  va_list v;
  intptr_t thread_index = (intptr_t)pthread_getspecific(ctf__thread_index);
  struct ctf__thread_data *thread_data = ctf__thread_data + thread_index;
  thread_data_states_increment(thread_data);
  thread_data->states[thread_data->states_size - 1].status = 1;
  assert_copy(thread_data->states + thread_data->states_size - 1, line, file);
  va_start(v, file);
  MSG_VSPRINTF(thread_data->states[thread_data->states_size - 1].msg, m, v);
  va_end(v);
  return 0;
}
uintmax_t ctf__pass(const char *m, int line, const char *file, ...) {
  va_list v;
  intptr_t thread_index = (intptr_t)pthread_getspecific(ctf__thread_index);
  struct ctf__thread_data *thread_data = ctf__thread_data + thread_index;
  thread_data_states_increment(thread_data);
  thread_data->states[thread_data->states_size - 1].status = 0;
  assert_copy(thread_data->states + thread_data->states_size - 1, line, file);
  va_start(v, file);
  MSG_VSPRINTF(thread_data->states[thread_data->states_size - 1].msg, m, v);
  va_end(v);
  return 1;
}
uintmax_t ctf__assert_msg(int status, const char *msg, int line,
                            const char *file, ...) {
  va_list args;
  intptr_t thread_index = (intptr_t)pthread_getspecific(ctf__thread_index);
  struct ctf__thread_data *thread_data = ctf__thread_data + thread_index;
  thread_data_states_increment(thread_data);
  thread_data->states[thread_data->states_size - 1].status = status;
  assert_copy(thread_data->states + thread_data->states_size - 1, line, file);
  thread_data->states[thread_data->states_size - 1].msg_size = 0;
  va_start(args, file);
  MSG_VSPRINTF(thread_data->states[thread_data->states_size - 1].msg, msg,
               args);
  va_end(args);
  return status;
}
static void mock_check_base(struct ctf__mock_state *state,
                                  const char *v, int memory) {
  int removed = 0;
  for(uintmax_t i = 0; i < state->check_count; i++) {
    if(!(state->check[i].type & CTF__MOCK_TYPE_MEMORY) == memory) continue;
    if(strcmp(state->check[i].var, v)) continue;
    if(state->check[i].call_count != state->call_count) continue;
    state->check[i].f.i = NULL;
    removed++;
  }
  for(uintmax_t i = 0; i < state->check_count; i++) {
    if(state->check[i].f.i == NULL) {
      for(uintmax_t j = i + 1; j < state->check_count; j++) {
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
static void mock_base(struct ctf__mock_state *state, int call_count, int type,
                      int line, const char *file, const char *print_var,
                      void *f, const char *var) {
  struct ctf__check *const check = state->check + state->check_count;
  check->f.i = f;
  check->var = var;
  check->call_count = call_count;
  check->type = type;
  check->line = line;
  check->file = file;
  check->print_var = print_var;
  state->check_count++;
}
// clang-format off
/* EXPECTS AND ASSERTS
define(`EXPECT_HELPER',
`int ctf__expect_$1_$2($3 a, $3 b, const char *a_str,
const char *b_str, int line, const char *file) {
int status;
intptr_t thread_index = (intptr_t)pthread_getspecific(ctf__thread_index);
struct ctf__thread_data *thread_data = ctf__thread_data + thread_index;
thread_data_states_increment(thread_data);
thread_data->states[thread_data->states_size-1].status = 2;
assert_copy(thread_data->states + thread_data->states_size - 1, line,
            file);
MSG_SPRINTF(thread_data->states[thread_data->states_size -1].msg, "%s $4 %s ( "$5" $4 "$5" )", a_str, b_str, a,
b);
status = $6 $4 $7;
thread_data->states[thread_data->states_size - 1].status = !status;
return status;
}')
define(`EXPECT_MEMORY_HELPER',
`int ctf__expect_memory_$1_$2(                                      \
$3(a), $3(b), uintmax_t l, uintmax_t step, int sign,        \
const char *a_str, const char *b_str, int line, const char *file) {       \
    int status;                                                               \
    intptr_t thread_index = (intptr_t)pthread_getspecific(ctf__thread_index);
    struct ctf__thread_data *thread_data = ctf__thread_data + thread_index;
    thread_data_states_increment(thread_data);
    thread_data->states[thread_data->states_size-1].status = 2;                 \
    assert_copy(thread_data->states + thread_data->states_size - 1, line,     \
                file);                                                        \
    print_mem(thread_data->states + thread_data->states_size - 1, a, b, l, l, \
              step, sign, a_str, b_str, "$4", $5 ", ");        \
    status = thread_data->states[thread_data->states_size - 1].status $4 0;   \
    thread_data->states[thread_data->states_size - 1].status = !status;       \
    return status;                                                            \
  }')
define(`EXPECT_ARRAY_HELPER',
`int ctf__expect_array_$1_$2(                                     \
  $3(a), $3(b), uintmax_t la, uintmax_t lb, uintmax_t step,    \
  int sign, const char *a_str, const char *b_str, int line,               \
  const char *file) {                                                     \
  int status;                                                             \
  intptr_t thread_index = (intptr_t)pthread_getspecific(ctf__thread_index);
  struct ctf__thread_data *thread_data = ctf__thread_data + thread_index;
  thread_data_states_increment(thread_data);
  thread_data->states[thread_data->states_size-1].status = 2;               \
  assert_copy(thread_data->states + thread_data->states_size - 1, line,   \
              file);                                                      \
  print_mem(thread_data->states + thread_data->states_size - 1, a, b, la, \
            lb, step, sign, a_str, b_str, "$4", $5 ", ");  \
  if(thread_data->states[thread_data->states_size - 1].status == 0) {     \
    status = (la $4 lb);                                                  \
  } else {                                                                \
    status =                                                              \
      (thread_data->states[thread_data->states_size - 1].status $4 0);    \
  }                                                                       \
  thread_data->states[thread_data->states_size - 1].status = !status;     \
  return status;                                                          \
}')
define(`ASSERT_HELPER',
`int ctf__assert_$1_$2($3 a, $3 b, const char *a_str,
const char *b_str, int line, const char *file) {
int status;
intptr_t thread_index = (intptr_t)pthread_getspecific(ctf__thread_index);
struct ctf__thread_data *thread_data = ctf__thread_data + thread_index;
thread_data_states_increment(thread_data);
thread_data->states[thread_data->states_size-1].status = 2;
assert_copy(thread_data->states + thread_data->states_size - 1, line,
            file);
            MSG_SPRINTF(thread_data->states[thread_data->states_size - 1].msg,
"%s $4 %s ( "$5" $4 "$5" )", a_str, b_str, a,
         b);
status = $6 $4 $7;
thread_data->states[thread_data->states_size - 1].status = !status;
if(!status) longjmp(ctf__assert_jmp_buff[thread_index], 1);                           \
return status;
}')
define(`ASSERT_MEMORY_HELPER',
`int ctf__assert_memory_$1_$2(                                      \
$3(a), $3(b), uintmax_t l, uintmax_t step, int sign,        \
const char *a_str, const char *b_str, int line, const char *file) {       \
    int status;                                                               \
    intptr_t thread_index = (intptr_t)pthread_getspecific(ctf__thread_index);
    struct ctf__thread_data *thread_data = ctf__thread_data + thread_index;
    thread_data_states_increment(thread_data);
    thread_data->states[thread_data->states_size-1].status = 2;                 \
    assert_copy(thread_data->states + thread_data->states_size - 1, line,     \
                file);                                                        \
    print_mem(thread_data->states + thread_data->states_size - 1, a, b, l, l, \
              step, sign, a_str, b_str, "$4", $5 ", ");        \
    status = thread_data->states[thread_data->states_size - 1].status $4 0;   \
    thread_data->states[thread_data->states_size - 1].status = !status;       \
    if(!status) longjmp(ctf__assert_jmp_buff[thread_index], 1);                           \
    return status;                                                            \
  }')
define(`ASSERT_ARRAY_HELPER',
`int ctf__assert_array_$1_$2(                                     \
  $3(a), $3(b), uintmax_t la, uintmax_t lb, uintmax_t step,    \
  int sign, const char *a_str, const char *b_str, int line,               \
  const char *file) {                                                     \
  int status;                                                             \
  intptr_t thread_index = (intptr_t)pthread_getspecific(ctf__thread_index);
  struct ctf__thread_data *thread_data = ctf__thread_data + thread_index;
  thread_data_states_increment(thread_data);
  thread_data->states[thread_data->states_size-1].status = 2;               \
  assert_copy(thread_data->states + thread_data->states_size - 1, line,   \
              file);                                                      \
  print_mem(thread_data->states + thread_data->states_size - 1, a, b, la, \
            lb, step, sign, a_str, b_str, "$4", $5 ", ");  \
  if(thread_data->states[thread_data->states_size - 1].status == 0) {     \
    status = (la $4 lb);                                                  \
  } else {                                                                \
    status =                                                              \
      (thread_data->states[thread_data->states_size - 1].status $4 0);    \
  }                                                                       \
  thread_data->states[thread_data->states_size - 1].status = !status;     \
  if(!status) longjmp(ctf__assert_jmp_buff[thread_index], 1);                           \
  return status;                                                          \
}')
*/
/* MOCKS
define(`MOCK_HELPER',
`void ctf__mock_$1(struct ctf__mock_state *state,uintmax_t call_count, int type,
                            int line, const char *file, const char *print_var,
                            void *f, const char *var, $2 val) {
  struct ctf__check *const check = state->check + state->check_count;
  check->val.$3 = val;
  mock_base(state, call_count, type | $4, line, file, print_var, f, var);
}')
define(`MOCK_STR',
`void ctf__mock_str(struct ctf__mock_state *state,uintmax_t call_count, int type,
                            int line, const char *file, const char *print_var,
                            void *f, const char *var, const char *val) {
  struct ctf__check *const check = state->check + state->check_count;
  check->val.p = val;
  mock_base(state, call_count, type | CTF__MOCK_TYPE_MEMORY, line, file, print_var, f, var);
}')
define(`MOCK_MEMORY_HELPER',
`void ctf__mock_$1(struct ctf__mock_state *state,uintmax_t call_count, int type,
                            int line, const char *file, const char *print_var,
                            void *f, const char *var, $2 val, uintmax_t l) {
  struct ctf__check *const check = state->check + state->check_count;
  check->val.$3 = val;
  check->length = l;
  mock_base(state, call_count, type | CTF__MOCK_TYPE_MEMORY, line, file, print_var, f, var);
}')
*/
/* MOCK CHECKS
define(`MOCK_CHECK_HELPER',
`void ctf__mock_check_$1(struct ctf__mock_state *state, $2 v,
const char *v_print) {
  int ret = 1;
    intptr_t thread_index =                                                   \
      (intptr_t)pthread_getspecific(ctf__thread_index);               \
  for(uintmax_t i=0; i<state->check_count; i++) {
    if(state->check[i].type & CTF__MOCK_TYPE_MEMORY) continue;
    if(strcmp(state->check[i].var, v_print)) continue;
    if(state->check[i].call_count != state->call_count) continue;
    ret = state->check[i].f.$3(
      state->check[i].val.$3, v,
      state->check[i].print_var, v_print, state->check[i].line,
        state->check[i].file);
    if(!ret && state->check[i].type & CTF__MOCK_TYPE_ASSERT) {
    longjmp(ctf__assert_jmp_buff[thread_index], 1);
    }
  }
  mock_check_base(state, v_print, 0);
}')
define(`MOCK_CHECK_STR',
       `void ctf__mock_check_str(struct ctf__mock_state *state,
                                         const char *v, const char *v_print) {
  int ret = 1;
    intptr_t thread_index =                                                   \
      (intptr_t)pthread_getspecific(ctf__thread_index);               \
  for(uintmax_t i=0; i<state->check_count; i++) {
    if(!(state->check[i].type & CTF__MOCK_TYPE_MEMORY)) continue;
    if(strcmp(state->check[i].var, v_print)) continue;
    if(state->check[i].call_count != state->call_count) continue;
    ret = state->check[i].f.p(
      state->check[i].val.p, v,
      state->check[i].print_var, v_print, state->check[i].line,
        state->check[i].file);
    if(!ret && state->check[i].type & CTF__MOCK_TYPE_ASSERT) {
    longjmp(ctf__assert_jmp_buff[thread_index], 1);
    }
  }
  mock_check_base(state, v_print, 1);
}')
define(`MOCK_CHECK_MEMORY_HELPER',
`void ctf__mock_check_memory_$1(struct ctf__mock_state *state,
const void * v,
                                               const char *v_print, uintmax_t step, int sign) {
  int ret = 1;
    intptr_t thread_index =                                                   \
      (intptr_t)pthread_getspecific(ctf__thread_index);               \
  for(uintmax_t i=0; i<state->check_count; i++) {
    if(!(state->check[i].type & CTF__MOCK_TYPE_MEMORY)) continue;
    if(strcmp(state->check[i].var, v_print)) continue;
    if(state->check[i].call_count != state->call_count) continue;
    ret = state->check[i].f.m( state->check[i].val.p, v, state->check[i].length, step, sign,
      state->check[i].print_var, v_print, state->check[i].line,
        state->check[i].file);
    if(!ret && state->check[i].type & CTF__MOCK_TYPE_ASSERT) {
    longjmp(ctf__assert_jmp_buff[thread_index], 1);
    }
  }
  mock_check_base(state, v_print, 1);
}')
*/
/* DEFINES
define(`MOCK', `MOCK_HELPER(`$1', TYPE(`$1'), SHORT(`$1'), 0)')
define(`MOCK_MEMORY', `MOCK_MEMORY_HELPER(`$1', `const void *', `p')')
define(`EXPECT_PRIMITIVE', `EXPECT_HELPER(`$1',`$2',TYPE(`$1'),CMP_SYMBOL(`$2'),FORMAT(`$1'),a,b)')
define(`EXPECT_STRING', `EXPECT_HELPER(`$1',`$2',TYPE(`$1'),CMP_SYMBOL(`$2'),FORMAT(`$1'),strcmp(a,b),0)')
define(`EXPECT_MEMORY', `EXPECT_MEMORY_HELPER(`$1',`$2',`$3',CMP_SYMBOL(`$2'),FORMAT(`$1'))')
define(`EXPECT_ARRAY', `EXPECT_ARRAY_HELPER(`$1',`$2',`$3',CMP_SYMBOL(`$2'),FORMAT(`$1'))')
define(`ASSERT_PRIMITIVE', `ASSERT_HELPER(`$1',`$2',TYPE(`$1'),CMP_SYMBOL(`$2'),FORMAT(`$1'),a,b)')
define(`ASSERT_STRING', `ASSERT_HELPER(`$1',`$2',TYPE(`$1'),CMP_SYMBOL(`$2'),FORMAT(`$1'),strcmp(a,b),0)')
define(`ASSERT_MEMORY', `ASSERT_MEMORY_HELPER(`$1',`$2',`$3',CMP_SYMBOL(`$2'),FORMAT(`$1'))')
define(`ASSERT_ARRAY', `ASSERT_ARRAY_HELPER(`$1',`$2',`$3',CMP_SYMBOL(`$2'),FORMAT(`$1'))')
define(`MOCK_CHECK', `MOCK_CHECK_HELPER(`$1', TYPE(`$1'), SHORT(`$1'))')
define(`MOCK_CHECK_MEMORY', `MOCK_CHECK_MEMORY_HELPER(`$1')')
*/
COMB2(`EXPECT_PRIMITIVE', `(PRIMITIVE_TYPES)', `(CMPS)')
COMB2(`EXPECT_STRING', `(str)', `(CMPS)')
COMB3(`EXPECT_MEMORY', `(char, int, uint)', `(CMPS)', `(const void *)')
COMB3(`EXPECT_MEMORY', `(ptr)', `(CMPS)', `(const void *)')
COMB3(`EXPECT_ARRAY', `(char, int, uint)', `(CMPS)', `(const void *)')
COMB3(`EXPECT_ARRAY', `(ptr)', `(CMPS)', `(const void *)')
COMB2(`ASSERT_PRIMITIVE', `(PRIMITIVE_TYPES)', `(CMPS)')
COMB2(`ASSERT_STRING', `(str)', `(CMPS)')
COMB3(`ASSERT_MEMORY', `(char, int, uint)', `(CMPS)', `(const void *)')
COMB3(`ASSERT_MEMORY', `(ptr)', `(CMPS)', `(const void *)')
COMB3(`ASSERT_ARRAY', `(char, int, uint)', `(CMPS)', `(const void *)')
COMB3(`ASSERT_ARRAY', `(ptr)', `(CMPS)', `(const void *)')
COMB(`MOCK', `(PRIMITIVE_TYPES)')
MOCK_STR
COMB(`MOCK_MEMORY', `(memory)')
COMB(`MOCK_CHECK', `(PRIMITIVE_TYPES)')
MOCK_CHECK_STR
COMB(`MOCK_CHECK_MEMORY', `(PRIMITIVE_TYPES)')
// clang-format on

