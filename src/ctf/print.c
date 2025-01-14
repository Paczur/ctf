#define DEFAULT_PRINT_BUFF_SIZE 1024
#define MIN_DIGITS_FOR_LINE 4
#define INDENT_PER_LEVEL 4
#define PRINT_INDICATOR_LENGTH 3

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

static uintmax_t print_indent(struct buff *buff, int count) {
  if(buff == NULL) return count;
  memset(buff->buff + buff->size, ' ', count);
  buff->size += count;
  return 0;
}

static uintmax_t print_indicator(struct buff *buff, int status) {
  static const char print_unknown_color[] = "\x1b[33m";
  // Padding required in order to replace it with fail indicators easily
  static const char print_unknown_branded[] = "\x1a\x1a?";
  static const char print_unknown_generic[] = "\x1a\x1a?";
  static const char print_unknown_off[] = "?";

  static const char print_fail_color[] = "\x1b[31m";
  static const char print_fail_branded[] = "⚑";
  static const char print_fail_generic[] = "✗";
  static const char print_fail_off[] = "F";

  static const char print_pass_color[] = "\x1b[32m";
  static const char print_pass_branded[] = "✓";
  static const char print_pass_generic[] = "✓";
  static const char print_pass_off[] = "P";

  uintmax_t full_size = 0;

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
  if(status == 0) {
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
  } else if(status == 1) {
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
  } else {
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
  }
  if(color) {
    strcpy(buff->buff + buff->size, print_color_reset);
    buff->size += sizeof(print_color_reset) - 1;
  }
  buff->buff[buff->size++] = ']';
  return 0;
}

static uintmax_t print_char(struct buff *buff, char c) {
  if(buff == NULL) return 1;
  buff->buff[buff->size++] = c;
  return 0;
}

static uintmax_t print_string(struct buff *buff, const char *s, uintmax_t len) {
  if(buff == NULL) return len;
  memcpy(buff->buff + buff->size, s, len);
  buff->size += len;
  return 0;
}

static uintmax_t print_raw_name_converted(struct buff *buff, const char *name,
                                          uintmax_t name_len) {
  uintmax_t last_index = 0;
  if(buff == NULL) return name_len;
  for(uintmax_t i = 0; i < name_len; i++) {
    if(name[i] == '_') {
      memcpy(buff->buff + buff->size, name + last_index, i - last_index);
      buff->size += i - last_index;
      buff->buff[buff->size++] = ' ';
      last_index = i + 1;
    }
  }
  memcpy(buff->buff + buff->size, name + last_index, name_len - last_index);
  buff->size += name_len - last_index;
  return 0;
}

static uintmax_t print_name_converted(struct buff *buff, const char *name,
                                      uintmax_t name_len) {
  uintmax_t full_size = 0;
  full_size += print_raw_name_converted(buff, name, name_len);
  if(buff == NULL) return full_size + 1;
  buff->buff[buff->size++] = '\n';
  return 0;
}

static uintmax_t find_last_word_boundary(const char *name, uintmax_t name_len) {
  for(uintmax_t i = name_len - 1; i > 0; i--) {
    if(name[i] == '_') return i;
  }
  return 0;
}

static uintmax_t print_wrapped_name(struct buff *buff, const char *name,
                                    uintmax_t name_len, uintmax_t indent) {
  uintmax_t after_indent;
  uintmax_t full_size = 0;
  uintmax_t index;
  after_indent = opt_wrap - indent;

  if(!(index = find_last_word_boundary(name, after_indent))) {
    index = after_indent - 1;
    full_size += print_raw_name_converted(buff, name, index);
    full_size += print_string(buff, "-\n", sizeof("-\n") - 1);
  } else {
    full_size += print_name_converted(buff, name, index);
  }
  name += index;
  name_len -= index;

  while(name_len > after_indent) {
    full_size += print_indent(buff, indent);
    if(!(index = find_last_word_boundary(name, after_indent))) {
      index = after_indent - 1;
      full_size += print_raw_name_converted(buff, name, index);
      full_size += print_string(buff, "-\n", sizeof("-\n") - 1);
    } else {
      full_size += print_name_converted(buff, name, index);
    }
    name += index;
    name_len -= index;
  }

  if(name_len % after_indent > 0) {
    full_size += print_indent(buff, indent);
    full_size += print_name_converted(buff, name, name_len % after_indent);
  }
  return full_size;
}

