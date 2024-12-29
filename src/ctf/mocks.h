#define CTF_CONST_MOCK_GROUP_SIZE 64

#include <stddef.h>
#include <stdlib.h>

#define CTF__MOCK_TYPE_EXPECT 0
#define CTF__MOCK_TYPE_ASSERT 1
#define CTF__MOCK_TYPE_MEM 2

#define CTF__MOCK_SELECTED                                    \
  ((struct ctf__mock *)(((void *)ctf__mock_return_selected) - \
                        offsetof(struct ctf__mock, returns)))
#define CTF__MOCK_STRUCT_SELECTED \
  ((CTF__MOCK_SELECTED)->states + ctf__local_thread_index)

struct ctf__check {
  const char *var;
  union {
    int (*i)(intmax_t, const char *, intmax_t, const char *, const char *, int,
             int, const char *);
    int (*u)(uintmax_t, const char *, uintmax_t, const char *, const char *,
             int, int, const char *);
    int (*f)(long double, const char *, long double, const char *, const char *,
             int, int, const char *);
    int (*c)(char, const char *, char, const char *, const char *, int, int,
             const char *);
    int (*p)(const void *, const char *, const void *, const char *,
             const char *, int, int, const char *);
    int (*s)(const char *, const char *, const char *, const char *,
             const char *, int, int, const char *);
  } f;
  uint8_t type;
  uint8_t mem_type;
  const char *cmp;
  uintmax_t length;
  int line;
  uintmax_t call_count;
  const char *file;
  const char *print_var;
  union {
    uintmax_t u;
    intmax_t i;
    long double f;
    char c;
    const void *p;
    const char *s;
  } val;
};
struct ctf__mock_state {
  void (*mock_f)(void);
  uint8_t enabled;
  uintmax_t call_count;
  uintmax_t *return_overrides;
  uintmax_t return_overrides_size;
  uintmax_t return_overrides_capacity;
  struct ctf__check *checks;
  uintmax_t checks_count;
  uintmax_t checks_capacity;
};
struct ctf__mock_return {
  void *values;
  uintmax_t size;
  uintmax_t capacity;
};
struct ctf__mock {
  struct ctf__mock_state *states;
  int states_initialized;
  struct ctf__mock_return *returns;
  int returns_initialized;
};
struct ctf__mock_bind {
  struct ctf__mock *mock;
  const void *f;
};

#define CTF_MOCK(ret_type, name, typed, args)                                  \
  struct ctf__mock ctf__mock_struct_##name;                                    \
  struct ctf__mock_struct_##name {                                             \
    ret_type *values;                                                          \
    uintmax_t size;                                                            \
    uintmax_t capacity;                                                        \
  } **ctf__mock_return_##name =                                                \
    (struct ctf__mock_struct_##name **)&(ctf__mock_struct_##name.returns);     \
  ret_type __real_##name typed;                                                \
  void ctf__mock_checks_##name(struct ctf__mock_state *, CTF__MACRO_VA typed); \
  ret_type __wrap_##name typed {                                               \
    uintptr_t thread_index =                                                   \
      (uintptr_t)pthread_getspecific(ctf__thread_index);                       \
    struct ctf__mock_state *_data = ctf__mock_struct_##name.states;            \
    ret_type(*_mock_f) typed;                                                  \
    ret_type temp;                                                             \
    if(_data == NULL || !_data[thread_index].enabled)                          \
      return __real_##name args;                                               \
    _mock_f = (ret_type(*) typed)_data[thread_index].mock_f;                   \
    _data[thread_index].call_count++;                                          \
    ctf__mock_checks_##name(ctf__mock_struct_##name.states + thread_index,     \
                            CTF__MACRO_VA args);                               \
    for(uintmax_t i = 0; i < _data[thread_index].return_overrides_size; i++) { \
      if(_data[thread_index].return_overrides[i] ==                            \
         _data[thread_index].call_count) {                                     \
        temp = (*ctf__mock_return_##name)[thread_index].values[i];             \
        for(uintmax_t j = i;                                                   \
            j < _data[thread_index].return_overrides_size - 1; j++) {          \
          (*ctf__mock_return_##name)[thread_index].values[j] =                 \
            (*ctf__mock_return_##name)[thread_index].values[j + 1];            \
          _data[thread_index].return_overrides[j] =                            \
            _data[thread_index].return_overrides[j + 1];                       \
        }                                                                      \
        _data[thread_index].return_overrides_size--;                           \
        (*ctf__mock_return_##name)[thread_index].size--;                       \
        if(_mock_f != NULL) _mock_f args;                                      \
        return temp;                                                           \
      }                                                                        \
    }                                                                          \
    if(_mock_f == NULL)                                                        \
      ctf__error("Mock return value not overriden and function not selected"); \
    return _mock_f args;                                                       \
  }                                                                            \
  void ctf__mock_checks_##name(struct ctf__mock_state *ctf__mock_check_state,  \
                               CTF__MACRO_VA typed)
#define CTF_MOCK_VOID_RET(name, typed, args)                                   \
  struct ctf__mock ctf__mock_struct_##name;                                    \
  struct ctf__mock_struct_##name {                                             \
    void *values;                                                              \
    uintmax_t size;                                                            \
    uintmax_t capacity;                                                        \
  } **ctf__mock_return_##name =                                                \
    (struct ctf__mock_struct_##name **)&(ctf__mock_struct_##name.returns);     \
  void ctf__mock_checks_##name(struct ctf__mock_state *, CTF__MACRO_VA typed); \
  void __real_##name typed;                                                    \
  void __wrap_##name typed {                                                   \
    uintptr_t thread_index =                                                   \
      (uintptr_t)pthread_getspecific(ctf__thread_index);                       \
    struct ctf__mock_state *_data = ctf__mock_struct_##name.states;            \
    void(*_mock_f) typed;                                                      \
    if(_data == NULL || !_data[thread_index].enabled)                          \
      return __real_##name args;                                               \
    _mock_f = (void(*) typed)_data[thread_index].mock_f;                       \
    _data[thread_index].call_count++;                                          \
    ctf__mock_checks_##name(ctf__mock_struct_##name.states + thread_index,     \
                            CTF__MACRO_VA args);                               \
    if(_mock_f != NULL) _mock_f args;                                          \
  }                                                                            \
  void ctf__mock_checks_##name(struct ctf__mock_state *ctf__mock_check_state,  \
                               CTF__MACRO_VA typed)
