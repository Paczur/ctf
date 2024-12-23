#define DEFAULT_SUBTEST_STACK_CAPACITY 32
#define DEFAULT_LEVEL_STACK_CAPACITY 32
#define DEFAULT_STATES_ALLOCATED_CAPACITY 32
#define DEFAULT_SUBTEST_ALLOCATED_CAPACITY 32
#define DEFAULT_SUBTEST_CAPACITY 32
#define DEFAULT_STATES_CAPACITY 32

static struct ctf__states **states_allocated;
static uintmax_t states_allocated_capacity;
static uintmax_t states_allocated_size;

static struct ctf__subtest **subtests_allocated;
static uintmax_t subtests_allocated_capacity;
static uintmax_t subtests_allocated_size;

static struct ctf__subtest **subtest_stack;
static uintmax_t subtest_stack_size;
static uintmax_t subtest_stack_capacity;

static uintmax_t *level_stack;
static uintmax_t level_stack_size;
static uintmax_t level_stack_capacity;

static void subtest_stack_push(struct ctf__subtest *subtest) {
  if(subtest_stack_size + 1 >= subtest_stack_capacity) {
    if(subtest_stack == NULL) {
      subtest_stack =
        malloc(DEFAULT_SUBTEST_STACK_CAPACITY * sizeof(*subtest_stack));
      subtest_stack_capacity = DEFAULT_SUBTEST_STACK_CAPACITY;
    } else {
      subtest_stack_capacity *= 2;
      subtest_stack =
        realloc(subtest_stack, subtest_stack_capacity * sizeof(*subtest_stack));
    }
  }
  subtest_stack[subtest_stack_size++] = subtest;
}

static struct ctf__subtest *subtest_stack_pop(void) {
  if(subtest_stack_size == 0) return NULL;
  return subtest_stack[--subtest_stack_size];
}

static void level_stack_push(uintmax_t level) {
  if(level_stack_size + 1 >= level_stack_capacity) {
    if(level_stack == NULL) {
      level_stack = malloc(DEFAULT_LEVEL_STACK_CAPACITY * sizeof(*level_stack));
      level_stack_capacity = DEFAULT_LEVEL_STACK_CAPACITY;
    } else {
      level_stack_capacity *= 2;
      level_stack =
        realloc(level_stack, level_stack_capacity * sizeof(*level_stack));
    }
  }
  level_stack[level_stack_size++] = level;
}

static uintmax_t level_stack_pop(void) {
  if(level_stack_size == 0) return 0;
  return level_stack[--level_stack_size];
}

static int states_status(const struct ctf__states *states) {
  for(uintmax_t i = 0; i < states->size; i++) {
    if(states->states[i].status == 1) return 1;
    if(states->states[i].status == 2) return 2;
  }
  return 0;
}

static int subtest_status_fill(struct ctf__subtest *subtest) {
  struct ctf__subtest *sub = subtest;
  struct ctf__subtest *rev_sub;
  sub->status = 0;
  while(1) {
    for(uintmax_t i = 0; i < sub->size; i++) {
      if(sub->elements[i].issubtest) {
        if(sub->elements[i].el.subtest->status == 2) {
          subtest_stack_push(sub->elements[i].el.subtest);
        } else if(sub->elements[i].el.subtest->status == 1) {
          sub->status = 1;
        }
        continue;
      }
      if(!states_status(sub->elements[i].el.states)) continue;
      sub->status = 1;
      rev_sub = sub->parent;
      while(rev_sub != NULL && rev_sub->status != 1) {
        rev_sub->status = 1;
        rev_sub = rev_sub->parent;
      }
    }
    sub = subtest_stack_pop();
    if(sub == NULL) break;
    sub->status = 0;
  }
  return subtest->status;
}

static int test_status(struct ctf__thread_data *thread_data) {
  struct ctf__test_element *el;
  int test_status = 0;
  for(uintmax_t i = 0; i < thread_data->test_elements_size; i++) {
    el = thread_data->test_elements + i;
    if(el->issubtest) {
      if(el->el.subtest->status == 2) {
        test_status |= subtest_status_fill(el->el.subtest);
      } else {
        test_status |= el->el.subtest->status;
      }
    } else {
      test_status |= states_status(el->el.states);
    }
  }
  return test_status;
}

