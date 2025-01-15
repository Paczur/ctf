#define DEFAULT_MOCK_RETURN_CAPACITY 16
#define DEFAULT_MOCK_STATE_CHECKS_CAPACITY 16

#define CTF__MOCK_CHECK_TYPE_INT i
#define CTF__MOCK_CHECK_TYPE_UINT u
#define CTF__MOCK_CHECK_TYPE_PTR p
#define CTF__MOCK_CHECK_TYPE_CHAR c
#define CTF__MOCK_CHECK_TYPE_STR s
#define CTF__MOCK_CHECK_TYPE_FLOAT f

#define CTF__MOCK_CHECK_TYPE_TYPE_INT 0
#define CTF__MOCK_CHECK_TYPE_TYPE_UINT 0
#define CTF__MOCK_CHECK_TYPE_TYPE_PTR 0
#define CTF__MOCK_CHECK_TYPE_TYPE_CHAR 0
#define CTF__MOCK_CHECK_TYPE_TYPE_FLOAT 0
#define CTF__MOCK_CHECK_TYPE_TYPE_STR CTF__MOCK_TYPE_MEM

#define CTF__MOCK_CHECK_STR_INT
#define CTF__MOCK_CHECK_STR_UINT
#define CTF__MOCK_CHECK_STR_PTR
#define CTF__MOCK_CHECK_STR_CHAR
#define CTF__MOCK_CHECK_STR_FLOAT
#define CTF__MOCK_CHECK_STR_STR !

static pthread_rwlock_t mock_states_lock = PTHREAD_RWLOCK_INITIALIZER;
pthread_rwlock_t ctf__mock_returns_lock = PTHREAD_RWLOCK_INITIALIZER;

static void mock_state_init(struct ctf__mock_state *state) {
  state->mock_f = NULL;
  state->call_count = 0;
  state->checks_count = 0;
  state->checks_capacity = 0;
  state->checks = NULL;
  state->return_overrides = NULL;
  state->return_overrides_size = 0;
  state->return_overrides_capacity = 0;
}

void ctf__mock_reset(struct ctf__mock_state *state) {
  state->call_count = 0;
  state->checks_count = 0;
  if(state->return_overrides)
    memset(state->return_overrides, -1,
           state->return_overrides_size * sizeof(state->return_overrides[0]));
  state->return_overrides_size = 0;
}

static void mock_return_init(struct ctf__mock_return *ret) {
  ret->values = NULL;
  ret->size = 0;
  ret->capacity = 0;
}

static void mock_states_alloc(struct ctf__mock *mock, int thread_index) {
  if(ctf__opt_threads > 1) {
    while(!mock->states_initialized) {
      if(pthread_rwlock_trywrlock(&mock_states_lock) == 0) {
        if(mock->states_initialized) {
          pthread_rwlock_unlock(&mock_states_lock);
          ctf__mock_reset(mock->states + thread_index);
          return;
        } else {
          goto init;
        }
      }
    }
    ctf__mock_reset(mock->states + thread_index);
    return;
  } else {
    if(mock->states_initialized) return;
  }
init:
  mock->states =
    CTF__MALLOC(sizeof(mock->states[0]) * ctf__opt_threads, thread_index);
  for(int j = 0; j < ctf__opt_threads; j++) mock_state_init(mock->states + j);
  mock->states_initialized = 1;
  if(ctf__opt_threads > 1) pthread_rwlock_unlock(&mock_states_lock);
}

static void mock_returns_alloc(struct ctf__mock *mock, uintptr_t thread_index) {
  if(ctf__opt_threads > 1) {
    while(!mock->returns_initialized) {
      if(pthread_rwlock_trywrlock(&ctf__mock_returns_lock) == 0) {
        if(mock->returns_initialized) {
          pthread_rwlock_unlock(&ctf__mock_returns_lock);
          return;
        } else {
          goto init;
        }
      }
    }
    return;
  }
init:
  mock->returns =
    CTF__MALLOC(sizeof(mock->returns[0]) * ctf__opt_threads, thread_index);
  for(int i = 0; i < ctf__opt_threads; i++) mock_return_init(mock->returns + i);
  mock->returns_initialized = 1;
  if(ctf__opt_threads > 1) {
    pthread_rwlock_unlock(&ctf__mock_returns_lock);
  }
}