#define CTF_MOCK_STATIC(ret_type, name, typed, args)                           \
  struct ctf__mock ctf__mock_struct_##name;                                    \
  struct ctf__mock_struct_##name {                                             \
    ret_type *values;                                                          \
    uintmax_t size;                                                            \
    uintmax_t capacity;                                                        \
  } **ctf__mock_return_##name =                                                \
    (struct ctf__mock_struct_##name **)&(ctf__mock_struct_##name.returns);     \
  static ret_type __real_##name typed;                                         \
  static void ctf__mock_checks_##name(struct ctf__mock_state *,                \
                                      CTF__MACRO_VA typed);                    \
  static ret_type __wrap_##name typed {                                        \
    uintptr_t thread_index =                                                   \
      (uintptr_t)pthread_getspecific(ctf__thread_index);                       \
    struct ctf__mock_state *_data = ctf__mock_struct_##name.states;            \
    ret_type(*_mock_f) typed;                                                  \
    ret_type temp;                                                             \
    if(_data == NULL || !_data[thread_index].enabled)                          \
      return __real_##name args;                                               \
    _mock_f = (ret_type(*) typed)_data[thread_index].mock_f;                   \
    _data[thread_index].call_count++;                                          \
    ctf__mock_checks_##name(ctf__mock_struct_##name.states + thread_index,     \
                            CTF__MACRO_VA args);                               \
    for(uintmax_t i = 0; i < _data[thread_index].return_overrides_size; i++) { \
      if(_data[thread_index].return_overrides[i] ==                            \
         _data[thread_index].call_count) {                                     \
        temp = (*ctf__mock_return_##name)[thread_index].values[i];             \
        for(uintmax_t j = i;                                                   \
            j < _data[thread_index].return_overrides_size - 1; j++) {          \
          (*ctf__mock_return_##name)[thread_index].values[j] =                 \
            (*ctf__mock_return_##name)[thread_index].values[j + 1];            \
          _data[thread_index].return_overrides[j] =                            \
            _data[thread_index].return_overrides[j + 1];                       \
        }                                                                      \
        _data[thread_index].return_overrides_size--;                           \
        (*ctf__mock_return_##name)[thread_index].size--;                       \
        if(_mock_f != NULL) _mock_f args;                                      \
        return temp;                                                           \
      }                                                                        \
    }                                                                          \
    if(_mock_f == NULL)                                                        \
      ctf__error("Mock return value not overriden and function not selected"); \
    return _mock_f args;                                                       \
  }                                                                            \
  static void ctf__mock_checks_##name(                                         \
    struct ctf__mock_state *ctf__mock_check_state, CTF__MACRO_VA typed)
#define CTF_MOCK_VOID_RET_STATIC(name, typed, args)                        \
  static struct ctf__mock ctf__mock_struct_##name;                         \
  static struct ctf__mock_struct_##name {                                  \
    void *values;                                                          \
    uintmax_t size;                                                        \
    uintmax_t capacity;                                                    \
  } **ctf__mock_return_##name =                                            \
    (struct ctf__mock_struct_##name **)&(ctf__mock_struct_##name.returns); \
  static void ctf__mock_checks_##name(struct ctf__mock_state *,            \
                                      CTF__MACRO_VA typed);                \
  static void __real_##name typed;                                         \
  static void __wrap_##name typed {                                        \
    uintptr_t thread_index =                                               \
      (uintptr_t)pthread_getspecific(ctf__thread_index);                   \
    struct ctf__mock_state *_data = ctf__mock_struct_##name.states;        \
    void(*_mock_f) typed;                                                  \
    if(_data == NULL || !_data[thread_index].enabled)                      \
      return __real_##name args;                                           \
    _mock_f = (void(*) typed)_data[thread_index].mock_f;                   \
    _data[thread_index].call_count++;                                      \
    ctf__mock_checks_##name(ctf__mock_struct_##name.states + thread_index, \
                            CTF__MACRO_VA args);                           \
    if(_mock_f != NULL) _mock_f args;                                      \
  }                                                                        \
  static void ctf__mock_checks_##name(                                     \
    struct ctf__mock_state *ctf__mock_check_state, CTF__MACRO_VA typed)
