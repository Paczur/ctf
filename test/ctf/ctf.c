#include "ctf.h"

#include "add.h"

MOCK(int, sub, (int a, int b), (a, b))
MOCK(int, add, (int a, int b), (a, b))

int stub_add(int a, int b) {
  (void)a;
  (void)b;
  return 0;
}
int mock_add(int a, int b) {
  const char s[] = "string";
  const int m[] = {0, 1, 2};
  ctf_mock_check(add, str, s);
  ctf_mock_check(add, int, a);
  ctf_mock_check(add, int, b);
  ctf_mock_check(add, ptr, m);
  return a + b;
}
int stub_sub(int a, int b) {
  (void)a;
  (void)b;
  return 1;
}

MOCK_GROUP(add_sub) = {
  MOCK_BIND(add, stub_add),
  MOCK_BIND(sub, stub_sub),
};

TEST(mock_basic) {
  mock(add, stub_add);
  expect_int_eq(0, add(1, 2));
  expect_int_eq(1, mock_call_count(add));
}
TEST(mock_grouped) {
  mock_group(add_sub);
  expect_int_eq(0, add(1, 2));
  expect_int_eq(1, mock_call_count(add));
  expect_int_eq(1, sub(1, 2));
  expect_int_eq(1, mock_call_count(sub));
}
TEST(mock_return) {
  mock(add, mock_add);
  mock_will_return(add, 2);
  expect_int_eq(2, add(1, 3));
  expect_int_eq(4, add(1, 3));
  mock_will_return_always(add, 2);
  expect_int_eq(2, add(1, 3));
  expect_int_eq(2, add(1, 3));
}
TEST(mock_expect) {
  const int m[] = {0, 1, 3};
  mock(add, mock_add);
  mock_expect_int_eq(add, a, 1);
  mock_expect_int_eq(add, b, 3);
  mock_expect_str_eq(add, s, "string");
  mock_expect_memory_int_eq(add, m, m);
  add(1, 3);
  add(2, 5);
}
TEST(mock_assert) {
  mock(add, mock_add);
  mock_assert_int_eq(add, a, 1);
  mock_assert_int_eq(add, b, 3);
  add(1, 3);
  add(2, 5);
}
TEST(mock_failure) {
  mock(add, mock_add);
  mock_expect_always_int_eq(add, a, 1);
  mock_expect_always_int_eq(add, b, 3);
  add(3, 4);
  add(2, 5);
}
TEST(mock_reset) {
  expect_int_eq(3, add(1, 2));
  expect_int_eq(0, mock_call_count(add));
}
GROUP(mock) = {
  P_TEST(mock_basic),  P_TEST(mock_grouped), P_TEST(mock_return),
  P_TEST(mock_expect), P_TEST(mock_assert),  P_TEST(mock_reset),
};

