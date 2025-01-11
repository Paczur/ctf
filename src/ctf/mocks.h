#define CTF_CONST_MOCK_GROUP_SIZE 64

#include <stddef.h>
#include <stdlib.h>

#define CTF__MOCK_SELECTED                                    \
  ((struct ctf__mock *)(((void *)ctf__mock_return_selected) - \
                        offsetof(struct ctf__mock, returns)))
#define CTF__MOCK_STRUCT_SELECTED \
  ((CTF__MOCK_SELECTED)->states + ctf__local_thread_index)

struct ctf__check {
  uint8_t flags;
  int line;
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
  const char *cmp;
  uintmax_t length;
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

#define CTF__MOCK_TEMPLATE(mod, wrapped, real, ret_type, name, typed_comb,     \
                           typed, args)                                        \
  mod ret_type ctf__mock_checks_##name(                                        \
    struct ctf__mock_state *ctf__mock_check_state, int,                        \
    ret_type *ctf__mock_return_value CTF__MACRO_VA_COMMA typed_comb);          \
  mod ret_type real typed;                                                     \
  mod ret_type ctf__mock_fn_real_##name typed { return real args; }            \
  mod struct ctf__mock ctf__mock_struct_##name;                                \
  mod struct ctf__mock_struct_##name **ctf__mock_return_##name =               \
    (struct ctf__mock_struct_##name **)&(ctf__mock_struct_##name.returns);     \
  mod ret_type wrapped typed {                                                 \
    uintptr_t thread_index =                                                   \
      (uintptr_t)pthread_getspecific(ctf__thread_index);                       \
    struct ctf__mock_state *_data = ctf__mock_struct_##name.states;            \
    ret_type(*_mock_f) typed;                                                  \
    ret_type temp;                                                             \
    ret_type out;                                                              \
    int temp_set = 0;                                                          \
    if(_data == NULL || !_data[thread_index].enabled) return real args;        \
    _mock_f = (ret_type(*) typed)_data[thread_index].mock_f;                   \
    _data[thread_index].call_count++;                                          \
    ctf_subtest(name) {                                                        \
      ctf_subtest(in)                                                          \
        ctf__mock_checks_##name(ctf__mock_struct_##name.states + thread_index, \
                                0, NULL CTF__MACRO_VA_COMMA args);             \
      for(uintmax_t i = 0; i < _data[thread_index].return_overrides_size;      \
          i++) {                                                               \
        if(_data[thread_index].return_overrides[i] ==                          \
           _data[thread_index].call_count) {                                   \
          temp = (*ctf__mock_return_##name)[thread_index].values[i];           \
          temp_set = 1;                                                        \
          for(uintmax_t j = i;                                                 \
              j < _data[thread_index].return_overrides_size - 1; j++) {        \
            (*ctf__mock_return_##name)[thread_index].values[j] =               \
              (*ctf__mock_return_##name)[thread_index].values[j + 1];          \
            _data[thread_index].return_overrides[j] =                          \
              _data[thread_index].return_overrides[j + 1];                     \
          }                                                                    \
          _data[thread_index].return_overrides_size--;                         \
          (*ctf__mock_return_##name)[thread_index].size--;                     \
          break;                                                               \
        }                                                                      \
      }                                                                        \
      if(temp_set) {                                                           \
        if(_mock_f != NULL) _mock_f args;                                      \
        ctf_subtest(out) ctf__mock_checks_##name(                              \
          ctf__mock_struct_##name.states + thread_index, 1,                    \
          &out CTF__MACRO_VA_COMMA args);                                      \
      } else {                                                                 \
        if(_mock_f != NULL) {                                                  \
          out = _mock_f args;                                                  \
          ctf_subtest(out) ctf__mock_checks_##name(                            \
            ctf__mock_struct_##name.states + thread_index, 1,                  \
            &out CTF__MACRO_VA_COMMA args);                                    \
        } else {                                                               \
          temp_set = 1;                                                        \
          ctf_subtest(out) temp = ctf__mock_checks_##name(                     \
            ctf__mock_struct_##name.states + thread_index, 1,                  \
            &out CTF__MACRO_VA_COMMA args);                                    \
        }                                                                      \
      }                                                                        \
    }                                                                          \
    return (temp_set) ? temp : out;                                            \
  }                                                                            \
  mod ret_type ctf__mock_checks_##name(                                        \
    struct ctf__mock_state *ctf__mock_check_state, int ctf__mock_out,          \
    ret_type *ctf__mock_return_value CTF__MACRO_VA_COMMA typed_comb)
