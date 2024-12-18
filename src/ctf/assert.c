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

#define CTF__STRINGIFY2(x) #x
#define CTF__STRINGIFY(x) CTF__STRINGIFY2(x)

#define CTF__ASSERT_TYPE_INT intmax_t
#define CTF__ASSERT_TYPE_UINT uintmax_t
#define CTF__ASSERT_TYPE_CHAR char
#define CTF__ASSERT_TYPE_PTR const void *
#define CTF__ASSERT_TYPE_STR const char *

#define CTF__ASSERT_CMP_EQ ==
#define CTF__ASSERT_CMP_NEQ !=
#define CTF__ASSERT_CMP_GT >
#define CTF__ASSERT_CMP_LT <
#define CTF__ASSERT_CMP_GTE >=
#define CTF__ASSERT_CMP_LTE <=

#define CTF__ASSERT_FORMAT_INT "%jd"
#define CTF__ASSERT_FORMAT_UINT "%ju"
#define CTF__ASSERT_FORMAT_CHAR "'%c'"
#define CTF__ASSERT_FORMAT_PTR "%p"
#define CTF__ASSERT_FORMAT_STR "\"%s\""

#define CTF__ASSERT_FCMP_INT(a, eq, b) ((a)eq(b))
#define CTF__ASSERT_FCMP_UINT(a, eq, b) ((a)eq(b))
#define CTF__ASSERT_FCMP_CHAR(a, eq, b) ((a)eq(b))
#define CTF__ASSERT_FCMP_PTR(a, eq, b) ((a)eq(b))
#define CTF__ASSERT_FCMP_STR(a, eq, b) (strcmp((a), (b)) eq 0)

#define CTF__ASSERT_JMP_ASSERT(status, thread_index) \
  if(!(status)) longjmp(ctf__assert_jmp_buff[thread_index], 1)
#define CTF__ASSERT_JMP_EXPECT(status, thread_index)