TEST(char_expect_success) {
  char a = 'a';
  char b = 'b';
  expect_char_eq(a, a);
  expect_char_neq(a, b);
  expect_char_lt(a, b);
  expect_char_gt(b, a);
  expect_char_lte(a, a);
  expect_char_lte(a, b);
  expect_char_gte(a, a);
  expect_char_gte(b, a);
}
TEST(char_expect_failure) {
  char a = 'a';
  char b = 'b';
  expect_char_eq(a, b);
  expect_char_neq(a, a);
  expect_char_lt(a, a);
  expect_char_lt(b, a);
  expect_char_gt(a, a);
  expect_char_gt(a, b);
  expect_char_lte(b, a);
  expect_char_gte(a, b);
}
TEST(char_assert) {
  char a = 'a';
  char b = 'b';
  assert_char_eq(a, a);
  assert_char_neq(a, b);
  assert_char_lt(a, b);
  assert_char_gt(b, a);
  assert_char_lte(a, a);
  assert_char_lte(a, b);
  assert_char_gte(a, a);
  assert_char_gte(b, a);
}
TEST(int_expect_success) {
  int a = -1;
  int b = 0;
  expect_int_eq(a, a);
  expect_int_neq(a, b);
  expect_int_lt(a, b);
  expect_int_gt(b, a);
  expect_int_lte(a, a);
  expect_int_lte(a, b);
  expect_int_gte(a, a);
  expect_int_gte(b, a);
}
TEST(int_expect_failure) {
  int a = -1;
  int b = 0;
  expect_int_eq(a, b);
  expect_int_neq(a, a);
  expect_int_lt(a, a);
  expect_int_lt(b, a);
  expect_int_gt(a, a);
  expect_int_gt(a, b);
  expect_int_lte(b, a);
  expect_int_gte(a, b);
}
TEST(int_assert) {
  int a = -1;
  int b = 0;
  assert_int_eq(a, a);
  assert_int_neq(a, b);
  assert_int_lt(a, b);
  assert_int_gt(b, a);
  assert_int_lte(a, a);
  assert_int_lte(a, b);
  assert_int_gte(a, a);
  assert_int_gte(b, a);
}
TEST(uint_expect_success) {
  unsigned a = 0;
  unsigned b = 1;
  expect_uint_eq(a, a);
  expect_uint_neq(a, b);
  expect_uint_lt(a, b);
  expect_uint_gt(b, a);
  expect_uint_lte(a, a);
  expect_uint_lte(a, b);
  expect_uint_gte(a, a);
  expect_uint_gte(b, a);
}
TEST(uint_expect_failure) {
  unsigned a = 0;
  unsigned b = 1;
  expect_uint_eq(a, b);
  expect_uint_neq(a, a);
  expect_uint_lt(a, a);
  expect_uint_lt(b, a);
  expect_uint_gt(a, a);
  expect_uint_gt(a, b);
  expect_uint_lte(b, a);
  expect_uint_gte(a, b);
}
TEST(uint_assert) {
  unsigned a = 0;
  unsigned b = 1;
  assert_uint_eq(a, a);
  assert_uint_neq(a, b);
  assert_uint_lt(a, b);
  assert_uint_gt(b, a);
  assert_uint_lte(a, a);
  assert_uint_lte(a, b);
  assert_uint_gte(a, a);
  assert_uint_gte(b, a);
}
TEST(ptr_expect_success) {
  int arr[2];
  expect_ptr_eq(arr, arr);
  expect_ptr_neq(arr, arr + 1);
  expect_ptr_lt(arr, arr + 1);
  expect_ptr_gt(arr + 1, arr);
  expect_ptr_lte(arr, arr);
  expect_ptr_lte(arr, arr + 1);
  expect_ptr_gte(arr, arr);
  expect_ptr_gte(arr + 1, arr);
}
TEST(ptr_expect_failure) {
  int arr[2];
  expect_ptr_eq(arr, arr + 1);
  expect_ptr_neq(arr, arr);
  expect_ptr_lt(arr, arr);
  expect_ptr_lt(arr + 1, arr);
  expect_ptr_gt(arr, arr);
  expect_ptr_gt(arr, arr + 1);
  expect_ptr_lte(arr + 1, arr);
  expect_ptr_gte(arr, arr + 1);
}
TEST(ptr_assert) {
  int arr[2];
  assert_ptr_eq(arr, arr);
  assert_ptr_neq(arr, arr + 1);
  assert_ptr_lt(arr, arr + 1);
  assert_ptr_gt(arr + 1, arr);
  assert_ptr_lte(arr, arr);
  assert_ptr_lte(arr, arr + 1);
  assert_ptr_gte(arr, arr);
  assert_ptr_gte(arr + 1, arr);
}
TEST(str_expect_success) {
  const char s1[] = "a";
  const char s2[] = "b";
  const char s3[] = "ab";
  const char s4[] = "ba";
  expect_str_eq(s1, s1);
  expect_str_neq(s1, s2);
  expect_str_neq(s1, s3);
  expect_str_neq(s2, s4);
  expect_str_lt(s1, s2);
  expect_str_lt(s1, s3);
  expect_str_lt(s1, s4);
  expect_str_gt(s2, s1);
  expect_str_gt(s3, s1);
  expect_str_gt(s4, s1);
  expect_str_lte(s1, s1);
  expect_str_lte(s3, s3);
  expect_str_lte(s1, s2);
  expect_str_lte(s1, s3);
  expect_str_lte(s1, s4);
  expect_str_gte(s1, s1);
  expect_str_gte(s3, s3);
  expect_str_gte(s2, s1);
  expect_str_gte(s3, s1);
  expect_str_gte(s4, s1);
}
TEST(str_expect_failure) {
  const char s1[] = "a";
  const char s2[] = "b";
  const char s3[] = "ab";
  const char s4[] = "ba";
  expect_str_eq(s1, s2);
  expect_str_eq(s1, s3);
  expect_str_eq(s2, s4);
  expect_str_neq(s1, s1);
  expect_str_lt(s1, s1);
  expect_str_lt(s3, s3);
  expect_str_lt(s2, s1);
  expect_str_lt(s3, s1);
  expect_str_lt(s4, s1);
  expect_str_gt(s1, s1);
  expect_str_gt(s3, s3);
  expect_str_gt(s1, s2);
  expect_str_gt(s1, s3);
  expect_str_gt(s1, s4);
  expect_str_lte(s2, s1);
  expect_str_lte(s3, s1);
  expect_str_lte(s4, s1);
  expect_str_gte(s1, s2);
  expect_str_gte(s1, s3);
  expect_str_gte(s1, s4);
}
TEST(str_assert) {
  const char s1[] = "a";
  const char s2[] = "b";
  const char s3[] = "ab";
  const char s4[] = "ba";
  assert_str_eq(s1, s1);
  assert_str_neq(s1, s2);
  assert_str_neq(s1, s3);
  assert_str_neq(s2, s4);
  assert_str_lt(s1, s2);
  assert_str_lt(s1, s3);
  assert_str_lt(s1, s4);
  assert_str_gt(s2, s1);
  assert_str_gt(s3, s1);
  assert_str_gt(s4, s1);
  assert_str_lte(s1, s1);
  assert_str_lte(s3, s3);
  assert_str_lte(s1, s2);
  assert_str_lte(s1, s3);
  assert_str_lte(s1, s4);
  assert_str_gte(s1, s1);
  assert_str_gte(s3, s3);
  assert_str_gte(s2, s1);
  assert_str_gte(s3, s1);
  assert_str_gte(s4, s1);
}
GROUP(primitive_success) = {
  P_TEST(char_expect_success), P_TEST(char_assert),
  P_TEST(int_expect_success),  P_TEST(int_assert),
  P_TEST(uint_expect_success), P_TEST(uint_assert),
  P_TEST(ptr_expect_success),  P_TEST(ptr_assert),
  P_TEST(str_expect_success),  P_TEST(str_assert),
};

