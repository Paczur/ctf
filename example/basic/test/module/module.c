#include "module.h"

#include <module/module.h>

CTF_TEST(add_negative) { ctf_assert_int_eq(-2, add(-1, -1)); }

CTF_GROUP(add_tests) = {
  add_negative,
};