#define CTF__MOCK_VOID_TEMPLATE(mod, wrapped, real, name, typed_comb, typed,   \
                                args)                                          \
  mod void ctf__mock_checks_##name(                                            \
    struct ctf__mock_state *ctf__mock_check_state,                             \
    int CTF__MACRO_VA_COMMA typed_comb);                                       \
  mod struct ctf__mock ctf__mock_struct_##name;                                \
  mod struct ctf__mock_struct_##name **ctf__mock_return_##name =               \
    (struct ctf__mock_struct_##name **)&(ctf__mock_struct_##name.returns);     \
  mod void ctf__mock_fn_real_##name typed { real args; }                       \
  mod void wrapped typed {                                                     \
    uintptr_t thread_index =                                                   \
      (uintptr_t)pthread_getspecific(ctf__thread_index);                       \
    struct ctf__mock_state *_data = ctf__mock_struct_##name.states;            \
    void(*_mock_f) typed;                                                      \
    if(_data == NULL || !_data[thread_index].enabled) {                        \
      real args;                                                               \
      return;                                                                  \
    }                                                                          \
    _mock_f = (void(*) typed)_data[thread_index].mock_f;                       \
    _data[thread_index].call_count++;                                          \
    ctf_subtest(name) {                                                        \
      ctf_subtest(in)                                                          \
        ctf__mock_checks_##name(ctf__mock_struct_##name.states + thread_index, \
                                0 CTF__MACRO_VA_COMMA args);                   \
      if(_mock_f != NULL) _mock_f args;                                        \
      ctf_subtest(out)                                                         \
        ctf__mock_checks_##name(ctf__mock_struct_##name.states + thread_index, \
                                1 CTF__MACRO_VA_COMMA args);                   \
    }                                                                          \
  }                                                                            \
  mod void ctf__mock_checks_##name(                                            \
    struct ctf__mock_state *ctf__mock_check_state,                             \
    int ctf__mock_out CTF__MACRO_VA_COMMA typed_comb)

#define CTF__MOCK_EXTERN_TEMPLATE(wrapped, real, ret_type, name, typed_comb, \
                                  typed)                                     \
  struct ctf__mock_struct_##name {                                           \
    ret_type *values;                                                        \
    uintmax_t size;                                                          \
    uintmax_t capacity;                                                      \
  };                                                                         \
  extern struct ctf__mock ctf__mock_struct_##name;                           \
  extern struct ctf__mock_struct_##name **ctf__mock_return_##name;           \
  extern ret_type ctf__mock_checks_##name(                                   \
    struct ctf__mock_state *ctf__mock_check_state, int,                      \
    ret_type *ctf__mock_return_value CTF__MACRO_VA_COMMA typed_comb);        \
  ret_type ctf__mock_fn_real_##name typed;                                   \
  ret_type real typed;                                                       \
  ret_type wrapped typed
#define CTF__MOCK_VOID_RET_EXTERN_TEMPLATE(wrapped, real, name, typed_comb, \
                                           typed)                           \
  struct ctf__mock_struct_##name {                                          \
    void *values;                                                           \
    uintmax_t size;                                                         \
    uintmax_t capacity;                                                     \
  };                                                                        \
  extern struct ctf__mock ctf__mock_struct_##name;                          \
  extern struct ctf__mock_struct_##name **ctf__mock_return_##name;          \
  extern void ctf__mock_fn_real_##name typed;                               \
  extern void ctf__mock_checks_##name(                                      \
    struct ctf__mock_state *ctf__mock_check_state,                          \
    int CTF__MACRO_VA_COMMA typed_comb);                                    \
  void real typed;                                                          \
  void wrapped typed

