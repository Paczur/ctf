#define CTF_CONST_MOCK_GROUP_SIZE 64
#define CTF_CONST_MOCK_CHECKS_PER_TEST 64
#define CTF_CONST_MOCK_RETURN_COUNT 64

#define CTF__MOCK_TYPE_ASSERT 1
#define CTF__MOCK_TYPE_MEMORY 2

#define CTF__MOCK_EXPECT_MEMORY(t, call_count, comp, type, name, var, val, l) \
  do {                                                                        \
    prereq_assert(                                                            \
      ctf__mock_state_selected->check_count < CTF_CONST_MOCK_CHECKS_PER_TEST, \
      "Limit for mock asserts/expects per test reached");                     \
    ctf__mock_memory(ctf__mock_state_selected, call_count,                    \
                     type | CTF__MOCK_TYPE_MEMORY, __LINE__, __FILE__, #val,  \
                     (void *)ctf__expect_memory_##t##_##comp, var, val, l);   \
  } while(0)
#define CTF__MOCK_ASSERT_MEMORY(t, call_count, comp, type, name, var, val, l) \
  do {                                                                        \
    prereq_assert(                                                            \
      ctf__mock_state_selected->check_count < CTF_CONST_MOCK_CHECKS_PER_TEST, \
      "Limit for mock asserts/expects per test reached");                     \
    ctf__mock_memory(ctf__mock_state_selected, call_count,                    \
                     type | CTF__MOCK_TYPE_ASSERT | CTF__MOCK_TYPE_MEMORY,    \
                     __LINE__, __FILE__, #val,                                \
                     (void *)ctf__expect_memory_##t##_##comp, var, val, l);   \
  } while(0)
#define CTF__MOCK_EXPECT(t, call_count, comp, type, name, var, val)            \
  do {                                                                         \
    prereq_assert(                                                             \
      ctf__mock_state_selected->check_count < CTF_CONST_MOCK_CHECKS_PER_TEST,  \
      "Limit for mock asserts/expects per test reached");                      \
    ctf__mock_##t(ctf__mock_state_selected, call_count, type, __LINE__,        \
                  __FILE__, #val, (void *)ctf__expect_##t##_##comp, var, val); \
  } while(0)
#define CTF__MOCK_ASSERT(t, call_count, comp, type, name, var, val)            \
  do {                                                                         \
    preqreq_assert(                                                            \
      ctf__mock_st_##name.state[ctf__parallel_thread_index].mock_f != NULL,    \
      "Mock expect/assert used without mocking beforehand");                   \
    prereq_assert(                                                             \
      ctf__mock_st_##name.state[ctf__parallel_thread_index].check_count <      \
        CTF_CONST_MOCK_CHECKS_PER_TEST,                                        \
      "Limit fzoor mock asserts/expects per test reached");                    \
    ctf__mock_##t(call_count, type | CTF__MOCK_TYPE_ASSERT, __LINE__,          \
                  __FILE__, #val, (void *)ctf__expect_##t##_##comp, var, val); \
  } while(0)

struct ctf__check {
  const char *var;
  union {
    int (*i)(intmax_t, intmax_t, const char *, const char *, int, const char *);
    int (*u)(uintmax_t, uintmax_t, const char *, const char *, int,
             const char *);
    int (*p)(const void *, const void *, const char *, const char *, int,
             const char *);
    int (*c)(char, char, const char *, const char *, int, const char *);
    int (*s)(const char *, const char *, const char *, const char *, int,
             const char *);
    int (*m)(const void *, const void *, uintmax_t, uintmax_t, int,
             const char *, const char *, int, const char *);
  } f;
  int type;
  uintmax_t length;
  int line;
  uintmax_t call_count;
  const char *file;
  const char *print_var;
  union {
    uintmax_t u;
    intmax_t i;
    char c;
    const void *p;
  } val;
};
struct ctf__mock_state {
  void (*mock_f)(void);
  uintmax_t call_count;
  uint8_t return_overrides[CTF_CONST_MOCK_CHECKS_PER_TEST];
  struct ctf__check check[CTF_CONST_MOCK_CHECKS_PER_TEST];
  uintmax_t check_count;
};
struct ctf__mock {
  struct ctf__mock_state state[CTF_CONST_MAX_THREADS];
};
struct ctf__mock_bind {
  struct ctf__mock *mock;
  const void *f;
};

#define CTF_MOCK(ret_type, name, typed, args)                                 \
  ret_type ctf__mock_return_##name[CTF_CONST_MAX_THREADS]                     \
                                  [CTF_CONST_MOCK_RETURN_COUNT];              \
  struct ctf__mock ctf__mock_st_##name;                                       \
  ret_type __real_##name typed;                                               \
  ret_type __wrap_##name typed {                                              \
    intptr_t thread_index = (intptr_t)pthread_getspecific(ctf__thread_index); \
    struct ctf__mock_state *_data = ctf__mock_st_##name.state + thread_index; \
    ret_type(*const _mock) typed = (ret_type(*const) typed)_data->mock_f;     \
    if(_mock == NULL) {                                                       \
      return __real_##name args;                                              \
    } else {                                                                  \
      _data->call_count++;                                                    \
      if(_data->return_overrides[_data->call_count - 1]) {                    \
        _mock args;                                                           \
        return ctf__mock_return_##name[thread_index][_data->call_count - 1];  \
      }                                                                       \
      return _mock args;                                                      \
    }                                                                         \
  }
#define CTF_MOCK_VOID_RET(name, typed, args)                                  \
  struct ctf__mock ctf__mock_st_##name;                                       \
  void __real_##name typed;                                                   \
  void __wrap_##name typed {                                                  \
    intptr_t thread_index = (intptr_t)pthread_getspecific(ctf__thread_index); \
    struct ctf__mock_state *_data = ctf__mock_st_##name.state + thread_index; \
    void(*const _mock) typed = _data->mock_f;                                 \
    if(_mock == NULL) {                                                       \
      return __real_##name args;                                              \
    } else {                                                                  \
      _data->call_count++;                                                    \
      return _mock args;                                                      \
    }                                                                         \
  }