void ctf__mock_returns_ensure_allocated(struct ctf__mock *mock,
                                        uintptr_t thread_index) {
  if(ctf__opt_threads > 1) pthread_rwlock_rdlock(&ctf__mock_returns_lock);
  if(!mock->returns_initialized) {
    if(ctf__opt_threads > 1) pthread_rwlock_unlock(&ctf__mock_returns_lock);
    mock_returns_alloc(mock, thread_index);
  } else if(ctf__opt_threads > 1) {
    pthread_rwlock_unlock(&ctf__mock_returns_lock);
  }
}

void ctf__mock_return_nth(struct ctf__mock *mock, uintmax_t n,
                          struct ctf__mock_return *returns,
                          uintmax_t return_size, uintptr_t thread_index) {
  struct ctf__mock_state *const state = mock->states + thread_index;
  const uintmax_t cap =
    ctf__mock_return_capacity(state->return_overrides_size + 1);
  returns->size = state->return_overrides_size;
  if(cap > state->return_overrides_capacity) {
    if(state->return_overrides == NULL) {
      state->return_overrides =
        CTF__MALLOC(cap * sizeof(state->return_overrides[0]), thread_index);
    } else {
      state->return_overrides =
        CTF__REALLOC(state->return_overrides,
                     cap * sizeof(state->return_overrides[0]), thread_index);
    }
    state->return_overrides_capacity = cap;
  }
  if(cap >= returns->capacity) {
    if(returns->values == NULL) {
      returns->values = CTF__MALLOC(cap * return_size, thread_index);
    } else {
      returns->values =
        CTF__REALLOC(returns->values, cap * return_size, thread_index);
    }
    returns->capacity = cap;
  }
  state->return_overrides[state->return_overrides_size++] = n;
}

void ctf_unmock(void) {
  uintptr_t thread_index = (uintptr_t)pthread_getspecific(ctf__thread_index);
  struct ctf__thread_data *thread_data = ctf__thread_data + thread_index;
  if(thread_data->mock_reset_stack_size == 0) return;
  struct ctf__mock *mock =
    thread_data->mock_reset_stack[--thread_data->mock_reset_stack_size];
  if(mock->states == NULL) return;
  mock->states[thread_index].enabled = 0;
}

void ctf__mock_group(struct ctf__mock_bind *bind) {
  uintptr_t thread_index = (uintptr_t)pthread_getspecific(ctf__thread_index);
  struct ctf__thread_data *thread_data = ctf__thread_data + thread_index;
  struct ctf__mock *mock;
  for(uintmax_t i = 0; i < CTF_CONST_MOCK_GROUP_SIZE && bind[i].mock; i++) {
    mock = bind[i].mock;
    if(ctf__opt_threads > 1) pthread_rwlock_rdlock(&mock_states_lock);
    if(!mock->states_initialized) {
      if(ctf__opt_threads > 1) pthread_rwlock_unlock(&mock_states_lock);
      mock_states_alloc(mock, thread_index);
    } else {
      if(ctf__opt_threads > 1) pthread_rwlock_unlock(&mock_states_lock);
      ctf__mock_reset(mock->states + thread_index);
    }
    if(thread_data->mock_reset_stack_size + 1 >=
       thread_data->mock_reset_stack_capacity) {
      thread_data->mock_reset_stack_capacity *= 2;
      thread_data->mock_reset_stack =
        realloc(thread_data->mock_reset_stack,
                thread_data->mock_reset_stack_capacity *
                  sizeof(thread_data->mock_reset_stack[0]));
    }
    thread_data->mock_reset_stack[thread_data->mock_reset_stack_size++] = mock;
    mock->states[thread_index].mock_f = (void (*)(void))bind[i].f;
    mock->states[thread_index].enabled = 1;
  }
}

