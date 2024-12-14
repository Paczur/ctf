#define DEFAULT_PRINT_BUFF_SIZE 1024
#define MIN_DIGITS_FOR_LINE 4

#define MSG_SPRINTF_APPEND(msg, ...)                                        \
  do {                                                                      \
    const uintmax_t printf_size = snprintf(NULL, 0, __VA_ARGS__) + 1;       \
    uintmax_t mul = printf_size / DEFAULT_STATE_MSG_CAPACITY + 1;           \
    mul += (mul % 2 != 0 && mul != 1);                                      \
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
    mul += (mul % 2 != 0 && mul != 1);                                \
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
    mul += (mul % 2 != 0 && mul != 1);                            \
    if(msg == NULL) {                                             \
      msg##_capacity = DEFAULT_STATE_MSG_CAPACITY * mul;          \
      msg = malloc(msg##_capacity);                               \
    } else if(printf_size >= msg##_capacity) {                    \
      msg##_capacity = DEFAULT_STATE_MSG_CAPACITY * mul;          \
      msg = realloc(msg, msg##_capacity);                         \
    }                                                             \
    msg##_size = vsnprintf(msg, msg##_capacity, f, vc);           \
  } while(0)

struct buff {
  char *buff;
  uintmax_t size;
  uintmax_t capacity;
};

static const char print_color_reset[] = "\x1b[0m";
static int color = 0;
static int detail = 1;

static pthread_mutex_t parallel_print_mutex;
static struct buff *restrict print_buff;

static void buff_resize(struct buff *buff, uintmax_t cap) {
  uintmax_t mul = (cap + 1) / DEFAULT_PRINT_BUFF_SIZE + 1;
  mul += (mul % 2 != 0 && mul != 1);
  buff->capacity = DEFAULT_PRINT_BUFF_SIZE * mul;
  buff->buff = realloc(buff->buff, buff->capacity);
}

static void buff_reserve(struct buff *buff, uintmax_t size) {
  if(buff->size + size >= buff->capacity) buff_resize(buff, buff->size + size);
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

static uintmax_t print_group_pass(struct buff *buff, const char *name,
                                  uintmax_t name_len) {
  uintmax_t full_size = 0;
  if(buff == NULL) {
    if(opt_verbosity != 0) {
      full_size += print_pass_indicator(NULL);
      full_size += 2;  // ' ' '\n' chars
      full_size += name_len;
    }
    return full_size;
  }
  if(opt_verbosity != 0) {
    print_pass_indicator(buff);
    buff->size += sprintf(buff->buff + buff->size, " %s", name);
    buff->buff[buff->size++] = '\n';
  }
  return 0;
}

static uintmax_t print_group_fail(struct buff *buff, const char *name,
                                  uintmax_t name_len) {
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

static uintmax_t print_group_unknown(struct buff *buff, const char *name,
                                     uintmax_t name_len) {
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

static uintmax_t print_test_fail(struct buff *buff, const char *name,
                                 uintmax_t name_len) {
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

static uintmax_t print_test_unknown(struct buff *buff, const char *name,
                                    uintmax_t name_len) {
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

static uintmax_t print_test_unknown_info(struct buff *buff,
                                         struct ctf__state *state) {
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

static void print_arr(struct ctf__state *state, const void *data,
                      uintmax_t size, uintmax_t step, int sign,
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

static void print_mem(struct ctf__state *state, const void *a, const void *b,
                      uintmax_t la, uintmax_t lb, uintmax_t step, int sign,
                      const char *a_str, const char *b_str, const char *op_str,
                      const char *format) {
  int status = memcmp(a, b, MIN(la, lb) * step);
  MSG_SPRINTF(state->msg, "%s %s %s ({", a_str, op_str, b_str);
  print_arr(state, a, la, step, sign, format);
  MSG_SPRINTF_APPEND(state->msg, "} %s {", op_str);
  print_arr(state, b, lb, step, sign, format);
  MSG_SPRINTF_APPEND(state->msg, "})");
  state->status = status;
}

void ctf_sigsegv_handler(int unused) {
  (void)unused;
  intptr_t thread_index = (intptr_t)pthread_getspecific(ctf__thread_index);
  struct ctf__thread_data *thread_data = ctf__thread_data + thread_index;
  const char err_color[] = "\x1b[33m";
  const char upremsg[] =
    "────────────────SIGSEGV─────────────────\n"
    "─────────────BUFFER─STATE───────────────\n";
  const char upostmsg[] = "─────────────BUFFER─STATE───────────────\n";
  const char premsg[] =
    "----------------SIGSEGV-----------------\n"
    "-------------BUFFER-STATE---------------\n";
  const char postmsg[] = "-------------BUFFER-STATE---------------\n";
#pragma GCC diagnostic ignored "-Wunused-result"
  if(color) write(STDOUT_FILENO, err_color, sizeof(err_color));
  if(opt_unicode == OFF) {
    write(STDOUT_FILENO, premsg, sizeof(premsg));
  } else {
    write(STDOUT_FILENO, upremsg, sizeof(upremsg));
  }
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
  if(opt_unicode == OFF) {
    write(STDOUT_FILENO, postmsg, sizeof(postmsg));
  } else {
    write(STDOUT_FILENO, upostmsg, sizeof(upostmsg));
  }
  if(color) write(STDOUT_FILENO, print_color_reset, sizeof(print_color_reset));
#pragma GCC diagnostic pop
  fflush(stdout);
}

static void print_stats(struct ctf__stats *stats) {
  const char fail_color[] = "\x1b[31m";
  const char pass_color[] = "\x1b[32m";
  const char upremsg[] = "──Groups────Tests────Asserts───Expects──\n";
  const char premsg[] = "--Groups----Tests----Asserts---Expects--\n";
#pragma GCC diagnostic ignored "-Wunused-result"
  if(opt_unicode == OFF) {
    write(STDOUT_FILENO, premsg, sizeof(premsg));
  } else {
    write(STDOUT_FILENO, upremsg, sizeof(upremsg));
  }
  if(color) {
    if(stats->groups_failed == 0) {
      write(STDOUT_FILENO, pass_color, sizeof(pass_color));
    } else {
      write(STDOUT_FILENO, fail_color, sizeof(fail_color));
    }
  }
  printf("%4lu/%-4lu ", stats->groups_passed,
         stats->groups_passed + stats->groups_failed);
  if(color) {
    fflush(stdout);
    if(stats->tests_failed == 0) {
      write(STDOUT_FILENO, pass_color, sizeof(pass_color));
    } else {
      write(STDOUT_FILENO, fail_color, sizeof(fail_color));
    }
  }
  printf("%4lu/%-4lu ", stats->tests_passed,
         stats->tests_passed + stats->tests_failed);
  if(color) {
    fflush(stdout);
    if(stats->asserts_failed == 0) {
      write(STDOUT_FILENO, pass_color, sizeof(pass_color));
    } else {
      write(STDOUT_FILENO, fail_color, sizeof(fail_color));
    }
  }
  printf("%4lu/%-4lu ", stats->asserts_passed,
         stats->asserts_passed + stats->asserts_failed);
  if(color) {
    fflush(stdout);
    if(stats->expects_failed == 0) {
      write(STDOUT_FILENO, pass_color, sizeof(pass_color));
    } else {
      write(STDOUT_FILENO, fail_color, sizeof(fail_color));
    }
  }
  printf("%4lu/%-4lu \n", stats->expects_passed,
         stats->expects_passed + stats->expects_failed);
  if(color) {
    fflush(stdout);
    write(STDOUT_FILENO, print_color_reset, sizeof(print_color_reset));
  }
#pragma GCC diagnostic pop
}
