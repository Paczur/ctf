#define DEFAULT_SUBTEST_STACK_CAPACITY 32
#define DEFAULT_LEVEL_STACK_CAPACITY 32
#define DEFAULT_STATES_ALLOCATED_CAPACITY 32
#define DEFAULT_SUBTEST_ALLOCATED_CAPACITY 32
#define DEFAULT_SUBTEST_CAPACITY 32
#define DEFAULT_STATES_CAPACITY 32

struct subtest_vec {
  struct ctf__subtest **subtests;
  uintmax_t size;
  uintmax_t capacity;
};
struct states_vec {
  struct ctf__states **states;
  uintmax_t size;
  uintmax_t capacity;
};
struct level_vec {
  uintmax_t *levels;
  uintmax_t size;
  uintmax_t capacity;
};

static struct states_vec *states_allocated;
static struct subtest_vec *subtests_allocated;
static struct subtest_vec *subtest_stack;
static struct level_vec *level_stack;

static void subtest_stack_push(struct subtest_vec *stack,
                               struct ctf__subtest *subtest) {
  if(stack->size + 1 >= stack->capacity) {
    if(stack->subtests == NULL) {
      stack->subtests =
        malloc(DEFAULT_SUBTEST_STACK_CAPACITY * sizeof(*stack->subtests));
      stack->capacity = DEFAULT_SUBTEST_STACK_CAPACITY;
    } else {
      stack->capacity *= 2;
      stack->subtests =
        realloc(stack->subtests, stack->capacity * sizeof(*stack->subtests));
    }
  }
  stack->subtests[stack->size++] = subtest;
}

static struct ctf__subtest *subtest_stack_pop(struct subtest_vec *stack) {
  if(stack->size == 0) return NULL;
  return stack->subtests[--stack->size];
}

static void level_stack_push(struct level_vec *stack, uintmax_t level) {
  if(stack->size + 1 >= stack->capacity) {
    if(stack->levels == NULL) {
      stack->levels =
        malloc(DEFAULT_LEVEL_STACK_CAPACITY * sizeof(*stack->levels));
      stack->capacity = DEFAULT_LEVEL_STACK_CAPACITY;
    } else {
      stack->capacity *= 2;
      stack->levels =
        realloc(stack->levels, stack->capacity * sizeof(*stack->levels));
    }
  }
  stack->levels[stack->size++] = level;
}

static uintmax_t level_stack_pop(struct level_vec *stack) {
  if(stack->size == 0) return 0;
  return stack->levels[--stack->size];
}

static int states_status(const struct ctf__states *states) {
  for(uintmax_t i = 0; i < states->size; i++) {
    if(states->states[i].status == 1) return 1;
    if(states->states[i].status == 2) return 2;
  }
  return 0;
}

