#define CTF_PARALLEL
#include <ctf/ctf.h>

CTF_TEST(add_signed) {
  int x = 1;
  int y = 2;
  int res = 3;
  assert_int_eq(res, x + y);
}

CTF_TEST(add_unsigned) {
  int x = -1;
  int y = 2;
  int res = 1;
  assert_int_eq(res, x + y);
}

CTF_TEST(add_failure) {
  int x = -1;
  int y = -2;
  int res = 1;
  expect_int_eq(res, x - y);
  assert_int_eq(res, x + y);
}

CTF_TEST(add_ptr) {
  char buff[3];
  char *buff_p = buff + 1;
  char *res = buff + 1;
  assert_ptr_eq(res, buff_p);
}

CTF_TEST(add_ptr_failure) {
  char buff[3];
  char *buff_p = buff + 1;
  char *res = buff;
  assert_ptr_eq(res, buff_p);
}

CTF_GROUP(add, add_signed, add_unsigned, add_failure, add_ptr, add_ptr_failure)

CTF_TEST(boolean_true) {
  int a = 24;
  assert_true(a);
}

CTF_TEST(boolean_true_failure) {
  int a = 0;
  assert_true(a);
}

CTF_TEST(boolean_false_failure) {
  int a = 23;
  assert_false(a);
}

CTF_GROUP(boolean, boolean_true, boolean_true_failure, boolean_false_failure)

CTF_TEST(string_equality) {
  char buff[25] = "eq";
  assert_string_eq("eq", buff);
}

CTF_TEST(string_failure) {
  char buff[25] = "eq";
  assert_string_eq("equa", buff);
}

CTF_GROUP(string, string_equality, string_failure)

CTF_TEST(array_equality) {
  int buff[] = {1, 2, 3, 4, 5, 6, 7};
  int buff2[] = {1, 2, 3, 4, 5, 6, 7};
  assert_array_int_eq(buff2, buff);
}

CTF_TEST(array_failure) {
  int buff[] = {1, 2, 3, 2, 5, -1, 7};
  int buff2[] = {1, 2, 3, 4, 5, 6, 7};
  ctf_assert_array_int_eq(buff2, buff);
}

CTF_GROUP(add_success, add_signed, add_unsigned, add_ptr)
CTF_GROUP(boolean_success, boolean_true)
CTF_GROUP(string_success, string_equality)
CTF_GROUP(array_success, array_equality)
CTF_GROUP(failure, add_failure, add_ptr_failure, boolean_true_failure,
          boolean_false_failure, string_failure, array_failure)

int main(void) {
  ctf_parallel_start();
  ctf_groups_run(&add_success, &boolean_success, &string_success,
                 &array_success);
  // ctf_barrier();
  // ctf_groups_run(&shttp_header_ops, &shttp_slice_ops);
  ctf_barrier();
  ctf_groups_run(&add, &failure);
  ctf_parallel_stop();
  return ctf_exit_code;
}