#define CTF_MOCK_CUSTOM(real, ret_type, name, typed, args) \
  CTF__MOCK_TEMPLATE(, name, real, ret_type, name, typed, typed, args)
#define CTF_MOCK_VOID_ARG_CUSTOM(real, ret_type, name) \
  CTF__MOCK_TEMPLATE(, name, real, ret_type, name, (), (void), ())
#define CTF_MOCK_VOID_RET_CUSTOM(real, name, typed, args) \
  CTF__MOCK_VOID_TEMPLATE(, name, real, name, typed, typed, args)
#define CTF_MOCK_VOID_CUSTOM(real, name) \
  CTF__MOCK_VOID_TEMPLATE(, name, real, name, (), (void), ())

#define CTF_MOCK(ret_type, name, typed, args)                               \
  CTF__MOCK_TEMPLATE(, __wrap_##name, __real_##name, ret_type, name, typed, \
                     typed, args)
#define CTF_MOCK_VOID_ARG(ret_type, name)                                \
  CTF__MOCK_TEMPLATE(, __wrap_##name, __real_##name, ret_type, name, (), \
                     (void), ())
#define CTF_MOCK_VOID_RET(name, typed, args)                                  \
  CTF__MOCK_VOID_TEMPLATE(, __wrap_##name, __real_##name, name, typed, typed, \
                          args)
#define CTF_MOCK_VOID(name) \
  CTF__MOCK_VOID_TEMPLATE(, __wrap_##name, __real_##name, name, (), (void), ())

#define CTF_MOCK_CUSTOM_STATIC(real, ret_type, name, typed, args) \
  CTF__MOCK_TEMPLATE(static, name, real, ret_type, name, typed, typed, args)
#define CTF_MOCK_VOID_ARG_CUSTOM_STATIC(real, ret_type, name) \
  CTF__MOCK_TEMPLATE(static, name, real, ret_type, name, (), (void), ())
#define CTF_MOCK_VOID_RET_CUSTOM_STATIC(real, name, typed, args) \
  CTF__MOCK_VOID_TEMPLATE(static, name, real, name, typed, typed, args)
#define CTF_MOCK_VOID_CUSTOM_STATIC(real, name) \
  CTF__MOCK_VOID_TEMPLATE(static, name, real, name, (), (void), ())

#define CTF_MOCK_STATIC(ret_type, name, typed, args)                       \
  CTF__MOCK_TEMPLATE(static, __wrap_##name, __real_##name, ret_type, name, \
                     typed, typed, args)
#define CTF_MOCK_VOID_ARG_STATIC(ret_type, name)                               \
  CTF__MOCK_TEMPLATE(static, __wrap_##name, __real_##name, ret_type, name, (), \
                     (void), ())
#define CTF_MOCK_VOID_RET_STATIC(name, typed, args)                          \
  CTF__MOCK_VOID_TEMPLATE(static, __wrap_##name, __real_##name, name, typed, \
                          typed, args)
#define CTF_MOCK_VOID_STATIC(name)                                        \
  CTF__MOCK_VOID_TEMPLATE(static, __wrap_##name, __real_##name, name, (), \
                          (void), ())

#define CTF_MOCK_EXTERN(ret_type, name, typed)                            \
  CTF__MOCK_EXTERN_TEMPLATE(__wrap_##name, __real_##name, ret_type, name, \
                            typed, typed)
#define CTF_MOCK_VOID_RET_EXTERN(name, typed)                            \
  CTF__MOCK_VOID_RET_EXTERN_TEMPLATE(__wrap_##name, __real_##name, name, \
                                     typed, typed)
#define CTF_MOCK_VOID_ARG_EXTERN(ret_type, name)                              \
  CTF__MOCK_EXTERN_TEMPLATE(__wrap_##name, __real_##name, ret_type, name, (), \
                            (void))
#define CTF_MOCK_VOID_EXTERN(name)                                           \
  CTF__MOCK_VOID_RET_EXTERN_TEMPLATE(__wrap_##name, __real_##name, name, (), \
                                     (void))

#define CTF_MOCK_CUSTOM_EXTERN(real, ret_type, name, typed) \
  CTF__MOCK_EXTERN_TEMPLATE(name, real, ret_type, name, typed, typed)
#define CTF_MOCK_VOID_RET_CUSTOM_EXTERN(real, name, typed) \
  CTF__MOCK_VOID_RET_EXTERN_TEMPLATE(name, real, name, typed, typed)
#define CTF_MOCK_VOID_ARG_CUSTOM_EXTERN(real, ret_type, name) \
  CTF__MOCK_EXTERN_TEMPLATE(name, real, ret_type, name, (), (void))
#define CTF_MOCK_VOID_CUSTOM_EXTERN(real, name) \
  CTF__MOCK_VOID_RET_EXTERN_TEMPLATE(name, real, name, (), (void))

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
                         const char *v_print, int in_out, uintmax_t step,
                         int sign);