static int subtest_status_fill(struct subtest_vec *stack,
                               struct ctf__subtest *subtest) {
  struct ctf__subtest *sub = subtest;
  struct ctf__subtest *rev_sub;
  sub->status = 0;
  while(1) {
    for(uintmax_t i = 0; i < sub->size; i++) {
      if(sub->elements[i].issubtest) {
        if(sub->elements[i].el.subtest->status == 2) {
          subtest_stack_push(stack, sub->elements[i].el.subtest);
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
    sub = subtest_stack_pop(stack);
    if(sub == NULL) break;
    sub->status = 0;
  }
  return subtest->status;
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

static struct ctf__states *states_new(struct states_vec *allocated) {
  struct ctf__states *states;
  if(allocated->size > 0) {
    states = allocated->states[--allocated->size];
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
    for(uintmax_t i = states->size; i < states->capacity; i++)
      state_init(states->states + i);
  }
}

static void states_delete(uintptr_t thread_index, struct ctf__states *states) {
  struct states_vec *allocated = states_allocated + thread_index;
  allocated->size++;
  if(allocated->size >= allocated->capacity) {
    allocated->capacity *= 2;
    allocated->states = realloc(
      allocated->states, sizeof(*allocated->states) * allocated->capacity);
  }
  allocated->states[allocated->size - 1] = states;
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

static struct ctf__subtest *subtest_new(struct subtest_vec *allocated) {
  struct ctf__subtest *subtest;
  if(allocated->size > 0) {
    subtest = allocated->subtests[--allocated->size];
    subtest_clear(subtest);
    return subtest;
  }
  subtest = malloc(sizeof(*subtest));
  subtest_init(subtest);
  return subtest;
}

static void subtest_flat_delete(uintptr_t thread_index,
                                struct ctf__subtest *subtest) {
  struct subtest_vec *allocated_subtests = subtests_allocated + thread_index;
  allocated_subtests->size++;
  if(allocated_subtests->size >= allocated_subtests->capacity) {
    allocated_subtests->capacity *= 2;
    allocated_subtests->subtests = realloc(
      allocated_subtests->subtests,
      sizeof(*allocated_subtests->subtests) * allocated_subtests->capacity);
  }
  allocated_subtests->subtests[allocated_subtests->size - 1] = subtest;
}

static void subtest_delete(uintptr_t thread_index,
                           struct ctf__subtest *subtest) {
  struct subtest_vec *stack = subtest_stack + thread_index;
  do {
    for(uintmax_t i = 0; i < subtest->size; i++) {
      if(subtest->elements[i].issubtest) {
        subtest_stack_push(stack, subtest->elements[i].el.subtest);
      } else {
        states_delete(thread_index, subtest->elements[i].el.states);
      }
    }
    subtest_flat_delete(thread_index, subtest);
    subtest = subtest_stack_pop(stack);
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

static void test_elements_increment(uintptr_t thread_index) {
  struct ctf__thread_data *thread_data = ctf__thread_data + thread_index;
  thread_data->test_elements_size++;
  if(thread_data->test_elements_size >= thread_data->test_elements_capacity) {
    thread_data->test_elements_capacity *= 2;
    thread_data->test_elements = realloc(thread_data->test_elements,
                                         sizeof(*thread_data->test_elements) *
                                           thread_data->test_elements_capacity);
  }
}

static void test_elements_cleanup(uintptr_t thread_index) {
  struct ctf__thread_data *thread_data = ctf__thread_data + thread_index;
  for(uintmax_t i = 0; i < thread_data->test_elements_size; i++) {
    if(thread_data->test_elements[i].issubtest) {
      subtest_delete(thread_index, thread_data->test_elements[i].el.subtest);
    } else {
      states_delete(thread_index, thread_data->test_elements[i].el.states);
    }
  }
  thread_data->test_elements_size = 0;
}

static void test_elements_init(void) {
  states_allocated =
    malloc(ctf__opt_threads *
           (sizeof(*states_allocated) + sizeof(*subtests_allocated) +
            sizeof(*subtest_stack) + sizeof(*level_stack)));
  subtests_allocated = (void *)(states_allocated + ctf__opt_threads);
  subtest_stack = (void *)(subtests_allocated + ctf__opt_threads);
  level_stack = (void *)(subtest_stack + ctf__opt_threads);

  for(int i = 0; i < ctf__opt_threads; i++) {
    states_allocated[i].states = malloc(sizeof(*states_allocated[i].states) *
                                        DEFAULT_STATES_ALLOCATED_CAPACITY);
    states_allocated[i].capacity = DEFAULT_STATES_ALLOCATED_CAPACITY;
    states_allocated[i].size = 0;

    subtests_allocated[i].subtests =
      malloc(sizeof(*subtests_allocated[i].subtests) *
             DEFAULT_SUBTEST_ALLOCATED_CAPACITY);
    subtests_allocated[i].capacity = DEFAULT_SUBTEST_ALLOCATED_CAPACITY;
    subtests_allocated[i].size = 0;

    subtest_stack[i].subtests = malloc(sizeof(*subtest_stack[i].subtests) *
                                       DEFAULT_SUBTEST_STACK_CAPACITY);
    subtest_stack[i].capacity = DEFAULT_SUBTEST_STACK_CAPACITY;
    subtest_stack[i].size = 0;

    level_stack[i].levels =
      malloc(sizeof(*level_stack[i].levels) * DEFAULT_LEVEL_STACK_CAPACITY);
    level_stack[i].capacity = DEFAULT_LEVEL_STACK_CAPACITY;
    level_stack[i].size = 0;
  }
}

static void test_elements_deinit(void) {
  for(int j = 0; j < ctf__opt_threads; j++) {
    for(uintmax_t i = 0; i < states_allocated[j].size; i++)
      states_deinit(states_allocated[j].states[i]);
    for(uintmax_t i = 0; i < subtests_allocated[j].size; i++)
      subtest_deinit(subtests_allocated[j].subtests[i]);
    free(states_allocated[j].states);
    free(subtests_allocated[j].subtests);
    free(subtest_stack[j].subtests);
    free(level_stack[j].levels);
  }
  free(states_allocated);
  states_allocated = NULL;
  subtests_allocated = NULL;
  subtest_stack = NULL;
  level_stack = NULL;
}

static int test_status(uintptr_t thread_index) {
  struct ctf__thread_data *thread_data = ctf__thread_data + thread_index;
  struct ctf__test_element *el;
  int test_status = 0;
  for(uintmax_t i = 0; i < thread_data->test_elements_size; i++) {
    el = thread_data->test_elements + i;
    if(el->issubtest) {
      if(el->el.subtest->status == 2) {
        test_status |=
          subtest_status_fill(subtest_stack + thread_index, el->el.subtest);
      } else {
        test_status |= el->el.subtest->status;
      }
    } else {
      test_status |= states_status(el->el.states);
    }
  }
  return test_status;
}

void ctf__subtest_enter(uintptr_t thread_index, const char *name) {
  struct ctf__thread_data *thread_data = ctf__thread_data + thread_index;
  struct ctf__subtest *const subtest = thread_data->subtest_current;
  struct ctf__subtest *subtest_t =
    subtest_new(subtests_allocated + thread_index);
  subtest_t->name = name;
  thread_data->subtest_current = subtest_t;

  if(subtest == NULL) {
    test_elements_increment(thread_index);
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

void ctf__subtest_leave(uintptr_t thread_index) {
  struct ctf__subtest *sub;
  struct ctf__subtest *parent;
  struct ctf__thread_data *thread_data = ctf__thread_data + thread_index;
  sub = thread_data->subtest_current;
  parent = sub->parent;
  thread_data->subtest_current = parent;
  if(sub->size > 0) {
    if(opt_statistics) {
      subtest_status_fill(subtest_stack + thread_index, sub);
      if(sub->status) {
        thread_data->stats.subtests_failed++;
      } else {
        thread_data->stats.subtests_passed++;
      }
    }
  } else {
    if(parent == NULL) {
      for(uintmax_t i = 0; i < thread_data->test_elements_size; i++) {
        if(thread_data->test_elements[i].issubtest &&
           thread_data->test_elements[i].el.subtest == sub) {
          for(uintmax_t j = i; j < thread_data->test_elements_size - 1; j++)
            thread_data->test_elements[j] = thread_data->test_elements[j + 1];
          thread_data->test_elements_size--;
          subtest_flat_delete(thread_index, sub);
          break;
        }
      }
    } else {
      for(uintmax_t i = 0; i < parent->size; i++) {
        if(parent->elements[i].issubtest &&
           parent->elements[i].el.subtest == sub) {
          for(uintmax_t j = i; j < parent->size - 1; j++)
            parent->elements[j] = parent->elements[j + 1];
          parent->size--;
          subtest_flat_delete(thread_index, sub);
          break;
        }
      }
    }
  }
}

static struct ctf__states *states_last(uintptr_t thread_index) {
  struct ctf__thread_data *thread_data = ctf__thread_data + thread_index;
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

static struct ctf__state *state_next(uintptr_t thread_index) {
  struct ctf__thread_data *thread_data = ctf__thread_data + thread_index;
  struct ctf__subtest *const subtest = thread_data->subtest_current;
  struct ctf__states *states = states_last(thread_index);
  if(states == NULL) {
    if(subtest == NULL) {
      test_elements_increment(thread_index);
      thread_data->test_elements[thread_data->test_elements_size - 1]
        .issubtest = 0;
      thread_data->test_elements[thread_data->test_elements_size - 1]
        .el.states = states_new(states_allocated + thread_index);
      states = thread_data->test_elements[thread_data->test_elements_size - 1]
                 .el.states;
    } else {
      subtest_increment(subtest);
      subtest->elements[subtest->size - 1].issubtest = 0;
      subtest->elements[subtest->size - 1].el.states =
        states_new(states_allocated + thread_index);
      states = subtest->elements[subtest->size - 1].el.states;
    }
  }
  states_increment(states);
  return &states->states[states->size - 1];
}
