#define CTF__EA_MEM_TYPE_int 0
#define CTF__EA_MEM_TYPE_uint 1
#define CTF__EA_MEM_TYPE_ptr 2
#define CTF__EA_MEM_TYPE_char 3
#define CTF__EA_MEM_TYPE_float 4

#define CTF__EA_FLAG_expect 0
#define CTF__EA_FLAG_assert 1

#define CTF__EA_SIGN_uint 0
#define CTF__EA_SIGN_int 1
#define CTF__EA_SIGN_char 0
#define CTF__EA_SIGN_ptr 0
#define CTF__EA_SIGN_float 2

int ctf__ea_arr(const void *a, const char *cmp, const void *b, uintmax_t la,
                uintmax_t lb, const char *a_str, const char *b_str, int assert,
                uintmax_t step, int sign, int type, int line, const char *file);
void ctf__assert_fold(uintmax_t count, const char *msg, int line,
                      const char *file);
uintmax_t ctf__pass(const char *, int, const char *, ...);
uintmax_t ctf__fail(const char *, int, const char *, ...);
uintmax_t ctf__ea_msg(int, const char *, int, int, const char *, ...);

void ctf_assert_barrier(void);
void ctf_assert_hide(uintmax_t count);
#define ctf_assert_fold(count, msg) \
  ctf__assert_fold(count, msg, __LINE__, __FILE__)
