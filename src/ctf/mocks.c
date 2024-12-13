#define DEFAULT_MOCK_RETURN_CAPACITY 16
#define DEFAULT_MOCK_STATE_CHECKS_CAPACITY 16

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

static void mock_state_reset(struct ctf__mock_state *state) {
  state->call_count = 0;
  state->checks_count = 0;
  memset(state->return_overrides,
         -1,
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
          mock_state_reset(mock->states + thread_index);
          return;
        } else {
          goto init;
        }
      }
    }
    mock_state_reset(mock->states + thread_index);
    return;
  }
init:
  mock->states = malloc(sizeof(mock->states[0]) * ctf__opt_threads);
  for(int j = 0; j < ctf__opt_threads; j++) mock_state_init(mock->states + j);
  mock->states_initialized = 1;
  if(ctf__opt_threads > 1) {
    pthread_rwlock_unlock(&mock_states_lock);
  }
  if(ctf__opt_cleanup) ctf__cleanup_append(mock->states, thread_index);
}

void ctf__mock_returns_alloc(struct ctf__mock *mock, intptr_t thread_index) {
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
  mock->returns = malloc(sizeof(mock->returns[0]) * ctf__opt_threads);
  for(int i = 0; i < ctf__opt_threads; i++) mock_return_init(mock->returns + i);
  mock->returns_initialized = 1;
  if(ctf__opt_threads > 1) {
    pthread_rwlock_unlock(&ctf__mock_returns_lock);
  }
  if(ctf__opt_cleanup) ctf__cleanup_append(mock->returns, thread_index);
}

void ctf_unmock(void) {
  intptr_t thread_index = (intptr_t)pthread_getspecific(ctf__thread_index);
  struct ctf__thread_data *thread_data = ctf__thread_data + thread_index;
  if(thread_data->mock_reset_stack_size == 0) return;
  struct ctf__mock *mock =
    thread_data->mock_reset_stack[--thread_data->mock_reset_stack_size];
  if(mock->states == NULL) return;
  mock->states[thread_index].mock_f = NULL;
}

void ctf__mock_group(struct ctf__mock_bind *bind) {
  intptr_t thread_index = (intptr_t)pthread_getspecific(ctf__thread_index);
  struct ctf__thread_data *thread_data = ctf__thread_data + thread_index;
  struct ctf__mock *mock;
  for(uintmax_t i = 0; i < CTF_CONST_MOCK_GROUP_SIZE && bind[i].f; i++) {
    mock = bind[i].mock;
    if(ctf__opt_threads > 1) pthread_rwlock_rdlock(&mock_states_lock);
    if(!mock->states_initialized) {
      if(ctf__opt_threads > 1) pthread_rwlock_unlock(&mock_states_lock);
      mock_states_alloc(mock, thread_index);
    } else {
      if(ctf__opt_threads > 1) pthread_rwlock_unlock(&mock_states_lock);
      mock_state_reset(mock->states + thread_index);
    }
    thread_data->mock_reset_stack[thread_data->mock_reset_stack_size++] = mock;
    mock->states[thread_index].mock_f = (void (*)(void))bind[i].f;
  }
}

void ctf__mock_global(struct ctf__mock *mock, void (*f)(void)) {
  intptr_t thread_index = (intptr_t)pthread_getspecific(ctf__thread_index);
  struct ctf__thread_data *thread_data = ctf__thread_data + thread_index;
  if(ctf__opt_threads > 1) pthread_rwlock_rdlock(&mock_states_lock);
  if(mock->states == NULL) {
    if(ctf__opt_threads > 1) pthread_rwlock_unlock(&mock_states_lock);
    mock_states_alloc(mock, thread_index);
  } else {
    if(ctf__opt_threads > 1) pthread_rwlock_unlock(&mock_states_lock);
    mock_state_reset(mock->states + thread_index);
  }
  mock->states[thread_index].mock_f = f;
  thread_data->mock_reset_stack[thread_data->mock_reset_stack_size++] = mock;
}

void ctf__unmock_group(struct ctf__mock_bind *bind) {
  intptr_t thread_index = (intptr_t)pthread_getspecific(ctf__thread_index);
  struct ctf__mock *mock;
  for(uintmax_t i = 0; i < CTF_CONST_MOCK_GROUP_SIZE && bind[i].f; i++) {
    mock = bind[i].mock;
    if(mock->states == NULL) continue;
    mock->states[thread_index].mock_f = NULL;
  }
}

uintmax_t ctf__mock_return_capacity(uintmax_t cap) {
  uintmax_t mul = (cap + 1) / DEFAULT_MOCK_RETURN_CAPACITY + 1;
  mul = mul + (mul % 2 != 0 && mul != 1);
  return DEFAULT_MOCK_RETURN_CAPACITY * mul;
}

static void mock_check_base(struct ctf__mock_state *state,
                            const char *v,
                            int memory) {
  int removed = 0;
  for(uintmax_t i = 0; i < state->checks_count; i++) {
    if(!(state->checks[i].type & CTF__MOCK_TYPE_MEMORY) == memory) continue;
    if(strcmp(state->checks[i].var, v)) continue;
    if(state->checks[i].call_count != state->call_count) continue;
    state->checks[i].f.i = NULL;
    removed++;
  }
  for(uintmax_t i = 0; i < state->checks_count; i++) {
    if(state->checks[i].f.i == NULL) {
      for(uintmax_t j = i + 1; j < state->checks_count; j++) {
        if(state->checks[j].f.i != NULL) {
          state->checks[i] = state->checks[j];
          state->checks[j].f.i = NULL;
          break;
        }
      }
    }
  }
  state->checks_count -= removed;
}