void ctf__mock_global(struct ctf__mock *mock, void (*f)(void)) {
  uintptr_t thread_index = (uintptr_t)pthread_getspecific(ctf__thread_index);
  struct ctf__thread_data *thread_data = ctf__thread_data + thread_index;
  if(ctf__opt_threads > 1) pthread_rwlock_rdlock(&mock_states_lock);
  if(!mock->states_initialized) {
    if(ctf__opt_threads > 1) pthread_rwlock_unlock(&mock_states_lock);
    mock_states_alloc(mock, thread_index);
  } else {
    if(ctf__opt_threads > 1) pthread_rwlock_unlock(&mock_states_lock);
    ctf__mock_reset(mock->states + thread_index);
  }
  mock->states[thread_index].mock_f = f;
  mock->states[thread_index].enabled = 1;
  if(thread_data->mock_reset_stack_size + 1 >=
     thread_data->mock_reset_stack_capacity) {
    thread_data->mock_reset_stack_capacity *= 2;
    thread_data->mock_reset_stack =
      realloc(thread_data->mock_reset_stack,
              thread_data->mock_reset_stack_capacity *
                sizeof(thread_data->mock_reset_stack[0]));
  }
  thread_data->mock_reset_stack[thread_data->mock_reset_stack_size++] = mock;
}

void ctf__unmock_group(struct ctf__mock_bind *bind) {
  uintptr_t thread_index = (uintptr_t)pthread_getspecific(ctf__thread_index);
  struct ctf__mock *mock;
  for(uintmax_t i = 0; i < CTF_CONST_MOCK_GROUP_SIZE && bind[i].mock; i++) {
    mock = bind[i].mock;
    if(mock->states == NULL) continue;
    mock->states[thread_index].enabled = 0;
  }
}

uintmax_t ctf__mock_return_capacity(uintmax_t cap) {
  uintmax_t mul = (cap + 1) / DEFAULT_MOCK_RETURN_CAPACITY + 1;
  mul = mul + (mul % 2 != 0 && mul != 1);
  return DEFAULT_MOCK_RETURN_CAPACITY * mul;
}

static void mock_check_base(struct ctf__mock_state *state, const char *v,
                            int mem, int in_out) {
  int removed = 0;
  for(uintmax_t i = 0; i < state->checks_count; i++) {
    if(!(state->checks[i].flags & CTF__MOCK_TYPE_MEM) == mem) continue;
    if(!(state->checks[i].flags & in_out)) continue;
    if(CTF__MOCK_CHECK_FLAGS_DIR(state->checks[i].flags) ==
         CTF__MOCK_CHECK_DIR_INOUT &&
       in_out == CTF__MOCK_CHECK_DIR_IN)
      continue;
    if(strcmp(state->checks[i].var, v)) continue;
    if(state->checks[i].call_count > 0 &&
       state->checks[i].call_count != state->call_count)
      continue;
    state->checks[i].cmp = NULL;
    removed++;
  }
  for(uintmax_t i = 0; i < state->checks_count; i++) {
    if(state->checks[i].cmp == NULL) {
      for(uintmax_t j = i + 1; j < state->checks_count; j++) {
        if(state->checks[j].cmp != NULL) {
          state->checks[i] = state->checks[j];
          state->checks[j].cmp = NULL;
          break;
        }
      }
    }
  }
  state->checks_count -= removed;
}

static void mock_base(struct ctf__mock_state *state, int call_count, int type,
                      int line, const char *file, const char *print_var,
                      const char *var) {
  uintptr_t thread_index = (uintptr_t)pthread_getspecific(ctf__thread_index);
  if(state->checks_count + 1 >= state->checks_capacity) {
    if(state->checks == NULL) {
      state->checks_capacity = DEFAULT_MOCK_STATE_CHECKS_CAPACITY;
      state->checks = CTF__MALLOC(
        sizeof(state->checks[0]) * DEFAULT_MOCK_STATE_CHECKS_CAPACITY,
        thread_index);
    } else {
      state->checks_capacity *= 2;
      state->checks = CTF__REALLOC(
        state->checks, sizeof(state->checks[0]) * state->checks_capacity,
        thread_index);
    }
  }
  struct ctf__check *const check = state->checks + state->checks_count;
  check->var = var;
  check->call_count = call_count;
  check->flags = type;
  check->line = line;
  check->file = file;
  check->print_var = print_var;
  state->checks_count++;
}