TEST(array_char_expect_success) {
  const char a1[] = {'a'};
  const char a2[] = {'b'};
  const char a3[] = {'a', 'b'};
  const char a4[] = {'b', 'a'};
  expect_array_char_eq(a1, a1);
  expect_array_char_neq(a1, a2);
  expect_array_char_neq(a1, a3);
  expect_array_char_neq(a2, a4);
  expect_array_char_lt(a1, a2);
  expect_array_char_lt(a1, a3);
  expect_array_char_lt(a1, a4);
  expect_array_char_gt(a2, a1);
  expect_array_char_gt(a3, a1);
  expect_array_char_gt(a4, a1);
  expect_array_char_lte(a1, a1);
  expect_array_char_lte(a3, a3);
  expect_array_char_lte(a1, a2);
  expect_array_char_lte(a1, a3);
  expect_array_char_lte(a1, a4);
  expect_array_char_gte(a1, a1);
  expect_array_char_gte(a3, a3);
  expect_array_char_gte(a2, a1);
  expect_array_char_gte(a3, a1);
  expect_array_char_gte(a4, a1);
}
TEST(array_char_expect_failure) {
  const char a1[] = {'a'};
  const char a2[] = {'b'};
  const char a3[] = {'a', 'b'};
  const char a4[] = {'b', 'a'};
  expect_array_char_eq(a1, a2);
  expect_array_char_eq(a1, a3);
  expect_array_char_eq(a2, a4);
  expect_array_char_neq(a1, a1);
  expect_array_char_lt(a1, a1);
  expect_array_char_lt(a3, a3);
  expect_array_char_lt(a2, a1);
  expect_array_char_lt(a3, a1);
  expect_array_char_lt(a4, a1);
  expect_array_char_gt(a1, a1);
  expect_array_char_gt(a3, a3);
  expect_array_char_gt(a1, a2);
  expect_array_char_gt(a1, a3);
  expect_array_char_gt(a1, a4);
  expect_array_char_lte(a2, a1);
  expect_array_char_lte(a3, a1);
  expect_array_char_lte(a4, a1);
  expect_array_char_gte(a1, a2);
  expect_array_char_gte(a1, a3);
  expect_array_char_gte(a1, a4);
}
TEST(array_char_assert) {
  const char a1[] = {'a'};
  const char a2[] = {'b'};
  const char a3[] = {'a', 'b'};
  const char a4[] = {'b', 'a'};
  assert_array_char_eq(a1, a1);
  assert_array_char_neq(a1, a2);
  assert_array_char_neq(a1, a3);
  assert_array_char_neq(a2, a4);
  assert_array_char_lt(a1, a2);
  assert_array_char_lt(a1, a3);
  assert_array_char_lt(a1, a4);
  assert_array_char_gt(a2, a1);
  assert_array_char_gt(a3, a1);
  assert_array_char_gt(a4, a1);
  assert_array_char_lte(a1, a1);
  assert_array_char_lte(a3, a3);
  assert_array_char_lte(a1, a2);
  assert_array_char_lte(a1, a3);
  assert_array_char_lte(a1, a4);
  assert_array_char_gte(a1, a1);
  assert_array_char_gte(a3, a3);
  assert_array_char_gte(a2, a1);
  assert_array_char_gte(a3, a1);
  assert_array_char_gte(a4, a1);
}
TEST(array_int_expect_success) {
  const int a1[] = {-2};
  const int a2[] = {-1};
  const int a3[] = {-2, -1};
  const int a4[] = {-1, -2};
  expect_array_int_eq(a1, a1);
  expect_array_int_neq(a1, a2);
  expect_array_int_neq(a1, a3);
  expect_array_int_neq(a2, a4);
  expect_array_int_lt(a1, a2);
  expect_array_int_lt(a1, a3);
  expect_array_int_lt(a1, a4);
  expect_array_int_gt(a2, a1);
  expect_array_int_gt(a3, a1);
  expect_array_int_gt(a4, a1);
  expect_array_int_lte(a1, a1);
  expect_array_int_lte(a3, a3);
  expect_array_int_lte(a1, a2);
  expect_array_int_lte(a1, a3);
  expect_array_int_lte(a1, a4);
  expect_array_int_gte(a1, a1);
  expect_array_int_gte(a3, a3);
  expect_array_int_gte(a2, a1);
  expect_array_int_gte(a3, a1);
  expect_array_int_gte(a4, a1);
}
TEST(array_int_expect_failure) {
  const int a1[] = {-2};
  const int a2[] = {-1};
  const int a3[] = {-2, -1};
  const int a4[] = {-1, -2};
  expect_array_int_eq(a1, a2);
  expect_array_int_eq(a1, a3);
  expect_array_int_eq(a2, a4);
  expect_array_int_neq(a1, a1);
  expect_array_int_lt(a1, a1);
  expect_array_int_lt(a3, a3);
  expect_array_int_lt(a2, a1);
  expect_array_int_lt(a3, a1);
  expect_array_int_lt(a4, a1);
  expect_array_int_gt(a1, a1);
  expect_array_int_gt(a3, a3);
  expect_array_int_gt(a1, a2);
  expect_array_int_gt(a1, a3);
  expect_array_int_gt(a1, a4);
  expect_array_int_lte(a2, a1);
  expect_array_int_lte(a3, a1);
  expect_array_int_lte(a4, a1);
  expect_array_int_gte(a1, a2);
  expect_array_int_gte(a1, a3);
  expect_array_int_gte(a1, a4);
}
TEST(array_int_assert) {
  const int a1[] = {-2};
  const int a2[] = {-1};
  const int a3[] = {-2, -1};
  const int a4[] = {-1, -2};
  assert_array_int_eq(a1, a1);
  assert_array_int_neq(a1, a2);
  assert_array_int_neq(a1, a3);
  assert_array_int_neq(a2, a4);
  assert_array_int_lt(a1, a2);
  assert_array_int_lt(a1, a3);
  assert_array_int_lt(a1, a4);
  assert_array_int_gt(a2, a1);
  assert_array_int_gt(a3, a1);
  assert_array_int_gt(a4, a1);
  assert_array_int_lte(a1, a1);
  assert_array_int_lte(a3, a3);
  assert_array_int_lte(a1, a2);
  assert_array_int_lte(a1, a3);
  assert_array_int_lte(a1, a4);
  assert_array_int_gte(a1, a1);
  assert_array_int_gte(a3, a3);
  assert_array_int_gte(a2, a1);
  assert_array_int_gte(a3, a1);
  assert_array_int_gte(a4, a1);
}
TEST(array_uint_expect_success) {
  const unsigned a1[] = {1};
  const unsigned a2[] = {2};
  const unsigned a3[] = {1, 2};
  const unsigned a4[] = {2, 1};
  expect_array_uint_eq(a1, a1);
  expect_array_uint_neq(a1, a2);
  expect_array_uint_neq(a1, a3);
  expect_array_uint_neq(a2, a4);
  expect_array_uint_lt(a1, a2);
  expect_array_uint_lt(a1, a3);
  expect_array_uint_lt(a1, a4);
  expect_array_uint_gt(a2, a1);
  expect_array_uint_gt(a3, a1);
  expect_array_uint_gt(a4, a1);
  expect_array_uint_lte(a1, a1);
  expect_array_uint_lte(a3, a3);
  expect_array_uint_lte(a1, a2);
  expect_array_uint_lte(a1, a3);
  expect_array_uint_lte(a1, a4);
  expect_array_uint_gte(a1, a1);
  expect_array_uint_gte(a3, a3);
  expect_array_uint_gte(a2, a1);
  expect_array_uint_gte(a3, a1);
  expect_array_uint_gte(a4, a1);
}
TEST(array_uint_expect_failure) {
  const unsigned a1[] = {1};
  const unsigned a2[] = {2};
  const unsigned a3[] = {1, 2};
  const unsigned a4[] = {2, 1};
  expect_array_uint_eq(a1, a2);
  expect_array_uint_eq(a1, a3);
  expect_array_uint_eq(a2, a4);
  expect_array_uint_neq(a1, a1);
  expect_array_uint_lt(a1, a1);
  expect_array_uint_lt(a3, a3);
  expect_array_uint_lt(a2, a1);
  expect_array_uint_lt(a3, a1);
  expect_array_uint_lt(a4, a1);
  expect_array_uint_gt(a1, a1);
  expect_array_uint_gt(a3, a3);
  expect_array_uint_gt(a1, a2);
  expect_array_uint_gt(a1, a3);
  expect_array_uint_gt(a1, a4);
  expect_array_uint_lte(a2, a1);
  expect_array_uint_lte(a3, a1);
  expect_array_uint_lte(a4, a1);
  expect_array_uint_gte(a1, a2);
  expect_array_uint_gte(a1, a3);
  expect_array_uint_gte(a1, a4);
}
TEST(array_uint_assert) {
  const unsigned a1[] = {1};
  const unsigned a2[] = {2};
  const unsigned a3[] = {1, 2};
  const unsigned a4[] = {2, 1};
  assert_array_uint_eq(a1, a1);
  assert_array_uint_neq(a1, a2);
  assert_array_uint_neq(a1, a3);
  assert_array_uint_neq(a2, a4);
  assert_array_uint_lt(a1, a2);
  assert_array_uint_lt(a1, a3);
  assert_array_uint_lt(a1, a4);
  assert_array_uint_gt(a2, a1);
  assert_array_uint_gt(a3, a1);
  assert_array_uint_gt(a4, a1);
  assert_array_uint_lte(a1, a1);
  assert_array_uint_lte(a3, a3);
  assert_array_uint_lte(a1, a2);
  assert_array_uint_lte(a1, a3);
  assert_array_uint_lte(a1, a4);
  assert_array_uint_gte(a1, a1);
  assert_array_uint_gte(a3, a3);
  assert_array_uint_gte(a2, a1);
  assert_array_uint_gte(a3, a1);
  assert_array_uint_gte(a4, a1);
}
TEST(array_ptr_expect_success) {
  int arr[2];
  const int *const a1[] = {arr};
  const int *const a2[] = {arr + 1};
  const int *const a3[] = {arr, arr + 1};
  const int *const a4[] = {arr + 1, arr};
  expect_array_ptr_eq(a1, a1);
  expect_array_ptr_neq(a1, a2);
  expect_array_ptr_neq(a1, a3);
  expect_array_ptr_neq(a2, a4);
  expect_array_ptr_lt(a1, a2);
  expect_array_ptr_lt(a1, a3);
  expect_array_ptr_lt(a1, a4);
  expect_array_ptr_gt(a2, a1);
  expect_array_ptr_gt(a3, a1);
  expect_array_ptr_gt(a4, a1);
  expect_array_ptr_lte(a1, a1);
  expect_array_ptr_lte(a3, a3);
  expect_array_ptr_lte(a1, a2);
  expect_array_ptr_lte(a1, a3);
  expect_array_ptr_lte(a1, a4);
  expect_array_ptr_gte(a1, a1);
  expect_array_ptr_gte(a3, a3);
  expect_array_ptr_gte(a2, a1);
  expect_array_ptr_gte(a3, a1);
  expect_array_ptr_gte(a4, a1);
}
TEST(array_ptr_expect_failure) {
  int arr[2];
  const int *const a1[] = {arr};
  const int *const a2[] = {arr + 1};
  const int *const a3[] = {arr, arr + 1};
  const int *const a4[] = {arr + 1, arr};
  expect_array_ptr_eq(a1, a2);
  expect_array_ptr_eq(a1, a3);
  expect_array_ptr_eq(a2, a4);
  expect_array_ptr_neq(a1, a1);
  expect_array_ptr_lt(a1, a1);
  expect_array_ptr_lt(a3, a3);
  expect_array_ptr_lt(a2, a1);
  expect_array_ptr_lt(a3, a1);
  expect_array_ptr_lt(a4, a1);
  expect_array_ptr_gt(a1, a1);
  expect_array_ptr_gt(a3, a3);
  expect_array_ptr_gt(a1, a2);
  expect_array_ptr_gt(a1, a3);
  expect_array_ptr_gt(a1, a4);
  expect_array_ptr_lte(a2, a1);
  expect_array_ptr_lte(a3, a1);
  expect_array_ptr_lte(a4, a1);
  expect_array_ptr_gte(a1, a2);
  expect_array_ptr_gte(a1, a3);
  expect_array_ptr_gte(a1, a4);
}
TEST(array_ptr_assert) {
  int arr[2];
  const int *const a1[] = {arr};
  const int *const a2[] = {arr + 1};
  const int *const a3[] = {arr, arr + 1};
  const int *const a4[] = {arr + 1, arr};
  assert_array_ptr_eq(a1, a1);
  assert_array_ptr_neq(a1, a2);
  assert_array_ptr_neq(a1, a3);
  assert_array_ptr_neq(a2, a4);
  assert_array_ptr_lt(a1, a2);
  assert_array_ptr_lt(a1, a3);
  assert_array_ptr_lt(a1, a4);
  assert_array_ptr_gt(a2, a1);
  assert_array_ptr_gt(a3, a1);
  assert_array_ptr_gt(a4, a1);
  assert_array_ptr_lte(a1, a1);
  assert_array_ptr_lte(a3, a3);
  assert_array_ptr_lte(a1, a2);
  assert_array_ptr_lte(a1, a3);
  assert_array_ptr_lte(a1, a4);
  assert_array_ptr_gte(a1, a1);
  assert_array_ptr_gte(a3, a3);
  assert_array_ptr_gte(a2, a1);
  assert_array_ptr_gte(a3, a1);
  assert_array_ptr_gte(a4, a1);
}
GROUP(array_success) = {
  P_TEST(array_char_expect_success), P_TEST(array_char_assert),
  P_TEST(array_int_expect_success),  P_TEST(array_int_assert),
  P_TEST(array_uint_expect_success), P_TEST(array_uint_assert),
  P_TEST(array_ptr_expect_success),  P_TEST(array_ptr_assert),
};