#define CTF_MOCK_EXTERN(ret_type, name, typed)                                 \
  extern struct ctf__mock ctf__mock_struct_##name;                             \
  extern struct ctf__mock_struct_##name **ctf__mock_return_##name;             \
  void ctf__mock_checks_##name(struct ctf__mock_state *, CTF__MACRO_VA typed); \
  ret_type __real_##name typed;                                                \
  ret_type __wrap_##name typed;
#define CTF_MOCK_VOID_RET_EXTERN(name, typed)                                  \
  extern struct ctf__mock ctf__mock_struct_##name;                             \
  extern struct ctf__mock_struct_##name **ctf__mock_return_##name;             \
  void ctf__mock_checks_##name(struct ctf__mock_state *, CTF__MACRO_VA typed); \
  void __real_##name typed;                                                    \
  void __wrap_##name typed;
#define CTF_MOCK_BIND(fn, mock) \
  (struct ctf__mock_bind) { &ctf__mock_struct_##fn, mock }
#define CTF_MOCK_GROUP(name) \
  struct ctf__mock_bind ctf__mock_group_st_##name[CTF_CONST_MOCK_GROUP_SIZE]
#define CTF_MOCK_GROUP_EXTERN(name) \
  extern struct ctf__mock_bind      \
    ctf__mock_group_st_##name[CTF_CONST_MOCK_GROUP_SIZE];

extern pthread_rwlock_t ctf__mock_returns_lock;

void ctf__mock_reset(struct ctf__mock_state *state);
void ctf__mock_global(struct ctf__mock *mock, void (*f)(void));
void ctf__mock_group(struct ctf__mock_bind *bind);
void ctf__unmock_group(struct ctf__mock_bind *bind);
void ctf__mock_mem(struct ctf__mock_state *state, uintmax_t call_count,
                   int type, int, const char *, const char *, void *f,
                   const char *var, const void *val, uintmax_t l, int mem_type);
void ctf__mock_check_mem(struct ctf__mock_state *state, const void *v,
                         const char *v_print, uintmax_t step, int sign);
void ctf__mock_returns_ensure_allocated(struct ctf__mock *mock,
                                        uintptr_t thread_index);
void ctf__mock_return_nth(struct ctf__mock *mock, uintmax_t n,
                          struct ctf__mock_return *returns,
                          uintmax_t return_size, uintptr_t thread_index);
uintmax_t ctf__mock_return_capacity(uintmax_t cap);
void ctf__mock_ea_mem(struct ctf__mock_state *state, uintmax_t call_count,
                      const char *var, const char *cmp, const void *val,
                      uintmax_t l, const char *val_str, int assert,
                      int mem_type, int line, const char *file);

#define ctf_mock_group(name) ctf__mock_group(ctf__mock_group_st_##name)
#define ctf_mock_global(fn, mock) \
  ctf__mock_global(&ctf__mock_struct_##fn, (void (*)(void))mock)
#define ctf_mock(fn, mock)                                             \
  for(uintptr_t ctf__local_thread_index =                              \
                  (uintptr_t)pthread_getspecific(ctf__thread_index),   \
                ctf__local_end_flag = 0;                               \
      !ctf__local_end_flag; ctf__local_end_flag = 1)                   \
    for(ctf_mock_global(fn, mock); !ctf__local_end_flag; ctf_unmock()) \
      for(typeof(ctf__mock_return_##fn) ctf__mock_return_selected =    \
            ctf__mock_return_##fn;                                     \
          !ctf__local_end_flag || ctf__mock_return_selected != NULL;   \
          ctf__mock_return_selected = NULL, ctf__local_end_flag = 1)
#define ctf_mock_spy(fn) ctf_mock(fn, ctf_mock_real(fn))
#define ctf_mock_reset(fn)                       \
  ctf__mock_reset(ctf__mock_struct_##fn.states + \
                  (uintptr_t)pthread_getspecific(ctf__thread_index))
#define ctf_mock_select(fn)                                          \
  for(uintptr_t ctf__local_thread_index =                            \
                  (uintptr_t)pthread_getspecific(ctf__thread_index), \
                ctf__local_end_flag = 0;                             \
      !ctf__local_end_flag; ctf__local_end_flag = 1)                 \
    for(ctf_mock_reset(fn); !ctf__local_end_flag;)                   \
      for(typeof(ctf__mock_return_##fn) ctf__mock_return_selected =  \
            ctf__mock_return_##fn;                                   \
          !ctf__local_end_flag || ctf__mock_return_selected != NULL; \
          ctf__mock_return_selected = NULL, ctf__local_end_flag = 1)
void ctf_unmock(void);
#define ctf_unmock_group(name) ctf__unmock_group(ctf__mock_group_st_##name)
#define ctf_mock_call_count ((CTF__MOCK_STRUCT_SELECTED)->call_count)
#define ctf_mock_return(val) ctf_mock_return_nth(1, val)
#define ctf_mock_return_nth(n, val)                                           \
  do {                                                                        \
    ctf__mock_returns_ensure_allocated(CTF__MOCK_SELECTED,                    \
                                       ctf__local_thread_index);              \
    ctf__mock_return_nth(                                                     \
      CTF__MOCK_SELECTED, n,                                                  \
      (struct ctf__mock_return *)((*ctf__mock_return_selected) +              \
                                  ctf__local_thread_index),                   \
      sizeof((*ctf__mock_return_selected)[0].values[0]),                      \
      ctf__local_thread_index);                                               \
    (*ctf__mock_return_selected)[ctf__local_thread_index]                     \
      .values[(*ctf__mock_return_selected)[ctf__local_thread_index].size++] = \
      val;                                                                    \
  } while(0)
#define ctf_mock_real(name) __real_##name
#define ctf_mock_check_select(name)               \
  struct ctf__mock_state *ctf__mock_check_state = \
    ctf__mock_struct_##name.states +              \
    (uintptr_t)pthread_getspecific(ctf__thread_index)

#if __STDC_VERSION__ >= 201112L
#define ctf_mock_check(name)            \
  _Generic((name),                      \
    char: ctf__mock_check_char,         \
    int8_t: ctf__mock_check_int,        \
    int16_t: ctf__mock_check_int,       \
    int32_t: ctf__mock_check_int,       \
    int64_t: ctf__mock_check_int,       \
    uint8_t: ctf__mock_check_uint,      \
    uint16_t: ctf__mock_check_uint,     \
    uint32_t: ctf__mock_check_uint,     \
    uint64_t: ctf__mock_check_uint,     \
    float: ctf__mock_check_float,       \
    double: ctf__mock_check_float,      \
    long double: ctf__mock_check_float, \
    default: ctf__mock_check_ptr)(ctf__mock_check_state, name, #name)
#define ctf_mock_expect(var, cmp, val)                                        \
  _Generic((val),                                                             \
    char: ctf__mock_ea_char,                                                  \
    int8_t: ctf__mock_ea_int,                                                 \
    int16_t: ctf__mock_ea_int,                                                \
    int32_t: ctf__mock_ea_int,                                                \
    int64_t: ctf__mock_ea_int,                                                \
    uint8_t: ctf__mock_ea_uint,                                               \
    uint16_t: ctf__mock_ea_uint,                                              \
    uint32_t: ctf__mock_ea_uint,                                              \
    uint64_t: ctf__mock_ea_uint,                                              \
    float: ctf__mock_ea_float,                                                \
    double: ctf__mock_ea_float,                                               \
    long double: ctf__mock_ea_float,                                          \
    default: ctf__mock_ea_ptr)(CTF__MOCK_STRUCT_SELECTED, 1, #var, #cmp, val, \
                               #val, 0, __LINE__, __FILE__)
#define ctf_mock_assert(var, cmp, val)                                        \
  _Generic((val),                                                             \
    char: ctf__mock_ea_char,                                                  \
    int8_t: ctf__mock_ea_int,                                                 \
    int16_t: ctf__mock_ea_int,                                                \
    int32_t: ctf__mock_ea_int,                                                \
    int64_t: ctf__mock_ea_int,                                                \
    uint8_t: ctf__mock_ea_uint,                                               \
    uint16_t: ctf__mock_ea_uint,                                              \
    uint32_t: ctf__mock_ea_uint,                                              \
    uint64_t: ctf__mock_ea_uint,                                              \
    float: ctf__mock_ea_float,                                                \
    double: ctf__mock_ea_float,                                               \
    long double: ctf__mock_ea_float,                                          \
    default: ctf__mock_ea_ptr)(CTF__MOCK_STRUCT_SELECTED, 1, #var, #cmp, val, \
                               #val, 1, __LINE__, __FILE__)
#define ctf_mock_expect_nth(n, var, cmp, val)                                 \
  _Generic((val),                                                             \
    char: ctf__mock_ea_char,                                                  \
    int8_t: ctf__mock_ea_int,                                                 \
    int16_t: ctf__mock_ea_int,                                                \
    int32_t: ctf__mock_ea_int,                                                \
    int64_t: ctf__mock_ea_int,                                                \
    uint8_t: ctf__mock_ea_uint,                                               \
    uint16_t: ctf__mock_ea_uint,                                              \
    uint32_t: ctf__mock_ea_uint,                                              \
    uint64_t: ctf__mock_ea_uint,                                              \
    float: ctf__mock_ea_float,                                                \
    double: ctf__mock_ea_float,                                               \
    long double: ctf__mock_ea_float,                                          \
    default: ctf__mock_ea_ptr)(CTF__MOCK_STRUCT_SELECTED, n, #var, #cmp, val, \
                               #val, 0, __LINE__, __FILE__)
#define ctf_mock_assert_nth(n, var, cmp, val)                                 \
  _Generic((val),                                                             \
    char: ctf__mock_ea_char,                                                  \
    int8_t: ctf__mock_ea_int,                                                 \
    int16_t: ctf__mock_ea_int,                                                \
    int32_t: ctf__mock_ea_int,                                                \
    int64_t: ctf__mock_ea_int,                                                \
    uint8_t: ctf__mock_ea_uint,                                               \
    uint16_t: ctf__mock_ea_uint,                                              \
    uint32_t: ctf__mock_ea_uint,                                              \
    uint64_t: ctf__mock_ea_uint,                                              \
    float: ctf__mock_ea_float,                                                \
    double: ctf__mock_ea_float,                                               \
    long double: ctf__mock_ea_float,                                          \
    default: ctf__mock_ea_ptr)(CTF__MOCK_STRUCT_SELECTED, n, #var, #cmp, val, \
                               #val, 1, __LINE__, __FILE__)

#define ctf_mock_expect_mem(var, cmp, val, l)                                 \
  ctf__mock_ea_mem(CTF__MOCK_STRUCT_SELECTED, 1, #var, #cmp, val, l, #val, 0, \
                   _Generic(*(val),                                           \
                     char: CTF__EA_MEM_TYPE_char,                             \
                     int8_t: CTF__EA_MEM_TYPE_int,                            \
                     int16_t: CTF__EA_MEM_TYPE_int,                           \
                     int32_t: CTF__EA_MEM_TYPE_int,                           \
                     int64_t: CTF__EA_MEM_TYPE_int,                           \
                     uint8_t: CTF__EA_MEM_TYPE_uint,                          \
                     uint16_t: CTF__EA_MEM_TYPE_uint,                         \
                     uint32_t: CTF__EA_MEM_TYPE_uint,                         \
                     uint64_t: CTF__EA_MEM_TYPE_uint,                         \
                     float: CTF__EA_MEM_TYPE_float,                           \
                     double: CTF__EA_MEM_TYPE_float,                          \
                     long double: CTF__EA_MEM_TYPE_float,                     \
                     default: CTF__EA_MEM_TYPE_ptr),                          \
                   __LINE__, __FILE__)
#define ctf_mock_assert_mem(var, cmp, val, l)                                 \
  ctf__mock_ea_mem(CTF__MOCK_STRUCT_SELECTED, 1, #var, #cmp, val, l, #val, 1, \
                   _Generic(*(val),                                           \
                     char: CTF__EA_MEM_TYPE_char,                             \
                     int8_t: CTF__EA_MEM_TYPE_int,                            \
                     int16_t: CTF__EA_MEM_TYPE_int,                           \
                     int32_t: CTF__EA_MEM_TYPE_int,                           \
                     int64_t: CTF__EA_MEM_TYPE_int,                           \
                     uint8_t: CTF__EA_MEM_TYPE_uint,                          \
                     uint16_t: CTF__EA_MEM_TYPE_uint,                         \
                     uint32_t: CTF__EA_MEM_TYPE_uint,                         \
                     uint64_t: CTF__EA_MEM_TYPE_uint,                         \
                     float: CTF__EA_MEM_TYPE_float,                           \
                     double: CTF__EA_MEM_TYPE_float,                          \
                     long double: CTF__EA_MEM_TYPE_float,                     \
                     default: CTF__EA_MEM_TYPE_ptr),                          \
                   __LINE__, __FILE__)
#define ctf_mock_expect_arr(var, cmp, val)                        \
  ctf__mock_ea_mem(CTF__MOCK_STRUCT_SELECTED, 1, #var, #cmp, val, \
                   sizeof(val) / sizeof(*(val)), #val, 0,         \
                   _Generic(*(val),                               \
                     char: CTF__EA_MEM_TYPE_char,                 \
                     int8_t: CTF__EA_MEM_TYPE_int,                \
                     int16_t: CTF__EA_MEM_TYPE_int,               \
                     int32_t: CTF__EA_MEM_TYPE_int,               \
                     int64_t: CTF__EA_MEM_TYPE_int,               \
                     uint8_t: CTF__EA_MEM_TYPE_uint,              \
                     uint16_t: CTF__EA_MEM_TYPE_uint,             \
                     uint32_t: CTF__EA_MEM_TYPE_uint,             \
                     uint64_t: CTF__EA_MEM_TYPE_uint,             \
                     float: CTF__EA_MEM_TYPE_float,               \
                     double: CTF__EA_MEM_TYPE_float,              \
                     long double: CTF__EA_MEM_TYPE_float,         \
                     default: CTF__EA_MEM_TYPE_ptr),              \
                   __LINE__, __FILE__)
#define ctf_mock_assert_arr(var, cmp, val)                      \
ctf__mock_ea_mem(CTF__MOCK_STRUCT_SELECTED, 1, #var, #cmp, val, \
                 sizeof(val)/sizeof(*(val), #val, 1, \
                   _Generic(*(val),                                           \
                     char: CTF__EA_MEM_TYPE_char,                       \
                     int8_t: CTF__EA_MEM_TYPE_int,                      \
                     int16_t: CTF__EA_MEM_TYPE_int,                     \
                     int32_t: CTF__EA_MEM_TYPE_int,                     \
                     int64_t: CTF__EA_MEM_TYPE_int,                     \
                     uint8_t: CTF__EA_MEM_TYPE_uint,                    \
                     uint16_t: CTF__EA_MEM_TYPE_uint,                   \
                     uint32_t: CTF__EA_MEM_TYPE_uint,                   \
                     uint64_t: CTF__EA_MEM_TYPE_uint,                   \
                     float: CTF__EA_MEM_TYPE_float,                     \
                     double: CTF__EA_MEM_TYPE_float,                    \
                     long double: CTF__EA_MEM_TYPE_float,               \
                     default: CTF__EA_MEM_TYPE_ptr),                    \
                   __LINE__, __FILE__)
#endif
#define MOCK_FUNCTION(type, TYPE)                                        \
  void ctf__mock_ea_##type(struct ctf__mock_state *state, uintmax_t,     \
                           const char *, const char *, CTF__TYPE_##TYPE, \
                           const char *, int, int, const char *);
#define MOCK_MEM_FUNCTION(ea, EA)                                     \
  void ctf__mock_##ea##_mem(struct ctf__mock_state *, uintmax_t, int, \
                            const char *, const char *, const char *, \
                            const char *, const void *, uintmax_t, int);
#define MOCK_CHECK_FUNCTION(type, TYPE)                                   \
  void ctf__mock_check_##type(struct ctf__mock_state *, CTF__TYPE_##TYPE, \
                              const char *);
// clang-format off
/*
define(`MOCK_EA_TEMPLATE', `#define $3mock_$2_$1(var, cmp, val) ctf__mock_ea_$1(CTF__MOCK_STRUCT_SELECTED, 1, #var, #cmp, val, #val, CTF__EA_FLAG_$2, __LINE__, __FILE__)')
define(`MOCK_EA_MEM_TEMPLATE', `#define $3mock_$2_mem_$1(var, cmp, val, length) ctf__mock_ea_mem(CTF__MOCK_STRUCT_SELECTED, 1, #var, #cmp, val, length, #val, CTF__EA_FLAG_$2, CTF__EA_MEM_TYPE_$1, __LINE__, __FILE__);')
define(`MOCK_EA_ARR_TEMPLATE', `#define $3mock_$2_arr_$1(var, cmp, val) ctf__mock_ea_mem(CTF__MOCK_STRUCT_SELECTED, 1, #var, #cmp, val, sizeof(val)/sizeof(*(val)), #val, CTF__EA_FLAG_$2, CTF__EA_MEM_TYPE_$1, __LINE__, __FILE__);')
define(`MOCK_EA_NTH_TEMPLATE', `#define $3mock_$2_nth_$1(n, var, cmp, val) ctf__mock_ea_$1(CTF__MOCK_STRUCT_SELECTED, n, #var, #cmp, val, #val, CTF__EA_FLAG_$2, __LINE__, __FILE__)')
define(`MOCK_EA_NTH_MEM_TEMPLATE', `#define $3mock_$2_nth_mem_$1(n, var, cmp, val, length) ctf__mock_ea_mem(CTF__MOCK_STRUCT_SELECTED, n, #var, #cmp, val, length, #val, CTF__EA_FLAG_$2, CTF__EA_MEM_TYPE_$1, __LINE__, __FILE__);')
define(`MOCK_EA_NTH_ARR_TEMPLATE', `#define $3mock_$2_nth_arr_$1(n, var, cmp, val) ctf__mock_ea_mem(CTF__MOCK_STRUCT_SELECTED, n, #var, #cmp, val, sizeof(val)/sizeof(*(val)), #val, CTF__EA_FLAG_$2, CTF__EA_MEM_TYPE_$1, __LINE__, __FILE__);')

define(`MOCK_CHECK', `#define $1mock_check_$2(v) ctf__mock_check_$2(ctf__mock_check_state, v, #v)')
define(`MOCK_CHECK_STRING', `#define $1mock_check_str(v) \
do { \
ctf__mock_check_ptr(ctf__mock_check_state, v, #v); \
ctf__mock_check_str(ctf__mock_check_state, v, #v); \
} while(0)')
define(`MOCK_CHECK_MEM',
`format(`#define $1mock_check_mem_$2(v) \
do { \
ctf__mock_check_ptr(ctf__mock_check_state, v, #v); \
ctf__mock_check_mem(ctf__mock_check_state, v, #v, sizeof(*(v)), %d);\
} while(0)', ifelse(`$2',`int', 1, `$2',`float',2,0))')
*/
COMB2(`RUN1', `(MOCK_FUNCTION)', `(PRIMITIVE_TYPES, str)')
COMB2(`RUN1', `(MOCK_CHECK_FUNCTION)', `(PRIMITIVE_TYPES, str)')
EA_FACTORY(`MOCK_EA_TEMPLATE',         `(PRIMITIVE_TYPES, str)', `ctf_')
EA_FACTORY(`MOCK_EA_MEM_TEMPLATE',     `(PRIMITIVE_TYPES)', `ctf_')
EA_FACTORY(`MOCK_EA_ARR_TEMPLATE',     `(PRIMITIVE_TYPES)', `ctf_')
EA_FACTORY(`MOCK_EA_NTH_TEMPLATE',     `(PRIMITIVE_TYPES, str)', `ctf_')
EA_FACTORY(`MOCK_EA_NTH_MEM_TEMPLATE', `(PRIMITIVE_TYPES)', `ctf_')
EA_FACTORY(`MOCK_EA_NTH_ARR_TEMPLATE', `(PRIMITIVE_TYPES)', `ctf_')
SCOMB(`MOCK_CHECK', `ctf_', `(PRIMITIVE_TYPES)')
MOCK_CHECK_STRING(`ctf_')
SCOMB(`MOCK_CHECK_MEM', `ctf_', `(PRIMITIVE_TYPES)')
// clang-format on

#if CTF_MOCK_ALIASES == CTF_ON
#if __STDC_VERSION__ >= 201112L
#define mock_check(name)                \
  _Generic((name),                      \
    char: ctf__mock_check_char,         \
    int8_t: ctf__mock_check_int,        \
    int16_t: ctf__mock_check_int,       \
    int32_t: ctf__mock_check_int,       \
    int64_t: ctf__mock_check_int,       \
    uint8_t: ctf__mock_check_uint,      \
    uint16_t: ctf__mock_check_uint,     \
    uint32_t: ctf__mock_check_uint,     \
    uint64_t: ctf__mock_check_uint,     \
    float: ctf__mock_check_float,       \
    double: ctf__mock_check_float,      \
    long double: ctf__mock_check_float, \
    default: ctf__mock_check_ptr)(ctf__mock_check_state, name, #name)
#define mock_expect(var, cmp, val)                                            \
  _Generic((val),                                                             \
    char: ctf__mock_ea_char,                                                  \
    int8_t: ctf__mock_ea_int,                                                 \
    int16_t: ctf__mock_ea_int,                                                \
    int32_t: ctf__mock_ea_int,                                                \
    int64_t: ctf__mock_ea_int,                                                \
    uint8_t: ctf__mock_ea_uint,                                               \
    uint16_t: ctf__mock_ea_uint,                                              \
    uint32_t: ctf__mock_ea_uint,                                              \
    uint64_t: ctf__mock_ea_uint,                                              \
    float: ctf__mock_ea_float,                                                \
    double: ctf__mock_ea_float,                                               \
    long double: ctf__mock_ea_float,                                          \
    default: ctf__mock_ea_ptr)(CTF__MOCK_STRUCT_SELECTED, 1, #var, #cmp, val, \
                               #val, 0, __LINE__, __FILE__)
#define mock_assert(var, cmp, val)                                            \
  _Generic((val),                                                             \
    char: ctf__mock_ea_char,                                                  \
    int8_t: ctf__mock_ea_int,                                                 \
    int16_t: ctf__mock_ea_int,                                                \
    int32_t: ctf__mock_ea_int,                                                \
    int64_t: ctf__mock_ea_int,                                                \
    uint8_t: ctf__mock_ea_uint,                                               \
    uint16_t: ctf__mock_ea_uint,                                              \
    uint32_t: ctf__mock_ea_uint,                                              \
    uint64_t: ctf__mock_ea_uint,                                              \
    float: ctf__mock_ea_float,                                                \
    double: ctf__mock_ea_float,                                               \
    long double: ctf__mock_ea_float,                                          \
    default: ctf__mock_ea_ptr)(CTF__MOCK_STRUCT_SELECTED, 1, #var, #cmp, val, \
                               #val, 1, __LINE__, __FILE__)
#define mock_expect_nth(n, var, cmp, val)                                     \
  _Generic((val),                                                             \
    char: ctf__mock_ea_char,                                                  \
    int8_t: ctf__mock_ea_int,                                                 \
    int16_t: ctf__mock_ea_int,                                                \
    int32_t: ctf__mock_ea_int,                                                \
    int64_t: ctf__mock_ea_int,                                                \
    uint8_t: ctf__mock_ea_uint,                                               \
    uint16_t: ctf__mock_ea_uint,                                              \
    uint32_t: ctf__mock_ea_uint,                                              \
    uint64_t: ctf__mock_ea_uint,                                              \
    float: ctf__mock_ea_float,                                                \
    double: ctf__mock_ea_float,                                               \
    long double: ctf__mock_ea_float,                                          \
    default: ctf__mock_ea_ptr)(CTF__MOCK_STRUCT_SELECTED, n, #var, #cmp, val, \
                               #val, 0, __LINE__, __FILE__)
#define mock_assert_nth(n, var, cmp, val)                                     \
  _Generic((val),                                                             \
    char: ctf__mock_ea_char,                                                  \
    int8_t: ctf__mock_ea_int,                                                 \
    int16_t: ctf__mock_ea_int,                                                \
    int32_t: ctf__mock_ea_int,                                                \
    int64_t: ctf__mock_ea_int,                                                \
    uint8_t: ctf__mock_ea_uint,                                               \
    uint16_t: ctf__mock_ea_uint,                                              \
    uint32_t: ctf__mock_ea_uint,                                              \
    uint64_t: ctf__mock_ea_uint,                                              \
    float: ctf__mock_ea_float,                                                \
    double: ctf__mock_ea_float,                                               \
    long double: ctf__mock_ea_float,                                          \
    default: ctf__mock_ea_ptr)(CTF__MOCK_STRUCT_SELECTED, n, #var, #cmp, val, \
                               #val, 1, __LINE__, __FILE__)

#define mock_expect_mem(var, cmp, val, l)                                     \
  ctf__mock_ea_mem(CTF__MOCK_STRUCT_SELECTED, 1, #var, #cmp, val, l, #val, 0, \
                   _Generic(*(val),                                           \
                     char: CTF__EA_MEM_TYPE_char,                             \
                     int8_t: CTF__EA_MEM_TYPE_int,                            \
                     int16_t: CTF__EA_MEM_TYPE_int,                           \
                     int32_t: CTF__EA_MEM_TYPE_int,                           \
                     int64_t: CTF__EA_MEM_TYPE_int,                           \
                     uint8_t: CTF__EA_MEM_TYPE_uint,                          \
                     uint16_t: CTF__EA_MEM_TYPE_uint,                         \
                     uint32_t: CTF__EA_MEM_TYPE_uint,                         \
                     uint64_t: CTF__EA_MEM_TYPE_uint,                         \
                     float: CTF__EA_MEM_TYPE_float,                           \
                     double: CTF__EA_MEM_TYPE_float,                          \
                     long double: CTF__EA_MEM_TYPE_float,                     \
                     default: CTF__EA_MEM_TYPE_ptr),                          \
                   __LINE__, __FILE__)
#define mock_assert_mem(var, cmp, val, l)                                     \
  ctf__mock_ea_mem(CTF__MOCK_STRUCT_SELECTED, 1, #var, #cmp, val, l, #val, 1, \
                   _Generic(*(val),                                           \
                     char: CTF__EA_MEM_TYPE_char,                             \
                     int8_t: CTF__EA_MEM_TYPE_int,                            \
                     int16_t: CTF__EA_MEM_TYPE_int,                           \
                     int32_t: CTF__EA_MEM_TYPE_int,                           \
                     int64_t: CTF__EA_MEM_TYPE_int,                           \
                     uint8_t: CTF__EA_MEM_TYPE_uint,                          \
                     uint16_t: CTF__EA_MEM_TYPE_uint,                         \
                     uint32_t: CTF__EA_MEM_TYPE_uint,                         \
                     uint64_t: CTF__EA_MEM_TYPE_uint,                         \
                     float: CTF__EA_MEM_TYPE_float,                           \
                     double: CTF__EA_MEM_TYPE_float,                          \
                     long double: CTF__EA_MEM_TYPE_float,                     \
                     default: CTF__EA_MEM_TYPE_ptr),                          \
                   __LINE__, __FILE__)
#define mock_expect_arr(var, cmp, val)                            \
  ctf__mock_ea_mem(CTF__MOCK_STRUCT_SELECTED, 1, #var, #cmp, val, \
                   sizeof(val) / sizeof(*(val)), #val, 0,         \
                   _Generic(*(val),                               \
                     char: CTF__EA_MEM_TYPE_char,                 \
                     int8_t: CTF__EA_MEM_TYPE_int,                \
                     int16_t: CTF__EA_MEM_TYPE_int,               \
                     int32_t: CTF__EA_MEM_TYPE_int,               \
                     int64_t: CTF__EA_MEM_TYPE_int,               \
                     uint8_t: CTF__EA_MEM_TYPE_uint,              \
                     uint16_t: CTF__EA_MEM_TYPE_uint,             \
                     uint32_t: CTF__EA_MEM_TYPE_uint,             \
                     uint64_t: CTF__EA_MEM_TYPE_uint,             \
                     float: CTF__EA_MEM_TYPE_float,               \
                     double: CTF__EA_MEM_TYPE_float,              \
                     long double: CTF__EA_MEM_TYPE_float,         \
                     default: CTF__EA_MEM_TYPE_ptr),              \
                   __LINE__, __FILE__)
#define mock_assert_arr(var, cmp, val)                            \
  ctf__mock_ea_mem(CTF__MOCK_STRUCT_SELECTED, 1, #var, #cmp, val, \
                   sizeof(val) / sizeof(*(val)), #val, 1,         \
                   _Generic(*(val),                               \
                     char: CTF__EA_MEM_TYPE_char,                 \
                     int8_t: CTF__EA_MEM_TYPE_int,                \
                     int16_t: CTF__EA_MEM_TYPE_int,               \
                     int32_t: CTF__EA_MEM_TYPE_int,               \
                     int64_t: CTF__EA_MEM_TYPE_int,               \
                     uint8_t: CTF__EA_MEM_TYPE_uint,              \
                     uint16_t: CTF__EA_MEM_TYPE_uint,             \
                     uint32_t: CTF__EA_MEM_TYPE_uint,             \
                     uint64_t: CTF__EA_MEM_TYPE_uint,             \
                     float: CTF__EA_MEM_TYPE_float,               \
                     double: CTF__EA_MEM_TYPE_float,              \
                     long double: CTF__EA_MEM_TYPE_float,         \
                     default: CTF__EA_MEM_TYPE_ptr),              \
                   __LINE__, __FILE__)
#endif
// clang-format off
EA_FACTORY(`MOCK_EA_TEMPLATE',         `(PRIMITIVE_TYPES, str)', `')
EA_FACTORY(`MOCK_EA_MEM_TEMPLATE',     `(PRIMITIVE_TYPES)', `')
EA_FACTORY(`MOCK_EA_ARR_TEMPLATE',     `(PRIMITIVE_TYPES)', `')
EA_FACTORY(`MOCK_EA_NTH_TEMPLATE',     `(PRIMITIVE_TYPES, str)', `')
EA_FACTORY(`MOCK_EA_NTH_MEM_TEMPLATE', `(PRIMITIVE_TYPES)', `')
EA_FACTORY(`MOCK_EA_NTH_ARR_TEMPLATE', `(PRIMITIVE_TYPES)', `')
SCOMB(`MOCK_CHECK', `', `(PRIMITIVE_TYPES)')
MOCK_CHECK_STRING(`')
SCOMB(`MOCK_CHECK_MEM', `', `(PRIMITIVE_TYPES)')
COMB(`ALIAS',
     `(mock_global(name, f), mock(name, f), mock_spy(name),
       unmock(), mock_select(fn), mock_group(name), unmock_group(name),
       mock_call_count, mock_real(name), mock_check_select(name),
       mock_return(val), mock_return_nth(n, val))')
// clang-format on
#endif