void ctf__mock_check_mem(struct ctf__mock_state *state, const void *v,
                         const char *v_print, int in_out, uintmax_t step,
                         int sign) {
  for(uintmax_t i = 0; i < state->checks_count; i++) {
    if(!(state->checks[i].flags & CTF__MOCK_TYPE_MEM)) continue;
    if(strcmp(state->checks[i].var, v_print)) continue;
    if(!(state->checks[i].flags & in_out)) continue;
    if(state->checks[i].call_count > 0 &&
       state->checks[i].call_count != state->call_count)
      continue;
    ctf__ea_arr(v, state->checks[i].cmp, state->checks[i].val.p,
                state->checks[i].length, state->checks[i].length, v_print,
                state->checks[i].print_var,
                state->checks[i].flags & CTF__MOCK_TYPE_ASSERT, step, sign,
                CTF__MOCK_CHECK_FLAGS_DATA_TYPE(state->checks[i].flags),
                state->checks[i].line, state->checks[i].file);
  }
  mock_check_base(state, v_print, 1, in_out);
}

#define MOCK_CHECK(t, T)                                                       \
  void ctf__mock_check_##t(struct ctf__mock_state *state, CTF__TYPE_##T v,     \
                           const char *v_print, int in_out) {                  \
    for(uintmax_t i = 0; i < state->checks_count; i++) {                       \
      if(CTF__MOCK_CHECK_STR_##T(state->checks[i].flags & CTF__MOCK_TYPE_MEM)) \
        continue;                                                              \
      if(!(state->checks[i].flags & in_out)) continue;                         \
      if(strcmp(state->checks[i].var, v_print)) continue;                      \
      if(state->checks[i].call_count > 0 &&                                    \
         state->checks[i].call_count != state->call_count)                     \
        continue;                                                              \
      state->checks[i].f.CTF__MOCK_CHECK_TYPE_##T(                             \
        v, state->checks[i].cmp,                                               \
        state->checks[i].val.CTF__MOCK_CHECK_TYPE_##T, v_print,                \
        state->checks[i].print_var,                                            \
        state->checks[i].flags &CTF__MOCK_TYPE_ASSERT, state->checks[i].line,  \
        state->checks[i].file);                                                \
    }                                                                          \
    mock_check_base(state, v_print, 0, in_out);                                \
  }
#define MOCK_EA(type, TYPE)                                                   \
  void ctf__mock_ea_##type(                                                   \
    struct ctf__mock_state *state, uintmax_t call_count, const char *var,     \
    const char *cmp, CTF__TYPE_##TYPE val, const char *val_str, int flags,    \
    int line, const char *file) {                                             \
    mock_base(state, call_count, flags | CTF__MOCK_CHECK_TYPE_TYPE_##TYPE,    \
              line, file, val_str, var);                                      \
    struct ctf__check *const check = state->checks + state->checks_count - 1; \
    check->val.CTF__MOCK_CHECK_TYPE_##TYPE = val;                             \
    check->cmp = cmp;                                                         \
    check->f.CTF__MOCK_CHECK_TYPE_##TYPE = ctf__ea_##type;                    \
  }

void ctf__mock_ea_mem(struct ctf__mock_state *state, uintmax_t call_count,
                      const char *var, const char *cmp, const void *val,
                      uintmax_t l, const char *val_str, int flags, int line,
                      const char *file) {
  mock_base(state, call_count, flags | CTF__MOCK_TYPE_MEM, line, file, val_str,
            var);
  struct ctf__check *const check = state->checks + state->checks_count - 1;
  check->val.p = val;
  check->length = l;
  check->cmp = cmp;
}

// clang-format off
COMB2(`RUN1', `(MOCK_CHECK)', `(PRIMITIVE_TYPES, str)')
COMB2(`RUN1', `(MOCK_EA)', `(PRIMITIVE_TYPES, str)')
// clang-format on