#define CTF_MOCK_STATIC(ret_type, name, typed, args)                          \
  static ret_type ctf__mock_return_##name[CTF_CONST_MAX_THREADS]              \
                                         [CTF_CONST_MOCK_RETURN_COUNT];       \
  static struct ctf__mock ctf__mock_st_##name;                                \
  static ret_type __real_##name typed;                                        \
  static ret_type __wrap_##name typed {                                       \
    intptr_t thread_index = (intptr_t)pthread_getspecific(ctf__thread_index); \
    struct ctf__mock_state *_data = ctf__mock_st_##name.state + thread_index; \
    ret_type(*const _mock) typed = _data->mock_f;                             \
    if(_mock == NULL) {                                                       \
      return __real_##name args;                                              \
    } else {                                                                  \
      _data->call_count++;                                                    \
      if(_data->return_overrides[_data->call_count - 1]) {                    \
        _mock args;                                                           \
        return ctf__mock_return_##name[thread_index][_data->call_count - 1];  \
      }                                                                       \
      return _mock args;                                                      \
    }                                                                         \
  }
#define CTF_MOCK_VOID_RET_STATIC(name, typed, args)                           \
  static struct ctf__mock ctf__mock_st_##name;                                \
  static void __real_##name typed;                                            \
  static void __wrap_##name typed {                                           \
    intptr_t thread_index = (intptr_t)pthread_getspecific(ctf__thread_index); \
    struct ctf__mock_state *_data = ctf__mock_st_##name.state + thread_index; \
    void(*const _mock) typed = _data->mock_f;                                 \
    if(_mock == NULL) {                                                       \
      return __real_##name args;                                              \
    } else {                                                                  \
      _data->call_count++;                                                    \
      return _mock args;                                                      \
    }                                                                         \
  }
#define CTF_MOCK_EXTERN(ret_type, name, typed)                          \
  extern ret_type ctf__mock_return_##name[CTF_CONST_MAX_THREADS]        \
                                         [CTF_CONST_MOCK_RETURN_COUNT]; \
  extern struct ctf__mock ctf__mock_st_##name;                          \
  ret_type __real_##name typed;                                         \
  ret_type __wrap_##name typed;
#define CTF_MOCK_VOID_RET_EXTERN(name, typed)  \
  extern struct ctf__mock ctf__mock_st_##name; \
  void __real_##name typed;                    \
  void __wrap_##name typed;