static uintmax_t print_name_status(struct buff *buff, const char *name,
                                   uintmax_t name_len, int status,
                                   uintmax_t level) {
  uintmax_t full_size = 0;
  const uintmax_t indent =
    level * INDENT_PER_LEVEL + PRINT_INDICATOR_LENGTH + 1;

  if(buff == NULL) {
    full_size += level * INDENT_PER_LEVEL;
    full_size += print_indicator(NULL, status);
    full_size++;  // ' ' char
    if(opt_wrap > 0 && full_size + name_len > opt_wrap) {
      full_size += print_wrapped_name(NULL, name, name_len, indent);
    } else {
      full_size += print_name_converted(NULL, name, name_len);
    }
    return full_size;
  }
  memset(buff->buff + buff->size, ' ', level * INDENT_PER_LEVEL);
  buff->size += level * INDENT_PER_LEVEL;
  print_indicator(buff, status);
  buff->buff[buff->size++] = ' ';

  if(opt_wrap > 0 && indent + name_len > opt_wrap) {
    print_wrapped_name(buff, name, name_len, indent);
  } else {
    print_name_converted(buff, name, name_len);
  }
  return 0;
}

static uintmax_t print_state(struct buff *buff, const struct ctf__state *state,
                             uintmax_t level, int ldetail) {
  uintmax_t spaces = level * INDENT_PER_LEVEL;
  uintmax_t full_size = 0;
  if(buff == NULL) {
    full_size += spaces;
    if(ldetail) {
      full_size +=
        snprintf(NULL, 0, "[%s|%0" STRINGIFY(MIN_DIGITS_FOR_LINE) "d",
                 state->file, state->line);
      if(color) {
        full_size += sizeof("\x1b[31mE\x1b[0m") - 1;
      } else {
        full_size++;  // 'E'
      }
      full_size++;  // ']'
    }
    full_size += print_indicator(NULL, state->status);
    full_size += 2;  // ' ' '\n' chars
    full_size += state->msg_size;
    return full_size;
  }
  if(state->status == 0) {
    if(ldetail) {
      spaces += snprintf(NULL, 0, "[%s|%0" STRINGIFY(MIN_DIGITS_FOR_LINE) "d",
                         state->file, state->line);
      spaces += 2;  // 'E' ']'
    }
    memset(buff->buff + buff->size, ' ', spaces);
    buff->size += spaces;
    print_indicator(buff, state->status);
  } else {
    memset(buff->buff + buff->size, ' ', spaces);
    buff->size += spaces;
    if(ldetail) {
      buff->size += sprintf(buff->buff + buff->size,
                            "[%s|%0" STRINGIFY(MIN_DIGITS_FOR_LINE) "d|",
                            state->file, state->line);
      if(color) {
        strcpy(buff->buff + buff->size, "\x1b[31mE\x1b[0m");
        buff->size += sizeof("\x1b[31mE\x1b[0m") - 1;
      } else {
        buff->buff[buff->size++] = 'E';
      }
      buff->buff[buff->size++] = ']';
    } else {
      print_indicator(buff, state->status);
    }
  }
  buff->buff[buff->size++] = ' ';
  memcpy(buff->buff + buff->size, state->msg, state->msg_size);
  buff->size += state->msg_size;
  buff->buff[buff->size++] = '\n';
  return 0;
}

static uintmax_t print_states(struct buff *buff,
                              const struct ctf__states *states, int level,
                              int parent_status) {
  uintmax_t full_size = 0;
  if(!parent_status && !states_status(states)) return 0;
  for(uintmax_t i = 0; i < states->size; i++)
    full_size += print_state(buff, states->states + i, level, detail);
  return full_size;
}