#define CTF__ASSERT_PRINT_GENERIC(TYPE, CMP, state, a, b, a_str, b_str)   \
  MSG_SPRINTF((state)->msg,                                               \
              "%s " CTF__STRINGIFY(                                       \
                CTF__ASSERT_CMP_##CMP) " %s (" CTF__ASSERT_FORMAT_##TYPE  \
              " " CTF__STRINGIFY(                                         \
                CTF__ASSERT_CMP_##CMP) " " CTF__ASSERT_FORMAT_##TYPE ")", \
              a_str, b_str, a, b)
#define CTF__ASSERT_PRINT_INT(CMP, state, a, b, a_str, b_str) \
  CTF__ASSERT_PRINT_GENERIC(INT, CMP, state, a, b, a_str, b_str)
#define CTF__ASSERT_PRINT_UINT(CMP, state, a, b, a_str, b_str) \
  CTF__ASSERT_PRINT_GENERIC(UINT, CMP, state, a, b, a_str, b_str)
#define CTF__ASSERT_PRINT_PTR(CMP, state, a, b, a_str, b_str) \
  CTF__ASSERT_PRINT_GENERIC(PTR, CMP, state, a, b, a_str, b_str)
#define CTF__ASSERT_PRINT_STR(CMP, state, a, b, a_str, b_str)                 \
  do {                                                                        \
    MSG_SPRINTF((state)->msg,                                                 \
                "%s " CTF__STRINGIFY(CTF__ASSERT_CMP_##CMP) " %s (\"", a_str, \
                b_str);                                                       \
    escaped_string(state, a, strlen(a));                                      \
    MSG_SPRINTF_APPEND((state)->msg,                                          \
                       "\" " CTF__STRINGIFY(CTF__ASSERT_CMP_##CMP) " \"");    \
    escaped_string(state, b, strlen(b));                                      \
    MSG_SPRINTF_APPEND((state)->msg, "\")");                                  \
  } while(0)
#define CTF__ASSERT_PRINT_CHAR(CMP, state, a, b, a_str, b_str)               \
  do {                                                                       \
    MSG_SPRINTF((state)->msg,                                                \
                "%s " CTF__STRINGIFY(CTF__ASSERT_CMP_##CMP) " %s ('", a_str, \
                b_str);                                                      \
    escaped_char(state, a);                                                  \
    MSG_SPRINTF_APPEND((state)->msg,                                         \
                       "' " CTF__STRINGIFY(CTF__ASSERT_CMP_##CMP) " '");     \
    escaped_char(state, b);                                                  \
    MSG_SPRINTF_APPEND((state)->msg, "')");                                  \
  } while(0)

#define EA_GEN(ea, EA, type, TYPE, cmp, CMP)                                  \
  int ctf__##ea##_##type##_##cmp(                                             \
    CTF__ASSERT_TYPE_##TYPE a, CTF__ASSERT_TYPE_##TYPE b, const char *a_str,  \
    const char *b_str, int line, const char *file) {                          \
    int status;                                                               \
    intptr_t thread_index = (intptr_t)pthread_getspecific(ctf__thread_index); \
    struct ctf__thread_data *thread_data = ctf__thread_data + thread_index;   \
    thread_data_states_increment(thread_data);                                \
    thread_data->states[thread_data->states_size - 1].status = 2;             \
    assert_copy(thread_data->states + thread_data->states_size - 1, line,     \
                file);                                                        \
    CTF__ASSERT_PRINT_##TYPE(                                                 \
      CMP, thread_data->states + thread_data->states_size - 1, a, b, a_str,   \
      b_str);                                                                 \
    status = CTF__ASSERT_FCMP_##TYPE(a, CTF__ASSERT_CMP_##CMP, b);            \
    thread_data->states[thread_data->states_size - 1].status = !status;       \
    if(status) {                                                              \
      thread_data->stats.expects_passed++;                                    \
    } else {                                                                  \
      thread_data->stats.expects_failed++;                                    \
    }                                                                         \
    CTF__ASSERT_JMP_##EA(status, thread_index);                               \
    return status;                                                            \
  }
#define EA_MEM_GEN(ea, EA, cmp, CMP)                                           \
  int ctf__##ea##_memory_##cmp(const void *a, const void *b, uintmax_t l,      \
                               uintmax_t step, int sign, int type,             \
                               const char *a_str, const char *b_str, int line, \
                               const char *file) {                             \
    int status;                                                                \
    intptr_t thread_index = (intptr_t)pthread_getspecific(ctf__thread_index);  \
    struct ctf__thread_data *thread_data = ctf__thread_data + thread_index;    \
    thread_data_states_increment(thread_data);                                 \
    thread_data->states[thread_data->states_size - 1].status = 2;              \
    assert_copy(thread_data->states + thread_data->states_size - 1, line,      \
                file);                                                         \
    print_mem(thread_data->states + thread_data->states_size - 1, a, b, l, l,  \
              step, sign, a_str, b_str, CTF__STRINGIFY(CTF__ASSERT_CMP_##CMP), \
              type);                                                           \
    status = thread_data->states[thread_data->states_size - 1]                 \
               .status CTF__ASSERT_CMP_##CMP 0;                                \
    thread_data->states[thread_data->states_size - 1].status = !status;        \
    if(status) {                                                               \
      thread_data->stats.expects_passed++;                                     \
    } else {                                                                   \
      thread_data->stats.expects_failed++;                                     \
    }                                                                          \
    CTF__ASSERT_JMP_##EA(status, thread_index);                                \
    return status;                                                             \
  }
#define EA_ARR_GEN(ea, EA, cmp, CMP)                                          \
  int ctf__##ea##_array_##cmp(const void *a, const void *b, uintmax_t la,     \
                              uintmax_t lb, uintmax_t step, int sign,         \
                              int type, const char *a_str, const char *b_str, \
                              int line, const char *file) {                   \
    int status;                                                               \
    intptr_t thread_index = (intptr_t)pthread_getspecific(ctf__thread_index); \
    struct ctf__thread_data *thread_data = ctf__thread_data + thread_index;   \
    thread_data_states_increment(thread_data);                                \
    thread_data->states[thread_data->states_size - 1].status = 2;             \
    assert_copy(thread_data->states + thread_data->states_size - 1, line,     \
                file);                                                        \
    print_mem(thread_data->states + thread_data->states_size - 1, a, b, la,   \
              lb, step, sign, a_str, b_str,                                   \
              CTF__STRINGIFY(CTF__ASSERT_CMP_##CMP), type);                   \
    if(thread_data->states[thread_data->states_size - 1].status == 0) {       \
      status = (la CTF__ASSERT_CMP_##CMP lb);                                 \
    } else {                                                                  \
      status = (thread_data->states[thread_data->states_size - 1]             \
                  .status CTF__ASSERT_CMP_##CMP 0);                           \
    }                                                                         \
    thread_data->states[thread_data->states_size - 1].status = !status;       \
    if(status) {                                                              \
      thread_data->stats.expects_passed++;                                    \
    } else {                                                                  \
      thread_data->stats.expects_failed++;                                    \
    }                                                                         \
    CTF__ASSERT_JMP_##EA(status, thread_index);                               \
    return status;                                                            \
  }

static void msg_reserve(struct ctf__state *state, uintmax_t size) {
  uintmax_t mul = state->msg_size + size / DEFAULT_STATE_MSG_CAPACITY + 1;
  mul += (mul % 2 != 0 && mul != 1);
  if(state->msg == NULL) {
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
  msg_reserve(state, str_len - last + 4);
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
    };
  } iterator;
  iterator.u8 = data;
  const char *format = (type == CTF__ASSERT_PRINT_TYPE_int)    ? "%jd, "
                       : (type == CTF__ASSERT_PRINT_TYPE_uint) ? "%ju, "
                       : (type == CTF__ASSERT_PRINT_TYPE_ptr)  ? "%p, "
                                                               : "";
  switch(step) {
  case 1:
    if(type == CTF__ASSERT_PRINT_TYPE_char) {
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
    };
  };
  struct iterator ia = *(struct iterator *)&a;
  struct iterator ib = *(struct iterator *)&b;
  uintmax_t min = MIN(la, lb);
  switch(step) {
  case 1:
    if(sign) {
      for(uintmax_t i = 0; i < min; i++) {
        if(ia.i8[i] < ib.i8[i]) {
          return -1;
        } else if(ia.i8[i] > ib.i8[i]) {
          return 1;
        }
      }
    } else {
      for(uintmax_t i = 0; i < min; i++) {
        if(ia.u8[i] < ib.u8[i]) {
          return -1;
        } else if(ia.u8[i] > ib.u8[i]) {
          return 1;
        }
      }
    }
    break;
  case 2:
    if(sign) {
      for(uintmax_t i = 0; i < min; i++) {
        if(ia.i16[i] < ib.i16[i]) {
          return -1;
        } else if(ia.i16[i] > ib.i16[i]) {
          return 1;
        }
      }
    } else {
      for(uintmax_t i = 0; i < min; i++) {
        if(ia.u16[i] < ib.u16[i]) {
          return -1;
        } else if(ia.u16[i] > ib.u16[i]) {
          return 1;
        }
      }
    }
    break;
  case 4:
    if(sign) {
      for(uintmax_t i = 0; i < min; i++) {
        if(ia.i32[i] < ib.i32[i]) {
          return -1;
        } else if(ia.i32[i] > ib.i32[i]) {
          return 1;
        }
      }
    } else {
      for(uintmax_t i = 0; i < min; i++) {
        if(ia.u32[i] < ib.u32[i]) {
          return -1;
        } else if(ia.u32[i] > ib.u32[i]) {
          return 1;
        }
      }
    }
    break;
  case 8:
    if(sign) {
      for(uintmax_t i = 0; i < min; i++) {
        if(ia.i64[i] < ib.i64[i]) {
          return -1;
        } else if(ia.i64[i] > ib.i64[i]) {
          return 1;
        }
      }
    } else {
      for(uintmax_t i = 0; i < min; i++) {
        if(ia.u64[i] < ib.u64[i]) {
          return -1;
        } else if(ia.u64[i] > ib.u64[i]) {
          return 1;
        }
      }
    }
    break;
  }
  if(la < lb) return -1;
  if(la > lb) return 1;
  return 0;
}

static void print_mem(struct ctf__state *state, const void *a, const void *b,
                      uintmax_t la, uintmax_t lb, uintmax_t step, int sign,
                      const char *a_str, const char *b_str, const char *op_str,
                      int type) {
  int status = order_arr(a, b, la, lb, step, sign);
  MSG_SPRINTF(state->msg, "%s %s %s ({", a_str, op_str, b_str);
  print_arr(state, a, la, step, sign, type);
  MSG_SPRINTF_APPEND(state->msg, "} %s {", op_str);
  print_arr(state, b, lb, step, sign, type);
  MSG_SPRINTF_APPEND(state->msg, "})");
  state->status = status;
}

void ctf__assert_fold(uintmax_t count, const char *msg, int line,
                      const char *file) {
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
  if(status) {
    thread_data->stats.asserts_passed++;
  } else {
    thread_data->stats.asserts_failed++;
  }
  MSG_VSPRINTF(thread_data->states[thread_data->states_size - 1].msg, msg,
               args);
  va_end(args);
  return status;
}

// clang-format off
COMB4(`RUN3', `(EA_GEN)', `(expect, assert)', `(PRIMITIVE_TYPES, str)', `(CMPS)')
COMB3(`RUN2', `(EA_MEM_GEN)', `(expect, assert)', `(CMPS)')
COMB3(`RUN2', `(EA_ARR_GEN)', `(expect, assert)', `(CMPS)')
// clang-format on