#define CTF_MOCK_BIND(fn, mock) \
  (struct ctf__mock_bind) { &ctf__mock_st_##fn, mock }
#define CTF_MOCK_GROUP(name) \
  struct ctf__mock_bind ctf__mock_group_st_##name[CTF_CONST_MOCK_GROUP_SIZE]
#define CTF_MOCK_GROUP_EXTERN(name) \
  extern struct ctf__mock_bind      \
    ctf__mock_group_st_##name[CTF_CONST_MOCK_GROUP_SIZE];

void ctf__mock_global(struct ctf__mock_state *state, void (*f)(void));
void ctf__mock_group(struct ctf__mock_bind *bind);
void ctf__unmock_group(struct ctf__mock_bind *bind);
void ctf__mock_memory(struct ctf__mock_state *state, uintmax_t call_count,
                      int type, int, const char *, const char *, void *f,
                      const char *var, const void *val, uintmax_t l);

#define ctf_mock_group(name) ctf__mock_group(ctf__mock_group_st_##name)
#define ctf_mock_global(fn, mock) \
  ctf__mock_global(ctf__mock_st_##fn.state, (void (*)(void))mock)
#define ctf_mock(fn, mock)                                                 \
  ctf_mock_global(fn, mock);                                               \
  for(typeof(ctf__mock_return_##fn[0][0]) *ctf__mock_return_selected =     \
        ctf__mock_return_##fn[(intptr_t)pthread_getspecific(               \
          ctf__thread_index)];                                             \
      ctf__mock_return_selected != NULL; ctf__mock_return_selected = NULL) \
    for(struct ctf__mock_state *ctf__mock_state_selected =                 \
          ctf__mock_st_##fn.state +                                        \
          (intptr_t)pthread_getspecific(ctf__thread_index);                \
        ctf__mock_state_selected != NULL;                                  \
        ctf__mock_state_selected = NULL, ctf_unmock())
#define ctf_mock_select(fn)                                                \
  for(typeof(ctf__mock_return_##fn[0][0]) *ctf__mock_return_selected =     \
        ctf__mock_return_##fn[(intptr_t)pthread_getspecific(               \
          ctf__thread_index)];                                             \
      ctf__mock_return_selected != NULL; ctf__mock_return_selected = NULL) \
    for(struct ctf__mock_state *ctf__mock_state_selected =                 \
          ctf__mock_st_##fn.state +                                        \
          (intptr_t)pthread_getspecific(ctf__thread_index);                \
        ctf__mock_state_selected != NULL; ctf__mock_state_selected = NULL)
void ctf_unmock(void);
#define ctf_unmock_group(name) ctf__unmock_group(ctf__mock_group_st_##name)
#define ctf_mock_call_count ctf__mock_state_selected->call_count
#define ctf_mock_will_return(val) ctf_mock_will_return_nth(1, val)
#define ctf_mock_will_return_nth(n, val)                      \
  do {                                                        \
    prereq_assert(n - 1 < CTF_CONST_MOCK_RETURN_COUNT,        \
                  "Number of mocked function calls reached"); \
    ctf__mock_return_selected[n - 1] = val;                   \
    ctf__mock_state_selected->return_overrides[n - 1] = 1;    \
  } while(0)
#define ctf_mock_real(name) __real_##name
#define ctf_mock_check(name)                      \
  struct ctf__mock_state *ctf__mock_check_state = \
    ctf__mock_st_##name.state +                   \
    (intptr_t)pthread_getspecific(ctf__thread_index)