static uintmax_t print_subtest(struct buff *buff, uintptr_t thread_index,
                               struct ctf__subtest *subtest, int test_status) {
  uintmax_t full_size = 0;
  unsigned level = 2;
  unsigned t;
  const int status = subtest->status;
  if(subtest->size == 0 || (!test_status && !status && opt_verbosity < 3))
    return full_size;

  full_size += print_name_status(buff, subtest->name, strlen(subtest->name),
                                 subtest->status, 2);
  while(1) {
    if(subtest->size > 0) {
      for(uintmax_t i = subtest->size - 1; i > 0; i--) {
        if(subtest->elements[i].issubtest) {
          subtest_stack_push(subtest_stack + thread_index,
                             subtest->elements[i].el.subtest);
          level_stack_push(level_stack + thread_index, level + 1);
        } else {
          full_size += print_states(buff, subtest->elements[i].el.states,
                                    level + 1, subtest->status);
        }
      }
      if(subtest->elements[0].issubtest) {
        subtest_stack_push(subtest_stack + thread_index,
                           subtest->elements[0].el.subtest);
        level_stack_push(level_stack + thread_index, level + 1);
      } else {
        full_size += print_states(buff, subtest->elements[0].el.states,
                                  level + 1, subtest->status);
      }
    }

    subtest = subtest_stack_pop(subtest_stack + thread_index);
    if(subtest == NULL) break;
    t = level_stack_pop(level_stack + thread_index);
    if(subtest->size > 0 && (subtest->status || t < opt_verbosity ||
                             (subtest->parent && subtest->parent->status)))
      full_size += print_name_status(buff, subtest->name, strlen(subtest->name),
                                     subtest->status, t);
    level = t;
  }
  return full_size;
}

static uintmax_t print_test(struct buff *buff, const struct ctf__test *test,
                            uintmax_t test_name_len, uintptr_t thread_index) {
  struct ctf__thread_data *thread_data = ctf__thread_data + thread_index;
  uintmax_t full_size = 0;
  int status;
  struct ctf__test_element *el;

  status = test_status(thread_index);
  if(!status && opt_verbosity < 2) return full_size;

  full_size += print_name_status(buff, test->name, test_name_len, status, 1);
  for(uintmax_t i = 0; i < thread_data->test_elements_size; i++) {
    el = thread_data->test_elements + i;
    if(el->issubtest) {
      full_size += print_subtest(buff, thread_index, el->el.subtest, status);
    } else {
      full_size += print_states(buff, el->el.states, 2, status);
    }
  }
  return full_size;
}

void ctf__print_error(char *msg, uintmax_t msg_size, int line,
                      const char *file) {
  struct buff buff = {0};
  const struct ctf__state state = {.status = 1,
                                   .line = line,
                                   .file = file,
                                   .msg = msg,
                                   .msg_size = msg_size,
                                   .msg_capacity = msg_size};
  buff_reserve(&buff, print_state(NULL, &state, 0, 1));
  print_state(&buff, &state, 0, 1);
#pragma GCC diagnostic ignored "-Wunused-result"
  write(STDERR_FILENO, buff.buff, buff.size);
#pragma GCC diagnostic pop
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
  for(uintmax_t i = 0; i < thread_data->test_elements_size; i++) {
    if(thread_data->test_elements[i].issubtest) {
      print_subtest(print_buff + thread_index, thread_index,
                    thread_data->test_elements[i].el.subtest, 2);
    } else {
      print_states(print_buff + thread_index,
                   thread_data->test_elements[i].el.states, 1, 2);
    }
  }
  fflush(stdout);
  if(color) write(STDOUT_FILENO, err_color, sizeof(err_color));
  if(opt_unicode == OFF) {
    write(STDOUT_FILENO, postmsg, sizeof(postmsg));
  } else {
    write(STDOUT_FILENO, upostmsg, sizeof(upostmsg));
  }
  if(color) write(STDOUT_FILENO, print_color_reset, sizeof(print_color_reset));
#pragma GCC diagnostic pop
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
