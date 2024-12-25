#define MSG_SPRINTF_APPEND(msg, ...)                                        \
  do {                                                                      \
    const uintmax_t printf_size =                                           \
      msg##_size + snprintf(NULL, 0, __VA_ARGS__) + 1;                      \
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
#define SIGN(x) (((x) > 0) - ((x) < 0))
#define CMP(x, y) (((x) > (y)) - ((x) < (y)))

#define CTF__STRINGIFY2(x) #x
#define CTF__STRINGIFY(x) CTF__STRINGIFY2(x)

#define CTF__EA_FORMAT_INT "%jd"
#define CTF__EA_FORMAT_UINT "%ju"
#define CTF__EA_FORMAT_PTR "%p"
#define CTF__EA_FORMAT_FLOAT "%Lg"

#define CTF__EA_MEM_GENERIC(state, a, cmp, b, a_str, b_str, format)          \
  MSG_SPRINTF(state->msg, "%s %s %s (" format " %s " format ")", a_str, cmp, \
              b_str, a, cmp, b)
#define CTF__EA_MEM_INT(state, a, cmp, b, a_str, b_str) \
  CTF__EA_MEM_GENERIC(state, a, cmp, b, a_str, b_str, CTF__EA_FORMAT_INT)
#define CTF__EA_MEM_UINT(state, a, cmp, b, a_str, b_str) \
  CTF__EA_MEM_GENERIC(state, a, cmp, b, a_str, b_str, CTF__EA_FORMAT_UINT)
#define CTF__EA_MEM_CHAR(state, a, cmp, b, a_str, b_str)         \
  do {                                                           \
    MSG_SPRINTF((state)->msg, "%s %s %s ('", a_str, cmp, b_str); \
    escaped_char(state, a);                                      \
    MSG_SPRINTF_APPEND((state)->msg, "' %s '", cmp);             \
    escaped_char(state, b);                                      \
    MSG_SPRINTF_APPEND((state)->msg, "')");                      \
  } while(0)
#define CTF__EA_MEM_PTR(state, a, cmp, b, a_str, b_str) \
  CTF__EA_MEM_GENERIC(state, a, cmp, b, a_str, b_str, CTF__EA_FORMAT_PTR)
#define CTF__EA_MEM_FLOAT(state, a, cmp, b, a_str, b_str) \
  CTF__EA_MEM_GENERIC(state, a, cmp, b, a_str, b_str, CTF__EA_FORMAT_FLOAT)
#define CTF__EA_MEM_STR(state, a, cmp, b, a_str, b_str)           \
  do {                                                            \
    MSG_SPRINTF((state)->msg, "%s %s %s (\"", a_str, cmp, b_str); \
    escaped_string(state, a, strlen(a));                          \
    MSG_SPRINTF_APPEND((state)->msg, "\" %s \"", cmp);            \
    escaped_string(state, b, strlen(b));                          \
    MSG_SPRINTF_APPEND((state)->msg, "\")");                      \
  } while(0)

#define CTF__EA_ORD_INT(a, b) CMP(a, b)
#define CTF__EA_ORD_UINT(a, b) CMP(a, b)
#define CTF__EA_ORD_FLOAT(a, b) CMP(a, b)
#define CTF__EA_ORD_CHAR(a, b) CMP(a, b)
#define CTF__EA_ORD_PTR(a, b) CMP(a, b)
#define CTF__EA_ORD_STR(a, b) strcmp(a, b)

static void msg_reserve(struct ctf__state *state, uintmax_t size) {
  uintmax_t mul = state->msg_size + size / DEFAULT_STATE_MSG_CAPACITY + 1;
  mul += (mul % 2 != 0 && mul != 1);
  if(state->msg == NULL && state->msg_capacity == 0) {
    state->msg_capacity = DEFAULT_STATE_MSG_CAPACITY * mul;
    state->msg = malloc(state->msg_capacity);
  } else if(state->msg_size + size >= state->msg_capacity) {
    state->msg_capacity = DEFAULT_STATE_MSG_CAPACITY * mul;
    state->msg = realloc(state->msg, state->msg_capacity);
  }
}

static void escaped_char(struct ctf__state *state, char c) {
  msg_reserve(state, 4);
  if(c < ' ' || c == '\\' || c == '\'' || c >= 127) {
    state->msg[state->msg_size++] = '\\';
  } else {
    state->msg[state->msg_size++] = c;
    return;
  }
  switch(c) {
  case '\0':
    state->msg[state->msg_size++] = '0';
    break;
  case '\a':
    state->msg[state->msg_size++] = 'a';
    break;
  case '\b':
    state->msg[state->msg_size++] = 'b';
    break;
  case '\t':
    state->msg[state->msg_size++] = 't';
    break;
  case '\n':
    state->msg[state->msg_size++] = 'n';
    break;
  case '\v':
    state->msg[state->msg_size++] = 'v';
    break;
  case '\f':
    state->msg[state->msg_size++] = 'f';
    break;
  case '\r':
    state->msg[state->msg_size++] = 'r';
    break;
  case '\\':
    state->msg[state->msg_size++] = '\\';
    break;
  case '\'':
    state->msg[state->msg_size++] = '\'';
    break;
  default:
    state->msg[state->msg_size++] = c / 64 + '0';
    state->msg[state->msg_size++] = (c % 64) / 8 + '0';
    state->msg[state->msg_size++] = c % 8 + '0';
    break;
  }
}

static void escaped_string(struct ctf__state *state, const char *str,
                           uintmax_t str_len) {
  uintmax_t last = 0;
  for(uintmax_t i = 0; i < str_len; i++) {
    if(str[i] < ' ' || str[i] == '\\' || str[i] == '"' || str[i] >= 127) {
      msg_reserve(state, i - last + 4);
      memcpy(state->msg + state->msg_size, str + last, i - last);
      state->msg_size += i - last;
      last = i + 1;
      state->msg[state->msg_size++] = '\\';
      switch(str[i]) {
      case '\0':
        state->msg[state->msg_size++] = '0';
        break;
      case '\a':
        state->msg[state->msg_size++] = 'a';
        break;
      case '\b':
        state->msg[state->msg_size++] = 'b';
        break;
      case '\t':
        state->msg[state->msg_size++] = 't';
        break;
      case '\n':
        state->msg[state->msg_size++] = 'n';
        break;
      case '\v':
        state->msg[state->msg_size++] = 'v';
        break;
      case '\f':
        state->msg[state->msg_size++] = 'f';
        break;
      case '\r':
        state->msg[state->msg_size++] = 'r';
        break;
      case '\\':
        state->msg[state->msg_size++] = '\\';
        break;
      case '\'':
        state->msg[state->msg_size++] = '"';
        break;
      default:
        state->msg[state->msg_size++] = str[i] / 64 + '0';
        state->msg[state->msg_size++] = (str[i] % 64) / 8 + '0';
        state->msg[state->msg_size++] = str[i] % 8 + '0';
        break;
      }
    }
  }
  msg_reserve(state, str_len - last);
  memcpy(state->msg + state->msg_size, str + last, str_len - last);
  state->msg_size += str_len - last;
}

static void print_arr(struct ctf__state *state, const void *data,
                      uintmax_t size, uintmax_t step, int sign, int type) {
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
      const float *f;
      const double *lf;
      const long double *Lf;
    };
  } iterator;
  iterator.u8 = data;
  const char *format = (type == CTF__EA_MEM_TYPE_int)     ? "%jd, "
                       : (type == CTF__EA_MEM_TYPE_uint)  ? "%ju, "
                       : (type == CTF__EA_MEM_TYPE_ptr)   ? "%p, "
                       : (type == CTF__EA_MEM_TYPE_float) ? "%Lg, "
                                                          : "";
  if(sign == 2) {
    if(step == sizeof(float)) {
      for(uintmax_t i = 0; i < size; i++)
        MSG_SPRINTF_APPEND(state->msg, format, (long double)iterator.f[i]);
    } else if(step == sizeof(double)) {
      for(uintmax_t i = 0; i < size; i++)
        MSG_SPRINTF_APPEND(state->msg, format, (long double)iterator.lf[i]);
    } else if(step == sizeof(long double)) {
      for(uintmax_t i = 0; i < size; i++)
        MSG_SPRINTF_APPEND(state->msg, format, (long double)iterator.Lf[i]);
    }
  } else {
    switch(step) {
    case 1:
      if(type == CTF__EA_MEM_TYPE_char) {
        for(uintmax_t i = 0; i < size; i++) {
          MSG_SPRINTF_APPEND(state->msg, "%c", '\'');
          escaped_char(state, (char)iterator.u8[i]);
          MSG_SPRINTF_APPEND(state->msg, "%s", "', ");
        }
      } else if(sign) {
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
  }
  state->msg_size -= 2;
}

static int order_arr(const void *a, const void *b, uintmax_t la, uintmax_t lb,
                     uintmax_t step, int sign) {
  struct iterator {
    union {
      const int8_t *i8;
      const int16_t *i16;
      const int32_t *i32;
      const int64_t *i64;
      const uint8_t *u8;
      const uint16_t *u16;
      const uint32_t *u32;
      const uint64_t *u64;
      const float *f;
      const double *lf;
      const long double *Lf;
    };
  };
  struct iterator ia = *(struct iterator *)&a;
  struct iterator ib = *(struct iterator *)&b;
  int t;
  uintmax_t min = MIN(la, lb);
  if(sign == 2) {
    if(step == sizeof(float)) {
      for(uintmax_t i = 0; i < min; i++)
        if((t = CMP(ia.f[i], ib.f[i]))) return t;
    } else if(step == sizeof(double)) {
      for(uintmax_t i = 0; i < min; i++)
        if((t = CMP(ia.lf[i], ib.lf[i]))) return t;
    } else if(step == sizeof(long double)) {
      for(uintmax_t i = 0; i < min; i++)
        if((t = CMP(ia.Lf[i], ib.Lf[i]))) return t;
    }
  } else {
    switch(step) {
    case 1:
      if(sign) {
        for(uintmax_t i = 0; i < min; i++)
          if((t = CMP(ia.i8[i], ib.i8[i]))) return t;
      } else {
        for(uintmax_t i = 0; i < min; i++)
          if((t = CMP(ia.u8[i], ib.u8[i]))) return t;
      }
      break;
    case 2:
      if(sign) {
        for(uintmax_t i = 0; i < min; i++)
          if((t = CMP(ia.i16[i], ib.i16[i]))) return t;
      } else {
        for(uintmax_t i = 0; i < min; i++)
          if((t = CMP(ia.u16[i], ib.u16[i]))) return t;
      }
      break;
    case 4:
      if(sign) {
        for(uintmax_t i = 0; i < min; i++)
          if((t = CMP(ia.i32[i], ib.i32[i]))) return t;
      } else {
        for(uintmax_t i = 0; i < min; i++)
          if((t = CMP(ia.u32[i], ib.u32[i]))) return t;
      }
      break;
    case 8:
      if(sign) {
        for(uintmax_t i = 0; i < min; i++)
          if((t = CMP(ia.i64[i], ib.i64[i]))) return t;
      } else {
        for(uintmax_t i = 0; i < min; i++)
          if((t = CMP(ia.u64[i], ib.u64[i]))) return t;
      }
      break;
    }
  }
  return CMP(la, lb);
}

static void print_mem(struct ctf__state *state, const void *a, const char *cmp,
                      const void *b, uintmax_t la, uintmax_t lb, uintmax_t step,
                      int sign, const char *a_str, const char *b_str,
                      int type) {
  MSG_SPRINTF(state->msg, "%s %s %s ({", a_str, cmp, b_str);
  print_arr(state, a, la, step, sign, type);
  MSG_SPRINTF_APPEND(state->msg, "} %s {", cmp);
  print_arr(state, b, lb, step, sign, type);
  MSG_SPRINTF_APPEND(state->msg, "})");
}

void ctf__assert_fold(uintmax_t count, const char *msg, int line,
                      const char *file) {
  ctf_assert_hide(count);
  ctf__pass(msg, line, file);
}

void ctf_assert_hide(uintmax_t count) {
  intptr_t thread_index = (intptr_t)pthread_getspecific(ctf__thread_index);
  struct ctf__thread_data *thread_data = ctf__thread_data + thread_index;
  struct ctf__states *states = states_last(thread_data);
  if(states == NULL) return;
  if(states->size <= count) {
    for(uintmax_t i = 0; i < states->size; i++) {
      if(states->states[i].status) {
        longjmp(ctf__assert_jmp_buff[thread_index], 1);
      }
    }
    states_delete(states);
  } else {
    for(uintmax_t i = states->size - count; i < states->size; i++) {
      if(states->states[i].status) {
        longjmp(ctf__assert_jmp_buff[thread_index], 1);
      }
    }
    states->size -= count;
  }
}

void ctf_assert_barrier(void) {
  intptr_t thread_index = (intptr_t)pthread_getspecific(ctf__thread_index);
  struct ctf__thread_data *thread_data = ctf__thread_data + thread_index;
  struct ctf__states *states = states_last(thread_data);
  for(uintmax_t i = 0; i < states->size; i++) {
    if(states->states[i].status) {
      longjmp(ctf__assert_jmp_buff[thread_index], 1);
    }
  }
}

uintmax_t ctf__fail(const char *m, int line, const char *file, ...) {
  va_list v;
  intptr_t thread_index = (intptr_t)pthread_getspecific(ctf__thread_index);
  struct ctf__thread_data *thread_data = ctf__thread_data + thread_index;
  struct ctf__state *state = state_next(thread_data);
  state->status = 1;
  state->line = line;
  state->file = file;
  va_start(v, file);
  MSG_VSPRINTF(state->msg, m, v);
  va_end(v);
  return 0;
}

uintmax_t ctf__pass(const char *m, int line, const char *file, ...) {
  va_list v;
  intptr_t thread_index = (intptr_t)pthread_getspecific(ctf__thread_index);
  struct ctf__thread_data *thread_data = ctf__thread_data + thread_index;
  struct ctf__state *state = state_next(thread_data);
  state->status = 0;
  state->line = line;
  state->file = file;
  va_start(v, file);
  MSG_VSPRINTF(state->msg, m, v);
  va_end(v);
  return 1;
}

uintmax_t ctf__assert_msg(int status, const char *msg, int line,
                          const char *file, ...) {
  va_list args;
  intptr_t thread_index = (intptr_t)pthread_getspecific(ctf__thread_index);
  struct ctf__thread_data *thread_data = ctf__thread_data + thread_index;
  struct ctf__state *state = state_next(thread_data);
  state->status = status;
  state->line = line;
  state->file = file;
  state->msg_size = 0;
  va_start(args, file);
  if(status) {
    thread_data->stats.asserts_passed++;
  } else {
    thread_data->stats.asserts_failed++;
  }
  MSG_VSPRINTF(state->msg, msg, args);
  va_end(args);
  return status;
}

static int ea_end(intptr_t thread_index, int status, int assert) {
  struct ctf__thread_data *thread_data = ctf__thread_data + thread_index;
  if(assert) {
    if(opt_statistics) {
      if(!status) {
        thread_data->stats.asserts_passed++;
      } else {
        thread_data->stats.asserts_failed++;
        longjmp(ctf__assert_jmp_buff[thread_index], 1);
      }
    }
  } else {
    if(opt_statistics) {
      if(!status) {
        thread_data->stats.expects_passed++;
      } else {
        thread_data->stats.expects_failed++;
      }
    }
  }
  return status;
}

#define EA(type, TYPE)                                                        \
  int ctf__ea_##type(CTF__EA_TYPE_##TYPE a, const char *cmp,                  \
                     CTF__EA_TYPE_##TYPE b, const char *a_str,                \
                     const char *b_str, int assert, int line,                 \
                     const char *file) {                                      \
    int ord;                                                                  \
    intptr_t thread_index = (intptr_t)pthread_getspecific(ctf__thread_index); \
    struct ctf__thread_data *thread_data = ctf__thread_data + thread_index;   \
    struct ctf__state *state = state_next(thread_data);                       \
    state->status = 2;                                                        \
    state->line = line;                                                       \
    state->file = file;                                                       \
    CTF__EA_MEM_##TYPE(state, a, cmp, b, a_str, b_str);                       \
    ord = CTF__EA_ORD_##TYPE(a, b);                                           \
    state->status =                                                           \
      !(ord != 0 && (cmp[0] == '!' || (ord > 0 && cmp[0] == '>') ||           \
                     (ord < 0 && cmp[0] == '<'))) &&                          \
      !(ord == 0 && cmp[0] != '!' && cmp[1] == '=');                          \
    return ea_end(thread_index, state->status, assert);                       \
  }

int ctf__ea_arr(const void *a, const char *cmp, const void *b, uintmax_t la,
                uintmax_t lb, const char *a_str, const char *b_str, int assert,
                uintmax_t step, int sign, int type, int line,
                const char *file) {
  int ord;
  intptr_t thread_index = (intptr_t)pthread_getspecific(ctf__thread_index);
  struct ctf__thread_data *thread_data = ctf__thread_data + thread_index;
  struct ctf__state *state = state_next(thread_data);
  state->status = 2;
  state->line = line;
  state->file = file;
  print_mem(state, a, cmp, b, la, lb, step, sign, a_str, b_str, type);
  ord = order_arr(a, b, la, lb, step, sign);
  state->status = !(ord != 0 && (cmp[0] == '!' || (ord > 0 && cmp[0] == '>') ||
                                 (ord < 0 && cmp[0] == '<'))) &&
                  !(ord == 0 && cmp[0] != '!' && cmp[1] == '=');
  return ea_end(thread_index, state->status, assert);
}

// clang-format off
COMB2(`RUN1', `(EA)', `(PRIMITIVE_TYPES, str)')
// clang-format on