static void mock_base(struct ctf__mock_state *state,
                      int call_count,
                      int type,
                      int line,
                      const char *file,
                      const char *print_var,
                      void *f,
                      const char *var) {
  if(state->checks_count + 1 >= state->checks_capacity) {
    if(state->checks == NULL) {
      state->checks_capacity = DEFAULT_MOCK_STATE_CHECKS_CAPACITY;
      state->checks =
        malloc(sizeof(state->checks[0]) * DEFAULT_MOCK_STATE_CHECKS_CAPACITY);
      if(ctf__opt_cleanup)
        ctf__cleanup_append(state->checks,
                            (intptr_t)pthread_getspecific(ctf__thread_index));
    } else {
      state->checks_capacity *= 2;
      state->checks = realloc(
        state->checks, sizeof(state->checks[0]) * state->checks_capacity);
    }
  }
  struct ctf__check *const check = state->checks + state->checks_count;
  check->f.i = f;
  check->var = var;
  check->call_count = call_count;
  check->type = type;
  check->line = line;
  check->file = file;
  check->print_var = print_var;
  state->checks_count++;
}

// clang-format off
/* MOCKS
   define(`MOCK_HELPER',
   `void ctf__mock_$1(struct ctf__mock_state *state,uintmax_t call_count, int type,
   int line, const char *file, const char *print_var,
   void *f, const char *var, $2 val) {
   mock_base(state, call_count, type | $4, line, file, print_var, f, var);
   struct ctf__check *const check = state->checks + state->checks_count-1;
   check->val.$3 = val;
   }')
   define(`MOCK_STR',
   `void ctf__mock_str(struct ctf__mock_state *state,uintmax_t call_count, int type,
   int line, const char *file, const char *print_var,
   void *f, const char *var, const char *val) {
   mock_base(state, call_count, type | CTF__MOCK_TYPE_MEMORY, line, file, print_var, f, var);
   struct ctf__check *const check = state->checks + state->checks_count-1;
   check->val.p = val;
   }')
   define(`MOCK_MEMORY_HELPER',
   `void ctf__mock_$1(struct ctf__mock_state *state,uintmax_t call_count, int type,
   int line, const char *file, const char *print_var,
   void *f, const char *var, $2 val, uintmax_t l) {
   mock_base(state, call_count, type | CTF__MOCK_TYPE_MEMORY, line, file, print_var, f, var);
   struct ctf__check *const check = state->checks + state->checks_count-1;
   check->val.$3 = val;
   check->length = l;
   }')
   */
/* MOCK CHECKS
   define(`MOCK_CHECK_HELPER',
   `void ctf__mock_check_$1(struct ctf__mock_state *state, $2 v,
   const char *v_print) {
   int ret = 1;
   intptr_t thread_index =                                                   \
   (intptr_t)pthread_getspecific(ctf__thread_index);               \
   for(uintmax_t i=0; i<state->checks_count; i++) {
   if(state->checks[i].type & CTF__MOCK_TYPE_MEMORY) continue;
   if(strcmp(state->checks[i].var, v_print)) continue;
   if(state->checks[i].call_count != state->call_count) continue;
   ret = state->checks[i].f.$3(
   state->checks[i].val.$3, v,
   state->checks[i].print_var, v_print, state->checks[i].line,
   state->checks[i].file);
   if(!ret && state->checks[i].type & CTF__MOCK_TYPE_ASSERT) {
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
   for(uintmax_t i=0; i<state->checks_count; i++) {
   if(!(state->checks[i].type & CTF__MOCK_TYPE_MEMORY)) continue;
   if(strcmp(state->checks[i].var, v_print)) continue;
   if(state->checks[i].call_count != state->call_count) continue;
   ret = state->checks[i].f.p(
   state->checks[i].val.p, v,
   state->checks[i].print_var, v_print, state->checks[i].line,
   state->checks[i].file);
   if(!ret && state->checks[i].type & CTF__MOCK_TYPE_ASSERT) {
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
   for(uintmax_t i=0; i<state->checks_count; i++) {
   if(!(state->checks[i].type & CTF__MOCK_TYPE_MEMORY)) continue;
   if(strcmp(state->checks[i].var, v_print)) continue;
   if(state->checks[i].call_count != state->call_count) continue;
   ret = state->checks[i].f.m( state->checks[i].val.p, v, state->checks[i].length, step, sign,
   state->checks[i].print_var, v_print, state->checks[i].line,
   state->checks[i].file);
   if(!ret && state->checks[i].type & CTF__MOCK_TYPE_ASSERT) {
   longjmp(ctf__assert_jmp_buff[thread_index], 1);
   }
   }
   mock_check_base(state, v_print, 1);
   }')
   */
/* DEFINES
define(`MOCK', `MOCK_HELPER(`$1', TYPE(`$1'), SHORT(`$1'), 0)')
define(`MOCK_MEMORY', `MOCK_MEMORY_HELPER(`$1', `const void *', `p')')
define(`MOCK_CHECK', `MOCK_CHECK_HELPER(`$1', TYPE(`$1'), SHORT(`$1'))')
define(`MOCK_CHECK_MEMORY', `MOCK_CHECK_MEMORY_HELPER(`$1')')
*/
COMB(`MOCK', `(PRIMITIVE_TYPES)')
MOCK_STR
COMB(`MOCK_MEMORY', `(memory)')
COMB(`MOCK_CHECK', `(PRIMITIVE_TYPES)')
MOCK_CHECK_STR
COMB(`MOCK_CHECK_MEMORY', `(PRIMITIVE_TYPES)')
// clang-format on