static void state_init(struct ctf__state *state) {
  state->msg = NULL;
  state->msg_capacity = 0;
}

static void state_deinit(struct ctf__state *state) {
  if(state->msg != NULL) free(state->msg);
  state->msg = NULL;
}

static void states_init(struct ctf__states *states) {
  states->states = malloc(sizeof(*states->states) * DEFAULT_STATES_CAPACITY);
  for(uintmax_t i = 0; i < DEFAULT_STATES_CAPACITY; i++) {
    state_init(states->states + i);
  }
  states->capacity = DEFAULT_STATES_CAPACITY;
  states->size = 0;
}

static void states_deinit(struct ctf__states *states) {
  for(uintmax_t i = 0; i < states->capacity; i++)
    state_deinit(states->states + i);
  states->capacity = 0;
  if(states->states != NULL) {
    free(states->states);
  }
  free(states);
}

static void states_clear(struct ctf__states *states) { states->size = 0; }

static struct ctf__states *states_new(void) {
  struct ctf__states *states;
  if(states_allocated_size > 0) {
    states = states_allocated[--states_allocated_size];
    states_clear(states);
  } else {
    states = malloc(sizeof(*states));
    states_init(states);
  }
  return states;
}

static void states_increment(struct ctf__states *states) {
  states->size++;
  if(states->size >= states->capacity) {
    states->capacity *= 2;
    states->states =
      realloc(states->states, sizeof(states->states[0]) * states->capacity);
  }
}

static void states_delete(struct ctf__states *states) {
  states_allocated_size++;
  if(states_allocated_size >= states_allocated_capacity) {
    states_allocated_capacity *= 2;
    states_allocated = realloc(
      states_allocated, sizeof(*states_allocated) * states_allocated_capacity);
  }
  states_allocated[states_allocated_size - 1] = states;
}

static void subtest_init(struct ctf__subtest *subtest) {
  subtest->name = NULL;
  subtest->status = 2;
  subtest->size = 0;
  subtest->elements =
    malloc(sizeof(*subtest->elements) * DEFAULT_SUBTEST_CAPACITY);
  subtest->capacity = DEFAULT_SUBTEST_CAPACITY;
}

static void subtest_deinit(struct ctf__subtest *subtest) {
  if(subtest->elements != NULL) free(subtest->elements);
  free(subtest);
}

static void subtest_clear(struct ctf__subtest *subtest) {
  subtest->status = 2;
  subtest->size = 0;
}

static struct ctf__subtest *subtest_new(void) {
  struct ctf__subtest *subtest;
  if(subtests_allocated_size > 0) {
    subtest = subtests_allocated[--subtests_allocated_size];
    subtest_clear(subtest);
    return subtest;
  }
  subtest = malloc(sizeof(*subtest));
  subtest_init(subtest);
  return subtest;
}

static void subtest_delete(struct ctf__subtest *subtest) {
  do {
    for(uintmax_t i = 0; i < subtest->size; i++) {
      if(subtest->elements[i].issubtest) {
        subtest_stack_push(subtest->elements[i].el.subtest);
      } else {
        states_delete(subtest->elements[i].el.states);
      }
    }
    subtests_allocated_size++;
    if(subtests_allocated_size >= subtests_allocated_capacity) {
      subtests_allocated_capacity *= 2;
      subtests_allocated =
        realloc(subtests_allocated,
                sizeof(*subtests_allocated) * subtests_allocated_capacity);
    }
    subtests_allocated[subtests_allocated_size - 1] = subtest;
    subtest = subtest_stack_pop();
  } while(subtest != NULL);
}

static void subtest_increment(struct ctf__subtest *subtest) {
  subtest->size++;
  if(subtest->size >= subtest->capacity) {
    subtest->capacity *= 2;
    subtest->elements = realloc(subtest->elements,
                                sizeof(*subtest->elements) * subtest->capacity);
  }
}

static void test_elements_increment(struct ctf__thread_data *thread_data) {
  thread_data->test_elements_size++;
  if(thread_data->test_elements_size >= thread_data->test_elements_capacity) {
    thread_data->test_elements_capacity *= 2;
    thread_data->test_elements = realloc(thread_data->test_elements,
                                         sizeof(*thread_data->test_elements) *
                                           thread_data->test_elements_capacity);
  }
}