// clang-format off
/*
define(`MOCK_FUNCTION_HELPER', `void ctf__mock_$1(struct ctf__mock_state *state, uintmax_t, int, int, const char *, const char*, void *, const char*, $2 val);')
define(`MOCK_FUNCTION', `MOCK_FUNCTION_HELPER(`$1',TYPE(`$1'))')
define(`MOCK_CHECK_FUNCTION_HELPER', `void ctf__mock_check_$1(struct ctf__mock_state *state, $2 v, const char *v_print);')
define(`MOCK_CHECK_MEMORY_FUNCTION_HELPER', `void ctf__mock_check_memory_$1(struct ctf__mock_state *state, const void *v, const char *v_print, uintmax_t step, int sign);')
define(`MOCK_CHECK_FUNCTION', `MOCK_CHECK_FUNCTION_HELPER(`$1', TYPE(`$1'))')
define(`MOCK_CHECK_MEMORY_FUNCTION', `MOCK_CHECK_MEMORY_FUNCTION_HELPER(`$1', TYPE(`$1'))')
define(`MOCK_EXPECT', `#define ctf_mock_expect_$1_$2(var, val) CTF__MOCK_EXPECT($1, 1, $2, 0, name, #var, val);')
define(`MOCK_ASSERT', `#define ctf_mock_assert_$1_$2(var, val) CTF__MOCK_EXPECT($1, 1, $2, CTF__MOCK_TYPE_ASSERT, name, #var, val);')
define(`MOCK_EXPECT_MEMORY', `#define ctf_mock_expect_memory_$1_$2(var, val, length) CTF__MOCK_EXPECT_MEMORY($1, 1, $2, 0, name, #var, val, length);')
define(`MOCK_ASSERT_MEMORY', `#define ctf_mock_assert_memory_$1_$2(var, val, length) CTF__MOCK_ASSERT_MEMORY($1, 1, $2, 0, name, #var, val, length);')
define(`MOCK_EXPECT_ARRAY', `#define ctf_mock_expect_array_$1_$2(var, val) CTF__MOCK_EXPECT_MEMORY($1, 1, $2, 0, name, #var, val, sizeof(val)/sizeof(*(val)));')
define(`MOCK_ASSERT_ARRAY', `#define ctf_mock_assert_array_$1_$2(var, val) CTF__MOCK_ASSERT_MEMORY($1, 1, $2, 0, name, #var, val, sizeof(val)/sizeof(*(val)));')
define(`MOCK_EXPECT_NTH', `#define ctf_mock_expect_nth_$1_$2(n, var, val) CTF__MOCK_EXPECT($1, n, $2, 0, name, #var, val);')
define(`MOCK_ASSERT_NTH', `#define ctf_mock_assert_nth_$1_$2(n, var, val) CTF__MOCK_EXPECT($1, n, $2, CTF__MOCK_TYPE_ASSERT, name, #var, val);')
define(`MOCK_EXPECT_NTH_MEMORY', `#define ctf_mock_expect_nth_memory_$1_$2(n, var, val, length) CTF__MOCK_EXPECT_MEMORY($1, n, $2, 0, name, #var, val, length);')
define(`MOCK_ASSERT_NTH_MEMORY', `#define ctf_mock_assert_nth_memory_$1_$2(n, var, val, length) CTF__MOCK_ASSERT_MEMORY($1, n, $2, 0, name, #var, val, length);')
define(`MOCK_EXPECT_NTH_ARRAY', `#define ctf_mock_expect_nth_array_$1_$2(n, var, val) CTF__MOCK_EXPECT_MEMORY($1, n, $2, 0, name, #var, val, sizeof(val)/sizeof(*(val)));')
define(`MOCK_ASSERT_NTH_ARRAY', `#define ctf_mock_assert_nth_array_$1_$2(n, var, val) CTF__MOCK_ASSERT_MEMORY($1, n, $2, 0, name, #var, val, sizeof(val)/sizeof(*(val)));')
define(`MOCK_CHECK', `#define ctf_mock_check_$1(v) ctf__mock_check_$1(ctf__mock_check_state, v, #v)')
define(`MOCK_CHECK_STRING', `#define ctf_mock_check_str(v) \
ctf__mock_check_ptr(ctf__mock_check_state, v, #v); \
ctf__mock_check_str(ctf__mock_check_state, v, #v)')
define(`MOCK_CHECK_MEMORY',
`format(`#define ctf_mock_check_memory_$1(v) \
ctf__mock_check_ptr(ctf__mock_check_state, v, #v); \
ctf__mock_check_memory_$1(ctf__mock_check_state, v, #v, sizeof(*(v)), %d)'
  ,ifelse(`$1',`int',1,0))')
*/
COMB(`MOCK_FUNCTION', `(PRIMITIVE_TYPES, str)')
COMB2(`MOCK_EXPECT', `(char, int, uint, ptr, str)', `(CMPS)')
COMB2(`MOCK_ASSERT', `(char, int, uint, ptr, str)', `(CMPS)')
COMB2(`MOCK_EXPECT_MEMORY', `(char, int, uint, ptr)', `(CMPS)')
COMB2(`MOCK_EXPECT_ARRAY', `(char, int, uint, ptr)', `(CMPS)')
COMB2(`MOCK_ASSERT_MEMORY', `(char, int, uint, ptr)', `(CMPS)')
COMB2(`MOCK_ASSERT_ARRAY', `(char, int, uint, ptr)', `(CMPS)')
COMB2(`MOCK_EXPECT_NTH', `(char, int, uint, ptr, str)', `(CMPS)')
COMB2(`MOCK_ASSERT_NTH', `(char, int, uint, ptr, str)', `(CMPS)')
COMB2(`MOCK_EXPECT_NTH_MEMORY', `(char, int, uint, ptr)', `(CMPS)')
COMB2(`MOCK_EXPECT_NTH_ARRAY', `(char, int, uint, ptr)', `(CMPS)')
COMB2(`MOCK_ASSERT_NTH_MEMORY', `(char, int, uint, ptr)', `(CMPS)')
COMB2(`MOCK_ASSERT_NTH_ARRAY', `(char, int, uint, ptr)', `(CMPS)')
COMB(`MOCK_CHECK_FUNCTION', `(PRIMITIVE_TYPES, str)')
COMB(`MOCK_CHECK_MEMORY_FUNCTION', `(PRIMITIVE_TYPES, str)')
COMB(`MOCK_CHECK', `(PRIMITIVE_TYPES)')
MOCK_CHECK_STRING
COMB(`MOCK_CHECK_MEMORY', `(PRIMITIVE_TYPES)')
// clang-format on

#if CTF_ALIASES == CTF_ON
// clang-format off
/*
define(`MOCK_EA_ALIAS', `ALIAS(mock_$3_$1_$2(var, val))')
define(`MOCK_EA_MEMORY_ALIAS', `ALIAS(mock_$3_memory_$1_$2(var, val, length))')
define(`MOCK_EA_ARRAY_ALIAS', `ALIAS(mock_$3_array_$1_$2(var, val))')
define(`MOCK_EA_NTH_ALIAS', `ALIAS(mock_$3_nth_$1_$2(n, var, val))')
define(`MOCK_EA_NTH_MEMORY_ALIAS', `ALIAS(mock_$3_nth_memory_$1_$2(n, var, val, length))')
define(`MOCK_EA_NTH_ARRAY_ALIAS', `ALIAS(mock_$3_nth_array_$1_$2(n, var, val))')
define(`MOCK_CHECK_ALIAS', `ALIAS(mock_check_$1(val))')
define(`MOCK_CHECK_MEMORY_ALIAS', `ALIAS(mock_check_memory_$1(val))')
*/
EA_FACTORY(`(PRIMITIVE_TYPES, str)', `(CMPS)', `MOCK_EA_ALIAS')
EA_FACTORY(`(PRIMITIVE_TYPES)', `(CMPS)', `MOCK_EA_MEMORY_ALIAS')
EA_FACTORY(`(PRIMITIVE_TYPES)', `(CMPS)', `MOCK_EA_ARRAY_ALIAS')
EA_FACTORY(`(PRIMITIVE_TYPES, str)', `(CMPS)', `MOCK_EA_NTH_ALIAS')
EA_FACTORY(`(PRIMITIVE_TYPES)', `(CMPS)', `MOCK_EA_NTH_MEMORY_ALIAS')
EA_FACTORY(`(PRIMITIVE_TYPES)', `(CMPS)', `MOCK_EA_NTH_ARRAY_ALIAS')
COMB(`MOCK_CHECK_ALIAS', `(PRIMITIVE_TYPES, str)')
COMB(`MOCK_CHECK_MEMORY_ALIAS', `(PRIMITIVE_TYPES)')
COMB(`ALIAS',
     `(mock_global(name, f), mock(name, f), unmock(), mock_select(fn),
       mock_group(name), unmock_group(name),
       mock_call_count, mock_real(name),
       mock_check(name),
       mock_will_return(val), mock_will_return_nth(n, val))')
// clang-format on
#endif