void ctf__mock_returns_ensure_allocated(struct ctf__mock *mock,
                                        uintptr_t thread_index);
void ctf__mock_return_nth(struct ctf__mock *mock, uintmax_t n,
                          struct ctf__mock_return *returns,
                          uintmax_t return_size, uintptr_t thread_index);
uintmax_t ctf__mock_return_capacity(uintmax_t cap);
void ctf__mock_ea_mem(struct ctf__mock_state *state, uintmax_t call_count,
                      const char *var, const char *cmp, const void *val,
                      uintmax_t l, const char *val_str, int flags, int line,
                      const char *file);

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
#define ctf_mock_out ctf__mock_out
#define ctf_mock_in (!ctf__mock_out)
#define ctf_mock_return_value (*ctf__mock_return_value)
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
#define ctf_mock_real(name) (ctf__mock_fn_real_##name)
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
    default: ctf__mock_check_ptr)(      \
    ctf__mock_check_state, name, #name, \
    (ctf_mock_out) ? CTF__MOCK_CHECK_DIR_OUT : CTF__MOCK_CHECK_DIR_IN)
// clang-format off
/*
(prefix, ea, inout, n)
define(`MOCK_EA_GENERIC',
`format(
`#define $1mock_$2$3$4(%svar, cmp, val)                   \
  _Generic((val),                                        \
    char: ctf__mock_ea_char,                             \
    int8_t: ctf__mock_ea_int,                            \
    int16_t: ctf__mock_ea_int,                           \
    int32_t: ctf__mock_ea_int,                           \
    int64_t: ctf__mock_ea_int,                           \
    uint8_t: ctf__mock_ea_uint,                          \
    uint16_t: ctf__mock_ea_uint,                         \
    uint32_t: ctf__mock_ea_uint,                         \
    uint64_t: ctf__mock_ea_uint,                         \
    float: ctf__mock_ea_float,                           \
    double: ctf__mock_ea_float,                          \
    long double: ctf__mock_ea_float,                     \
    default: ctf__mock_ea_ptr)(                          \
    CTF__MOCK_STRUCT_SELECTED, %s, #var, #cmp, val, #val, \
    CTF__MOCK_TYPE_$2 | CTF__MOCK_CHECK_DIR_%s, __LINE__, __FILE__)',
    ifelse(`$4', `_nth', ``n, '', `'),
    ifelse(`$4', `_nth', `n', `0'),
    ifelse(`$3', `_in', `IN', `$3', `_out', `OUT', `INOUT'))')

define(`MOCK_EA_MEM_GENERIC',
`format(
`#define $1mock_$2$3$4_$5(%svar, cmp, val%s)                              \
  ctf__mock_ea_mem(CTF__MOCK_STRUCT_SELECTED, %s, #var, #cmp, val, %s, #val, \
                   CTF__MOCK_TYPE_$2 | CTF__MOCK_CHECK_DIR_%s |     \
                     _Generic(*(val),                                      \
                       char: CTF__DATA_TYPE_char,                        \
                       int8_t: CTF__DATA_TYPE_int,                       \
                       int16_t: CTF__DATA_TYPE_int,                      \
                       int32_t: CTF__DATA_TYPE_int,                      \
                       int64_t: CTF__DATA_TYPE_int,                      \
                       uint8_t: CTF__DATA_TYPE_uint,                     \
                       uint16_t: CTF__DATA_TYPE_uint,                    \
                       uint32_t: CTF__DATA_TYPE_uint,                    \
                       uint64_t: CTF__DATA_TYPE_uint,                    \
                       float: CTF__DATA_TYPE_float,                      \
                       double: CTF__DATA_TYPE_float,                     \
                       long double: CTF__DATA_TYPE_float,                \
                       default: CTF__DATA_TYPE_ptr),                     \
                       __LINE__, __FILE__)',
    ifelse(`$4', `_nth', ``n, '', `'),
    ifelse(`$5', `mem', ``, l'', `'),
    ifelse(`$4', `_nth', `n', `0'),
    ifelse(`$5', `mem', `l', `sizeof(val)/sizeof(*(val))'),
    ifelse(`$3', `_in', `IN', `$3', `_out', `OUT', `INOUT'))')
*/
COMB4(`MOCK_EA_GENERIC', `(ctf_)', `(EAS)', `(`_in', `_out', `')', `(`_nth', `')')
COMB5(`MOCK_EA_MEM_GENERIC', `(ctf_)', `(EAS)', `(`_in', `_out', `')', `(`_nth', `')', `(mem, arr)')
// clang-format on
#endif
#define MOCK_FUNCTION(type, TYPE)                                        \
  void ctf__mock_ea_##type(struct ctf__mock_state *state, uintmax_t,     \
                           const char *, const char *, CTF__TYPE_##TYPE, \
                           const char *, int, int, const char *);
#define MOCK_CHECK_FUNCTION(type, TYPE)                                   \
  void ctf__mock_check_##type(struct ctf__mock_state *, CTF__TYPE_##TYPE, \
                              const char *, int);
// clang-format off
/*
define(`MOCK_EA_TEMPLATE',
`format(
`#define $1mock_$2$3$4_$5(%svar, cmp, val) \
ctf__mock_ea_$5(CTF__MOCK_STRUCT_SELECTED, %s, #var, #cmp, val, #val, \
CTF__EA_FLAG_$2 | CTF__MOCK_CHECK_DIR_%s, __LINE__, __FILE__)',
    ifelse(`$4', `_nth', ``n, '', `'),
    ifelse(`$4', `_nth', `n', `0'),
    ifelse(`$3', `_in', `IN', `$3', `_out', `OUT', `INOUT'))')
)
define(`MOCK_EA_MEM_TEMPLATE',
`format(
`#define $1mock_$2$3$4_$5_$6(%svar, cmp, val%s)\
ctf__mock_ea_mem(CTF__MOCK_STRUCT_SELECTED, %s, #var, #cmp, val, %s, #val, \
CTF__EA_FLAG_$2 | CTF__DATA_TYPE_$1 | CTF__MOCK_CHECK_DIR_%s, __LINE__, \
__FILE__);',
    ifelse(`$4', `_nth', ``n, '', `'),
    ifelse(`$5', `mem', ``, l'', `'),
    ifelse(`$4', `_nth', `n', `0'),
    ifelse(`$5', `mem', `l', `sizeof(val)/sizeof(*(val))'),
    ifelse(`$3', `_in', `IN', `$3', `_out', `OUT', `INOUT'))')
)

define(`MOCK_CHECK', `#define $1mock_check_$2(v) ctf__mock_check_$2(ctf__mock_check_state, v, #v, (ctf_mock_out) ? CTF__MOCK_CHECK_DIR_OUT : CTF__MOCK_CHECK_DIR_IN)')
define(`MOCK_CHECK_STRING', `#define $1mock_check_str(v) \
do { \
ctf__mock_check_ptr(ctf__mock_check_state, v, #v, (ctf_mock_out) ? CTF__MOCK_CHECK_DIR_OUT : CTF__MOCK_CHECK_DIR_IN); \
ctf__mock_check_str(ctf__mock_check_state, v, #v, (ctf_mock_out) ? CTF__MOCK_CHECK_DIR_OUT : CTF__MOCK_CHECK_DIR_IN); \
} while(0)')
define(`MOCK_CHECK_MEM',
`format(`#define $1mock_check_mem_$2(v) \
do { \
ctf__mock_check_ptr(ctf__mock_check_state, v, #v, (ctf_mock_out) ? CTF__MOCK_CHECK_DIR_OUT : CTF__MOCK_CHECK_DIR_IN); \
ctf__mock_check_mem(ctf__mock_check_state, v, #v, (ctf_mock_out) ? CTF__MOCK_CHECK_DIR_OUT : CTF__MOCK_CHECK_DIR_IN, sizeof(*(v)), %d);\
} while(0)', ifelse(`$2',`int', 1, `$2',`float',2,0))')
*/
COMB2(`RUN1', `(MOCK_FUNCTION)', `(PRIMITIVE_TYPES, str)')
COMB2(`RUN1', `(MOCK_CHECK_FUNCTION)', `(PRIMITIVE_TYPES, str)')
COMB5(`MOCK_EA_TEMPLATE', `(ctf_)', `(EAS)', `(`_in', `_out', `')', `(`_nth', `')', `(PRIMITIVE_TYPES, str)')
COMB6(`MOCK_EA_MEM_TEMPLATE', `(ctf_)', `(EAS)', `(`_in', `_out', `')', `(`_nth', `')', `(mem, arr)', `(PRIMITIVE_TYPES, str)')
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
    default: ctf__mock_check_ptr)(      \
    ctf__mock_check_state, name, #name, \
    (ctf_mock_out) ? CTF__MOCK_CHECK_DIR_OUT : CTF__MOCK_CHECK_DIR_IN)
// clang-format off
COMB4(`MOCK_EA_GENERIC', `(`')', `(EAS)', `(`_in', `_out', `')', `(`_nth', `')')
COMB5(`MOCK_EA_MEM_GENERIC', `(`')', `(EAS)', `(`_in', `_out', `')', `(`_nth', `')', `(mem, arr)')
// clang-format on
#endif
// clang-format off
COMB5(`MOCK_EA_TEMPLATE', `(`')', `(EAS)', `(`_in', `_out', `')', `(`_nth', `')', `(PRIMITIVE_TYPES, str)')
COMB6(`MOCK_EA_MEM_TEMPLATE', `(`')', `(EAS)', `(`_in', `_out', `')', `(`_nth', `')', `(mem, arr)', `(PRIMITIVE_TYPES, str)')
SCOMB(`MOCK_CHECK', `', `(PRIMITIVE_TYPES)')
MOCK_CHECK_STRING(`')
SCOMB(`MOCK_CHECK_MEM', `', `(PRIMITIVE_TYPES)')
COMB(`ALIAS',
     `(mock_global(name, f), mock(name, f), mock_spy(name),
       unmock(), mock_select(fn), mock_group(name), unmock_group(name),
       mock_out, mock_return_value,
       mock_in,
       mock_call_count, mock_real(name), mock_check_select(name),
       mock_return(val), mock_return_nth(n, val))')
// clang-format on
#endif