static void test_elements_cleanup(struct ctf__thread_data *thread_data) {
  for(uintmax_t i = 0; i < thread_data->test_elements_size; i++) {
    if(thread_data->test_elements[i].issubtest) {
      subtest_delete(thread_data->test_elements[i].el.subtest);
    } else {
      states_delete(thread_data->test_elements[i].el.states);
    }
  }
  thread_data->test_elements_size = 0;
}

static void test_elements_init(void) {
  states_allocated =
    malloc(sizeof(*states_allocated) * DEFAULT_STATES_ALLOCATED_CAPACITY);
  states_allocated_capacity = DEFAULT_STATES_ALLOCATED_CAPACITY;

  subtests_allocated =
    malloc(sizeof(*subtests_allocated) * DEFAULT_SUBTEST_ALLOCATED_CAPACITY);
  subtests_allocated_capacity = DEFAULT_SUBTEST_ALLOCATED_CAPACITY;

  subtest_stack =
    malloc(sizeof(*subtest_stack) * DEFAULT_SUBTEST_STACK_CAPACITY);
  subtest_stack_capacity = DEFAULT_SUBTEST_STACK_CAPACITY;

  level_stack = malloc(sizeof(*level_stack) * DEFAULT_LEVEL_STACK_CAPACITY);
  level_stack_capacity = DEFAULT_LEVEL_STACK_CAPACITY;
}

static void test_elements_deinit(void) {
  for(uintmax_t i = 0; i < states_allocated_size; i++) {
    states_deinit(states_allocated[i]);
  }
  states_allocated_size = 0;
  for(uintmax_t i = 0; i < subtests_allocated_size; i++) {
    subtest_deinit(subtests_allocated[i]);
  }
  subtests_allocated_size = 0;
  free(states_allocated);
  free(subtests_allocated);
  free(subtest_stack);
  free(level_stack);
}

void ctf__subtest_enter(struct ctf__thread_data *thread_data,
                        const char *name) {
  struct ctf__subtest *const subtest = thread_data->subtest_current;
  struct ctf__subtest *subtest_t = subtest_new();
  subtest_t->name = name;
  thread_data->subtest_current = subtest_t;

  if(subtest == NULL) {
    test_elements_increment(thread_data);
    subtest_t->parent = NULL;
    thread_data->test_elements[thread_data->test_elements_size - 1].issubtest =
      1;
    thread_data->test_elements[thread_data->test_elements_size - 1].el.subtest =
      subtest_t;
    return;
  }

  subtest_increment(subtest);
  subtest_t->parent = subtest;
  subtest->elements[subtest->size - 1].issubtest = 1;
  subtest->elements[subtest->size - 1].el.subtest = subtest_t;
}

void ctf__subtest_leave(struct ctf__thread_data *thread_data) {
  thread_data->subtest_current = thread_data->subtest_current->parent;
}

static struct ctf__states *states_last(struct ctf__thread_data *thread_data) {
  struct ctf__subtest *subtest = thread_data->subtest_current;
  if(subtest == NULL) {
    if(thread_data->test_elements_size == 0 ||
       thread_data->test_elements[thread_data->test_elements_size - 1]
         .issubtest)
      return NULL;
    return thread_data->test_elements[thread_data->test_elements_size - 1]
      .el.states;
  }
  if(subtest->size == 0 || subtest->elements[subtest->size - 1].issubtest)
    return NULL;
  return subtest->elements[subtest->size - 1].el.states;
}

static struct ctf__state *state_next(struct ctf__thread_data *thread_data) {
  struct ctf__subtest *const subtest = thread_data->subtest_current;
  struct ctf__states *states = states_last(thread_data);
  if(states == NULL) {
    if(subtest == NULL) {
      test_elements_increment(thread_data);
      thread_data->test_elements[thread_data->test_elements_size - 1]
        .issubtest = 0;
      thread_data->test_elements[thread_data->test_elements_size - 1]
        .el.states = states_new();
      states = thread_data->test_elements[thread_data->test_elements_size - 1]
                 .el.states;
    } else {
      subtest_increment(subtest);
      subtest->elements[subtest->size - 1].issubtest = 0;
      subtest->elements[subtest->size - 1].el.states = states_new();
      states = subtest->elements[subtest->size - 1].el.states;
    }
  }
  states_increment(states);
  return &states->states[states->size - 1];
}