#define ctf_pass(m, ...) ctf__pass(m, __LINE__, __FILE__, ##__VA_ARGS__)
#define ctf_fail(m, ...) ctf__fail(m, __LINE__, __FILE__, ##__VA_ARGS__)
#define ctf_assert_true(a) \
  ctf__ea_int(0, "!=", a, "0", #a, 1, __LINE__, __FILE__)
#define ctf_assert_false(a) \
  ctf__ea_int(0, "==", a, "0", #a, 1, __LINE__, __FILE__)
#define ctf_expect_true(a) \
  ctf__ea_int(0, "!=", a, "0", #a, 0, __LINE__, __FILE__)
#define ctf_expect_false(a) \
  ctf__ea_int(0, "==", a, "0", #a, 0, __LINE__, __FILE__)
#define ctf_assert_non_null(a) \
  ctf__ea_ptr(NULL, "!=", a, "NULL", #a, 1, __LINE__, __FILE__)
#define ctf_assert_null(a) \
  ctf__ea_ptr(NULL, "==", a, "NULL", #a, 1, __LINE__, __FILE__)
#define ctf_expect_non_null(a) \
  ctf__ea_ptr(NULL, "!=", a, "NULL", #a, 0, __LINE__, __FILE__)
#define ctf_expect_null(a) \
  ctf__ea_ptr(NULL, "==", a, "NULL", #a, 0, __LINE__, __FILE__)
#define ctf_assert_msg(p, m, ...) \
  ctf__ea_msg(p, m, 1, __LINE__, __FILE__, ##__VA_ARGS__)
#define ctf_expect_msg(p, m, ...) \
  ctf__ea_msg(p, m, 0, __LINE__, __FILE__, ##__VA_ARGS__)

#define CTF__EA_FUN(type, TYPE)                                               \
  int ctf__ea_##type(CTF__TYPE_##TYPE a, const char *cmp, CTF__TYPE_##TYPE b, \
                     const char *a_str, const char *b_str, int assert,        \
                     int line, const char *file);
// clang-format off
/*
define(`EA_FUNCTION',
`format(`int ctf__$3_$1_$2(%s, %s, const char *, const char *, int, const char *);',TYPE(`$1'),TYPE(`$1'))')dnl
define(`EA_TEMPLATE', `#define $3$2_$1(a, cmp, b) ctf__ea_$1(a, #cmp, b, #a, #b, CTF__EA_FLAG_$2, __LINE__, __FILE__)')dnl
define(`EA_MEM_TEMPLATE',
`#define $3$2_mem_$1(a, cmp, b, length) ctf__ea_arr((const void *)a, #cmp, (const void *)b, length, length, #a, #b, CTF__EA_FLAG_$2, sizeof(*(a)), CTF__EA_SIGN_$1, CTF__EA_MEM_TYPE_$1, __LINE__, __FILE__)')dnl
define(`EA_ARR_TEMPLATE',
`#define $3$2_arr_$1(a, cmp, b) ctf__ea_arr((const void *)a, #cmp, (const void *)b, sizeof(a)/sizeof(*(a)), sizeof(b)/sizeof(*(b)), #a, #b, CTF__EA_FLAG_$2, sizeof(*(a)), CTF__EA_SIGN_$1, CTF__EA_MEM_TYPE_$1, __LINE__, __FILE__)')dnl
define(`EA_FUNCTION', `format(`int ctf__$2_$1(%s, const char *, %s, const char *, const char *, int, const char*);',TYPE(`$1'),TYPE(`$1'))')
define(`EA_FACTORY', `foreach(`type', `$2',
`
indir(`$1', type, `assert', `$3')
indir(`$1', type, `expect', `$3')
')')')dnl
*/
EA_FACTORY(`EA_TEMPLATE',     `(PRIMITIVE_TYPES, str)', `ctf_')
EA_FACTORY(`EA_MEM_TEMPLATE', `(PRIMITIVE_TYPES)', `ctf_')
EA_FACTORY(`EA_ARR_TEMPLATE', `(PRIMITIVE_TYPES)', `ctf_')
EA_FACTORY(`EA_FUNCTION',     `(PRIMITIVE_TYPES, str)', `ctf_')
COMB2(`RUN1', `(CTF__EA_FUN)', `(PRIMITIVE_TYPES, str)')
#if __STDC_VERSION__ >= 201112L
#define ctf_expect(a, cmp, b) \
  _Generic((b), \
           char       : ctf__ea_char,\
           int8_t     : ctf__ea_int,\
           int16_t    : ctf__ea_int,\
           int32_t    : ctf__ea_int,\
           int64_t    : ctf__ea_int,\
           uint8_t    : ctf__ea_uint,\
           uint16_t   : ctf__ea_uint,\
           uint32_t   : ctf__ea_uint,\
           uint64_t   : ctf__ea_uint,\
           float      : ctf__ea_float,\
           double     : ctf__ea_float,\
           long double: ctf__ea_float,\
           default    : ctf__ea_ptr)\
((a), #cmp, (b), #a, #b, 0, __LINE__, __FILE__)
#define ctf_assert(a, cmp, b) \
  _Generic((b), \
           char    : ctf__ea_char,\
           int8_t  : ctf__ea_int,\
           int16_t : ctf__ea_int,\
           int32_t : ctf__ea_int,\
           int64_t : ctf__ea_int,\
           uint8_t : ctf__ea_uint,\
           uint16_t: ctf__ea_uint,\
           uint32_t: ctf__ea_uint,\
           uint64_t: ctf__ea_uint,\
           float:    ctf__ea_float,\
           double:    ctf__ea_float,\
           long double:    ctf__ea_float,\
           default    : ctf__ea_ptr)\
((a), #cmp, (b), #a, #b, 1, __LINE__, __FILE__)
#define ctf_expect_mem(a, cmp, b, l) \
  ctf__ea_arr((a), #cmp, (b), l, l, #a, #b, 0,\
                  sizeof(*a),\
                  _Generic((*(b)),\
                           int8_t  : CTF__EA_SIGN_int,\
                           int16_t : CTF__EA_SIGN_int,\
                           int32_t : CTF__EA_SIGN_int,\
                           int64_t : CTF__EA_SIGN_int,\
                           float   : CTF__EA_SIGN_float,\
                           double   : CTF__EA_SIGN_float,\
                           long double   : CTF__EA_SIGN_float,\
                           default : CTF__EA_SIGN_uint), \
                           _Generic((*(b)),\
           char     : CTF__EA_MEM_TYPE_char,\
           int8_t   : CTF__EA_MEM_TYPE_int,\
           int16_t  : CTF__EA_MEM_TYPE_int,\
           int32_t  : CTF__EA_MEM_TYPE_int,\
           int64_t  : CTF__EA_MEM_TYPE_int,\
           uint8_t  : CTF__EA_MEM_TYPE_uint,\
           uint16_t : CTF__EA_MEM_TYPE_uint,\
           uint32_t : CTF__EA_MEM_TYPE_uint,\
           uint64_t : CTF__EA_MEM_TYPE_uint,\
           float: CTF__EA_MEM_TYPE_float,\
           double: CTF__EA_MEM_TYPE_float,\
           long double: CTF__EA_MEM_TYPE_float,\
           default    : CTF__EA_MEM_TYPE_ptr),\
__LINE__, __FILE__)
#define ctf_assert_mem(a, cmp, b, l) \
  ctf__ea_arr((a), #cmp, (b), l, l, #a, #b, 1,\
                  sizeof(*a),\
                  _Generic((*(b)),\
                           int8_t  : CTF__EA_SIGN_int,\
                           int16_t : CTF__EA_SIGN_int,\
                           int32_t : CTF__EA_SIGN_int,\
                           int64_t : CTF__EA_SIGN_int,\
                           float   : CTF__EA_SIGN_float,\
                           double   : CTF__EA_SIGN_float,\
                           long double   : CTF__EA_SIGN_float,\
                           default : CTF__EA_SIGN_uint), \
                           _Generic((*(b)),\
           char     : CTF__EA_MEM_TYPE_char,\
           int8_t   : CTF__EA_MEM_TYPE_int,\
           int16_t  : CTF__EA_MEM_TYPE_int,\
           int32_t  : CTF__EA_MEM_TYPE_int,\
           int64_t  : CTF__EA_MEM_TYPE_int,\
           uint8_t  : CTF__EA_MEM_TYPE_uint,\
           uint16_t : CTF__EA_MEM_TYPE_uint,\
           uint32_t : CTF__EA_MEM_TYPE_uint,\
           uint64_t : CTF__EA_MEM_TYPE_uint,\
           float: CTF__EA_MEM_TYPE_float,\
           double: CTF__EA_MEM_TYPE_float,\
           long double: CTF__EA_MEM_TYPE_float,\
           default    : CTF__EA_MEM_TYPE_ptr),\
__LINE__, __FILE__)
#define ctf_expect_arr(a, cmp, b) \
  ctf__ea_arr((a), #cmp, (b), sizeof(a)/sizeof(*(a)), sizeof(b)/sizeof(*(b)), #a, #b, 0,\
                  sizeof(*a),\
                  _Generic((*(b)),\
                           int8_t  : CTF__EA_SIGN_int,\
                           int16_t : CTF__EA_SIGN_int,\
                           int32_t : CTF__EA_SIGN_int,\
                           int64_t : CTF__EA_SIGN_int,\
                           float   : CTF__EA_SIGN_float,\
                           double   : CTF__EA_SIGN_float,\
                           long double   : CTF__EA_SIGN_float,\
                           default : CTF__EA_SIGN_uint), \
                           _Generic((*(b)),\
           char     : CTF__EA_MEM_TYPE_char,\
           int8_t   : CTF__EA_MEM_TYPE_int,\
           int16_t  : CTF__EA_MEM_TYPE_int,\
           int32_t  : CTF__EA_MEM_TYPE_int,\
           int64_t  : CTF__EA_MEM_TYPE_int,\
           uint8_t  : CTF__EA_MEM_TYPE_uint,\
           uint16_t : CTF__EA_MEM_TYPE_uint,\
           uint32_t : CTF__EA_MEM_TYPE_uint,\
           uint64_t : CTF__EA_MEM_TYPE_uint,\
           float: CTF__EA_MEM_TYPE_float,\
           double: CTF__EA_MEM_TYPE_float,\
           long double: CTF__EA_MEM_TYPE_float,\
           default    : CTF__EA_MEM_TYPE_ptr),\
__LINE__, __FILE__)
#define ctf_assert_arr(a, cmp, b) \
  ctf__ea_arr((a), #cmp, (b), sizeof(a)/sizeof(*(a)), sizeof(b)/sizeof(*(b)), #a, #b, 1,\
                  sizeof(*a),\
                  _Generic((*(b)),\
                           int8_t  : CTF__EA_SIGN_int,\
                           int16_t : CTF__EA_SIGN_int,\
                           int32_t : CTF__EA_SIGN_int,\
                           int64_t : CTF__EA_SIGN_int,\
                           float   : CTF__EA_SIGN_float,\
                           double   : CTF__EA_SIGN_float,\
                           long double   : CTF__EA_SIGN_float,\
                           default : CTF__EA_SIGN_uint), \
                           _Generic((*(b)),\
           char     : CTF__EA_MEM_TYPE_char,\
           int8_t   : CTF__EA_MEM_TYPE_int,\
           int16_t  : CTF__EA_MEM_TYPE_int,\
           int32_t  : CTF__EA_MEM_TYPE_int,\
           int64_t  : CTF__EA_MEM_TYPE_int,\
           uint8_t  : CTF__EA_MEM_TYPE_uint,\
           uint16_t : CTF__EA_MEM_TYPE_uint,\
           uint32_t : CTF__EA_MEM_TYPE_uint,\
           uint64_t : CTF__EA_MEM_TYPE_uint,\
           float: CTF__EA_MEM_TYPE_float,\
           double: CTF__EA_MEM_TYPE_float,\
           long double: CTF__EA_MEM_TYPE_float,\
           default    : CTF__EA_MEM_TYPE_ptr),\
__LINE__, __FILE__)
#endif
#if CTF_EA_ALIASES == CTF_ON
#if __STDC_VERSION__ >= 201112L
#define expect(a, cmp, b) \
  _Generic((b), \
           char       : ctf__ea_char,\
           int8_t     : ctf__ea_int,\
           int16_t    : ctf__ea_int,\
           int32_t    : ctf__ea_int,\
           int64_t    : ctf__ea_int,\
           uint8_t    : ctf__ea_uint,\
           uint16_t   : ctf__ea_uint,\
           uint32_t   : ctf__ea_uint,\
           uint64_t   : ctf__ea_uint,\
           float      : ctf__ea_float,\
           double     : ctf__ea_float,\
           long double: ctf__ea_float,\
           default    : ctf__ea_ptr)\
((a), #cmp, (b), #a, #b, 0, __LINE__, __FILE__)
#define assert(a, cmp, b) \
  _Generic((b), \
           char    : ctf__ea_char,\
           int8_t  : ctf__ea_int,\
           int16_t : ctf__ea_int,\
           int32_t : ctf__ea_int,\
           int64_t : ctf__ea_int,\
           uint8_t : ctf__ea_uint,\
           uint16_t: ctf__ea_uint,\
           uint32_t: ctf__ea_uint,\
           uint64_t: ctf__ea_uint,\
           float:    ctf__ea_float,\
           double:    ctf__ea_float,\
           long double:    ctf__ea_float,\
           default    : ctf__ea_ptr)\
((a), #cmp, (b), #a, #b, 1, __LINE__, __FILE__)
#define expect_mem(a, cmp, b, l) \
  ctf__ea_arr((a), #cmp, (b), l, l, #a, #b, 0,\
                  sizeof(*a),\
                  _Generic((*(b)),\
                           int8_t  : CTF__EA_SIGN_int,\
                           int16_t : CTF__EA_SIGN_int,\
                           int32_t : CTF__EA_SIGN_int,\
                           int64_t : CTF__EA_SIGN_int,\
                           float   : CTF__EA_SIGN_float,\
                           double   : CTF__EA_SIGN_float,\
                           long double   : CTF__EA_SIGN_float,\
                           default : CTF__EA_SIGN_uint), \
                           _Generic((*(b)),\
           char     : CTF__EA_MEM_TYPE_char,\
           int8_t   : CTF__EA_MEM_TYPE_int,\
           int16_t  : CTF__EA_MEM_TYPE_int,\
           int32_t  : CTF__EA_MEM_TYPE_int,\
           int64_t  : CTF__EA_MEM_TYPE_int,\
           uint8_t  : CTF__EA_MEM_TYPE_uint,\
           uint16_t : CTF__EA_MEM_TYPE_uint,\
           uint32_t : CTF__EA_MEM_TYPE_uint,\
           uint64_t : CTF__EA_MEM_TYPE_uint,\
           float: CTF__EA_MEM_TYPE_float,\
           double: CTF__EA_MEM_TYPE_float,\
           long double: CTF__EA_MEM_TYPE_float,\
           default    : CTF__EA_MEM_TYPE_ptr),\
__LINE__, __FILE__)
#define assert_mem(a, cmp, b, l) \
  ctf__ea_arr((a), #cmp, (b), l, l, #a, #b, 1,\
                  sizeof(*a),\
                  _Generic((*(b)),\
                           int8_t  : CTF__EA_SIGN_int,\
                           int16_t : CTF__EA_SIGN_int,\
                           int32_t : CTF__EA_SIGN_int,\
                           int64_t : CTF__EA_SIGN_int,\
                           float   : CTF__EA_SIGN_float,\
                           double   : CTF__EA_SIGN_float,\
                           long double   : CTF__EA_SIGN_float,\
                           default : CTF__EA_SIGN_uint), \
                           _Generic((*(b)),\
           char     : CTF__EA_MEM_TYPE_char,\
           int8_t   : CTF__EA_MEM_TYPE_int,\
           int16_t  : CTF__EA_MEM_TYPE_int,\
           int32_t  : CTF__EA_MEM_TYPE_int,\
           int64_t  : CTF__EA_MEM_TYPE_int,\
           uint8_t  : CTF__EA_MEM_TYPE_uint,\
           uint16_t : CTF__EA_MEM_TYPE_uint,\
           uint32_t : CTF__EA_MEM_TYPE_uint,\
           uint64_t : CTF__EA_MEM_TYPE_uint,\
           float: CTF__EA_MEM_TYPE_float,\
           double: CTF__EA_MEM_TYPE_float,\
           long double: CTF__EA_MEM_TYPE_float,\
           default    : CTF__EA_MEM_TYPE_ptr),\
__LINE__, __FILE__)
#define expect_arr(a, cmp, b) \
  ctf__ea_arr((a), #cmp, (b), sizeof(a)/sizeof(*(a)), sizeof(b)/sizeof(*(b)), #a, #b, 0,\
                  sizeof(*a),\
                  _Generic((*(b)),\
                           int8_t  : CTF__EA_SIGN_int,\
                           int16_t : CTF__EA_SIGN_int,\
                           int32_t : CTF__EA_SIGN_int,\
                           int64_t : CTF__EA_SIGN_int,\
                           float   : CTF__EA_SIGN_float,\
                           double   : CTF__EA_SIGN_float,\
                           long double   : CTF__EA_SIGN_float,\
                           default : CTF__EA_SIGN_uint), \
                           _Generic((*(b)),\
           char     : CTF__EA_MEM_TYPE_char,\
           int8_t   : CTF__EA_MEM_TYPE_int,\
           int16_t  : CTF__EA_MEM_TYPE_int,\
           int32_t  : CTF__EA_MEM_TYPE_int,\
           int64_t  : CTF__EA_MEM_TYPE_int,\
           uint8_t  : CTF__EA_MEM_TYPE_uint,\
           uint16_t : CTF__EA_MEM_TYPE_uint,\
           uint32_t : CTF__EA_MEM_TYPE_uint,\
           uint64_t : CTF__EA_MEM_TYPE_uint,\
           float: CTF__EA_MEM_TYPE_float,\
           double: CTF__EA_MEM_TYPE_float,\
           long double: CTF__EA_MEM_TYPE_float,\
           default    : CTF__EA_MEM_TYPE_ptr),\
__LINE__, __FILE__)
#define assert_arr(a, cmp, b) \
  ctf__ea_arr((a), #cmp, (b), sizeof(a)/sizeof(*(a)), sizeof(b)/sizeof(*(b)), #a, #b, 1,\
                  sizeof(*a),\
                  _Generic((*(b)),\
                           int8_t  : CTF__EA_SIGN_int,\
                           int16_t : CTF__EA_SIGN_int,\
                           int32_t : CTF__EA_SIGN_int,\
                           int64_t : CTF__EA_SIGN_int,\
                           float   : CTF__EA_SIGN_float,\
                           double   : CTF__EA_SIGN_float,\
                           long double   : CTF__EA_SIGN_float,\
                           default : CTF__EA_SIGN_uint), \
                           _Generic((*(b)),\
           char     : CTF__EA_MEM_TYPE_char,\
           int8_t   : CTF__EA_MEM_TYPE_int,\
           int16_t  : CTF__EA_MEM_TYPE_int,\
           int32_t  : CTF__EA_MEM_TYPE_int,\
           int64_t  : CTF__EA_MEM_TYPE_int,\
           uint8_t  : CTF__EA_MEM_TYPE_uint,\
           uint16_t : CTF__EA_MEM_TYPE_uint,\
           uint32_t : CTF__EA_MEM_TYPE_uint,\
           uint64_t : CTF__EA_MEM_TYPE_uint,\
           float: CTF__EA_MEM_TYPE_float,\
           double: CTF__EA_MEM_TYPE_float,\
           long double: CTF__EA_MEM_TYPE_float,\
           default    : CTF__EA_MEM_TYPE_ptr),\
__LINE__, __FILE__)
#endif
#define assert_true(a) \
  ctf__ea_int(0, "!=", a, "0", #a, 1, __LINE__, __FILE__)
#define assert_false(a) \
  ctf__ea_int(0, "==", a, "0", #a, 1, __LINE__, __FILE__)
#define expect_true(a) \
  ctf__ea_int(0, "!=", a, "0", #a, 0, __LINE__, __FILE__)
#define expect_false(a) \
  ctf__ea_int(0, "==", a, "0", #a, 0, __LINE__, __FILE__)
#define assert_non_null(a) \
  ctf__ea_ptr(NULL, "!=", a, "NULL", #a, 1, __LINE__, __FILE__)
#define assert_null(a) \
  ctf__ea_ptr(NULL, "==", a, "NULL", #a, 1, __LINE__, __FILE__)
#define expect_non_null(a) \
  ctf__ea_ptr(NULL, "!=", a, "NULL", #a, 0, __LINE__, __FILE__)
#define expect_null(a) \
  ctf__ea_ptr(NULL, "==", a, "NULL", #a, 0, __LINE__, __FILE__)
#define assert_msg(p, m, ...) \
  ctf__ea_msg(p, m, 1, __LINE__, __FILE__, ##__VA_ARGS__)
#define expect_msg(p, m, ...) \
  ctf__ea_msg(p, m, 0, __LINE__, __FILE__, ##__VA_ARGS__)

COMB(`ALIAS', `(assert_barrier(), assert_fold(count, msg))')

EA_FACTORY(`EA_TEMPLATE',     `(PRIMITIVE_TYPES, str)', `')
EA_FACTORY(`EA_MEM_TEMPLATE', `(PRIMITIVE_TYPES)', `')
EA_FACTORY(`EA_ARR_TEMPLATE', `(PRIMITIVE_TYPES)', `')
// clang-format on

#endif