TEST(memory_char_expect_success) {
  const char a1[] = {'a'};
  const char a2[] = {'b'};
  expect_memory_char_eq(a1, a1, 1);
  expect_memory_char_lt(a1, a2, 1);
  expect_memory_char_gt(a2, a1, 1);
  expect_memory_char_lte(a1, a1, 1);
  expect_memory_char_lte(a1, a2, 1);
  expect_memory_char_gte(a1, a1, 1);
  expect_memory_char_gte(a2, a1, 1);
}
TEST(memory_char_expect_failure) {
  const char a1[] = {'a'};
  const char a2[] = {'b'};
  expect_memory_char_eq(a1, a2, 1);
  expect_memory_char_neq(a1, a1, 1);
  expect_memory_char_lt(a1, a1, 1);
  expect_memory_char_lt(a2, a1, 1);
  expect_memory_char_gt(a1, a1, 1);
  expect_memory_char_gt(a1, a2, 1);
  expect_memory_char_lte(a2, a1, 1);
  expect_memory_char_gte(a1, a2, 1);
}
TEST(memory_char_assert) {
  const char a1[] = {'a'};
  const char a2[] = {'b'};
  assert_memory_char_eq(a1, a1, 1);
  assert_memory_char_neq(a1, a2, 1);
  assert_memory_char_lt(a1, a2, 1);
  assert_memory_char_gt(a2, a1, 1);
  assert_memory_char_lte(a1, a1, 1);
  assert_memory_char_lte(a1, a2, 1);
  assert_memory_char_gte(a1, a1, 1);
  assert_memory_char_gte(a2, a1, 1);
}
TEST(memory_int_expect_success) {
  const int a1[] = {-2};
  const int a2[] = {-1};
  expect_memory_int_eq(a1, a1, 1);
  expect_memory_int_lt(a1, a2, 1);
  expect_memory_int_gt(a2, a1, 1);
  expect_memory_int_lte(a1, a1, 1);
  expect_memory_int_lte(a1, a2, 1);
  expect_memory_int_gte(a1, a1, 1);
  expect_memory_int_gte(a2, a1, 1);
}
TEST(memory_int_expect_failure) {
  const int a1[] = {-2};
  const int a2[] = {-1};
  expect_memory_int_eq(a1, a2, 1);
  expect_memory_int_neq(a1, a1, 1);
  expect_memory_int_lt(a1, a1, 1);
  expect_memory_int_lt(a2, a1, 1);
  expect_memory_int_gt(a1, a1, 1);
  expect_memory_int_gt(a1, a2, 1);
  expect_memory_int_lte(a2, a1, 1);
  expect_memory_int_gte(a1, a2, 1);
}
TEST(memory_int_assert) {
  const int a1[] = {-2};
  const int a2[] = {-1};
  assert_memory_int_eq(a1, a1, 1);
  assert_memory_int_neq(a1, a2, 1);
  assert_memory_int_lt(a1, a2, 1);
  assert_memory_int_gt(a2, a1, 1);
  assert_memory_int_lte(a1, a1, 1);
  assert_memory_int_lte(a1, a2, 1);
  assert_memory_int_gte(a1, a1, 1);
  assert_memory_int_gte(a2, a1, 1);
}
TEST(memory_uint_expect_success) {
  const unsigned a1[] = {1};
  const unsigned a2[] = {2};
  expect_memory_uint_eq(a1, a1, 1);
  expect_memory_uint_neq(a1, a2, 1);
  expect_memory_uint_lt(a1, a2, 1);
  expect_memory_uint_gt(a2, a1, 1);
  expect_memory_uint_lte(a1, a1, 1);
  expect_memory_uint_lte(a1, a2, 1);
  expect_memory_uint_gte(a1, a1, 1);
  expect_memory_uint_gte(a2, a1, 1);
}
TEST(memory_uint_expect_failure) {
  const unsigned a1[] = {1};
  const unsigned a2[] = {2};
  expect_memory_uint_eq(a1, a2, 1);
  expect_memory_uint_neq(a1, a1, 1);
  expect_memory_uint_lt(a1, a1, 1);
  expect_memory_uint_lt(a2, a1, 1);
  expect_memory_uint_gt(a1, a1, 1);
  expect_memory_uint_gt(a1, a2, 1);
  expect_memory_uint_lte(a2, a1, 1);
  expect_memory_uint_gte(a1, a2, 1);
}
TEST(memory_uint_assert) {
  const unsigned a1[] = {1};
  const unsigned a2[] = {2};
  assert_memory_uint_eq(a1, a1, 1);
  assert_memory_uint_neq(a1, a2, 1);
  assert_memory_uint_lt(a1, a2, 1);
  assert_memory_uint_gt(a2, a1, 1);
  assert_memory_uint_lte(a1, a1, 1);
  assert_memory_uint_lte(a1, a2, 1);
  assert_memory_uint_gte(a1, a1, 1);
  assert_memory_uint_gte(a2, a1, 1);
}
TEST(memory_ptr_expect_success) {
  int arr[2];
  const int *const a1[] = {arr};
  const int *const a2[] = {arr + 1};
  expect_memory_ptr_eq(a1, a1, 1);
  expect_memory_ptr_neq(a1, a2, 1);
  expect_memory_ptr_lt(a1, a2, 1);
  expect_memory_ptr_gt(a2, a1, 1);
  expect_memory_ptr_lte(a1, a1, 1);
  expect_memory_ptr_lte(a1, a2, 1);
  expect_memory_ptr_gte(a1, a1, 1);
  expect_memory_ptr_gte(a2, a1, 1);
}
TEST(memory_ptr_expect_failure) {
  int arr[2];
  const int *const a1[] = {arr};
  const int *const a2[] = {arr + 1};
  expect_memory_ptr_eq(a1, a2, 1);
  expect_memory_ptr_neq(a1, a1, 1);
  expect_memory_ptr_lt(a1, a1, 1);
  expect_memory_ptr_lt(a2, a1, 1);
  expect_memory_ptr_gt(a1, a1, 1);
  expect_memory_ptr_gt(a1, a2, 1);
  expect_memory_ptr_lte(a2, a1, 1);
  expect_memory_ptr_gte(a1, a2, 1);
}
TEST(memory_ptr_assert) {
  int arr[2];
  const int *const a1[] = {arr};
  const int *const a2[] = {arr + 1};
  assert_memory_ptr_eq(a1, a1, 1);
  assert_memory_ptr_neq(a1, a2, 1);
  assert_memory_ptr_lt(a1, a2, 1);
  assert_memory_ptr_gt(a2, a1, 1);
  assert_memory_ptr_lte(a1, a1, 1);
  assert_memory_ptr_lte(a1, a2, 1);
  assert_memory_ptr_gte(a1, a1, 1);
  assert_memory_ptr_gte(a2, a1, 1);
}
GROUP(memory_success) = {
  P_TEST(memory_char_expect_success), P_TEST(memory_char_assert),
  P_TEST(memory_int_expect_success),  P_TEST(memory_int_assert),
  P_TEST(memory_uint_expect_success), P_TEST(memory_uint_assert),
  P_TEST(memory_ptr_expect_success),  P_TEST(memory_ptr_assert),
};

TEST(pass_and_fail) {
  ctf_pass("pass");
  ctf_fail("fail");
}

GROUP(failure) = {
  P_TEST(char_expect_failure),
  P_TEST(int_expect_failure),
  P_TEST(uint_expect_failure),
  P_TEST(ptr_expect_failure),
  P_TEST(str_expect_failure),
  P_TEST(array_char_expect_failure),
  P_TEST(array_int_expect_failure),
  P_TEST(array_uint_expect_failure),
  P_TEST(array_ptr_expect_failure),
  P_TEST(memory_char_expect_failure),
  P_TEST(memory_int_expect_failure),
  P_TEST(memory_uint_expect_failure),
  P_TEST(memory_ptr_expect_failure),
  P_TEST(pass_and_fail),
  P_TEST(mock_failure),
};

