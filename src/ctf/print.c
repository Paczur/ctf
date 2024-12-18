#define DEFAULT_PRINT_BUFF_SIZE 1024
#define MIN_DIGITS_FOR_LINE 4
#define INDENT_PER_LEVEL 4

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

static uintmax_t print_name_status(struct buff *buff, const char *name,
                                   uintmax_t name_len, int status,
                                   uintmax_t level) {
  uintmax_t full_size = 0;
  if(buff == NULL) {
    full_size += level * INDENT_PER_LEVEL;
    if(status != 0 || (int)level < opt_verbosity) {
      full_size += print_indicator(NULL, status);
      full_size += 2;  // ' ' '\n' chars
      full_size += name_len;
    }
    return full_size;
  }
  if(status != 0 || (int)level < opt_verbosity) {
    memset(buff->buff + buff->size, ' ', level * INDENT_PER_LEVEL);
    buff->size += level * INDENT_PER_LEVEL;
    print_indicator(buff, status);
    buff->buff[buff->size++] = ' ';
    memcpy(buff->buff + buff->size, name, name_len);
    buff->size += name_len;
    buff->buff[buff->size++] = '\n';
  }
  return 0;
}

static uintmax_t print_state(struct buff *buff, const struct ctf__state *state,
                             uintmax_t level) {
  uintmax_t spaces = level * INDENT_PER_LEVEL;
  uintmax_t full_size = 0;
  if(buff == NULL) {
    full_size += spaces;
    if(detail) {
      full_size +=
        snprintf(NULL, 0, "[%s|%0" STRINGIFY(MIN_DIGITS_FOR_LINE) "d",
                 state->file, state->line);
      if(color) {
        full_size += sizeof("\x1b[31mE\x1b[0m") - 1;
      } else {
        full_size++;  // 'E'
      }
      full_size++;  // ']'
    } else {
      full_size += print_indicator(NULL, state->status);
    }
    full_size += 2;  // ' ' '\n' chars
    full_size += state->msg_size;
    return full_size;
  }
  if(state->status == 0) {
    if(detail) {
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
                              const struct ctf__states *states, int level) {
  uintmax_t full_size = 0;
  if(level < opt_verbosity || states_status(states)) {
    for(uintmax_t i = 0; i < states->size; i++)
      full_size += print_state(buff, states->states + i, level);
  }
  return full_size;
}

static uintmax_t print_subtest(struct buff *buff,
                               struct ctf__subtest *subtest) {
  uintmax_t full_size = 0;
  uintmax_t level = 2;
  uintmax_t t;
  do {
    for(uintmax_t i = 0; i < subtest->size; i++) {
      if(subtest->elements[i].issubtest) {
        subtest_stack_push(subtest->elements[i].el.subtest);
        level_stack_push(level + 1);
      } else {
        full_size += print_states(buff, subtest->elements[i].el.states, level);
      }
    }
    subtest = subtest_stack_pop();
    t = level_stack_pop();
    if(t > level) {
      full_size += print_name_status(buff, subtest->name, strlen(subtest->name),
                                     subtest->status, t);
    }
    level = t;
  } while(subtest != NULL);
  return full_size;
}

static uintmax_t print_test(struct buff *buff, const struct ctf__test *test,
                            uintmax_t test_name_len,
                            struct ctf__thread_data *thread_data) {
  uintmax_t full_size = 0;
  int status;
  struct ctf__test_element *el;

  status = test_status(thread_data);

  if(status || opt_verbosity >= 2) {
    full_size += print_name_status(buff, test->name, test_name_len, status, 1);
    for(uintmax_t i = 0; i < thread_data->test_elements_size; i++) {
      el = thread_data->test_elements + i;
      if(el->issubtest) {
        full_size += print_subtest(buff, el->el.subtest);
      } else {
        full_size += print_states(buff, el->el.states, 2);
      }
    }
  }
  return full_size;
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
      print_subtest(print_buff + thread_index,
                    thread_data->test_elements[i].el.subtest);
    } else {
      print_states(print_buff + thread_index,
                   thread_data->test_elements[i].el.states, 1);
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
