#include <ctf/ctf.h>

#include "add.h"

CTF_MOCK_BEGIN(int, add, int a, int b)
CTF_MOCK_CALL_ARGS(a, b)
CTF_MOCK_END

CTF_MOCK_BEGIN(int, sub, int a, int b)
CTF_MOCK_CALL_ARGS(a, b)
CTF_MOCK_END

int mock_add(int a, int b) { return 0; }
int mock_sub(int a, int b) { return 1; }

CTF_MOCK_GROUP(add_sub) = {
  CTF_MOCK_GROUP_BIND(add, mock_add),
  CTF_MOCK_GROUP_BIND(sub, mock_sub),
};

CTF_TEST(mock_basic) {
  ctf_mock(add, mock_add);
  expect_int_eq(0, add(1, 2));
  expect_int_eq(1, ctf_mock_call_count(add));
}
CTF_TEST(mock_reset) {
  expect_int_eq(3, add(1, 2));
  expect_int_eq(0, ctf_mock_call_count(add));
}
CTF_TEST(mock_group) {
  ctf_mock_group(add_sub);
  expect_int_eq(0, add(1, 2));
  expect_int_eq(1, ctf_mock_call_count(add));
  expect_int_eq(1, sub(1, 2));
  expect_int_eq(1, ctf_mock_call_count(sub));
}
CTF_GROUP(mock, mock_basic, mock_reset, mock_group)

CTF_TEST(char_expect_success) {
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
CTF_TEST(char_expect_failure) {
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
CTF_TEST(char_assert) {
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
CTF_TEST(int_expect_success) {
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
CTF_TEST(int_expect_failure) {
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
CTF_TEST(int_assert) {
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
CTF_TEST(uint_expect_success) {
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
CTF_TEST(uint_expect_failure) {
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
CTF_TEST(uint_assert) {
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
CTF_TEST(ptr_expect_success) {
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
CTF_TEST(ptr_expect_failure) {
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
CTF_TEST(ptr_assert) {
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
CTF_TEST(string_expect_success) {
  const char s1[] = "a";
  const char s2[] = "b";
  const char s3[] = "ab";
  const char s4[] = "ba";
  expect_string_eq(s1, s1);
  expect_string_neq(s1, s2);
  expect_string_neq(s1, s3);
  expect_string_neq(s2, s4);
  expect_string_lt(s1, s2);
  expect_string_lt(s1, s3);
  expect_string_lt(s1, s4);
  expect_string_gt(s2, s1);
  expect_string_gt(s3, s1);
  expect_string_gt(s4, s1);
  expect_string_lte(s1, s1);
  expect_string_lte(s3, s3);
  expect_string_lte(s1, s2);
  expect_string_lte(s1, s3);
  expect_string_lte(s1, s4);
  expect_string_gte(s1, s1);
  expect_string_gte(s3, s3);
  expect_string_gte(s2, s1);
  expect_string_gte(s3, s1);
  expect_string_gte(s4, s1);
}
CTF_TEST(string_expect_failure) {
  const char s1[] = "a";
  const char s2[] = "b";
  const char s3[] = "ab";
  const char s4[] = "ba";
  expect_string_eq(s1, s2);
  expect_string_eq(s1, s3);
  expect_string_eq(s2, s4);
  expect_string_neq(s1, s1);
  expect_string_lt(s1, s1);
  expect_string_lt(s3, s3);
  expect_string_lt(s2, s1);
  expect_string_lt(s3, s1);
  expect_string_lt(s4, s1);
  expect_string_gt(s1, s1);
  expect_string_gt(s3, s3);
  expect_string_gt(s1, s2);
  expect_string_gt(s1, s3);
  expect_string_gt(s1, s4);
  expect_string_lte(s2, s1);
  expect_string_lte(s3, s1);
  expect_string_lte(s4, s1);
  expect_string_gte(s1, s2);
  expect_string_gte(s1, s3);
  expect_string_gte(s1, s4);
}
CTF_TEST(string_assert) {
  const char s1[] = "a";
  const char s2[] = "b";
  const char s3[] = "ab";
  const char s4[] = "ba";
  assert_string_eq(s1, s1);
  assert_string_neq(s1, s2);
  assert_string_neq(s1, s3);
  assert_string_neq(s2, s4);
  assert_string_lt(s1, s2);
  assert_string_lt(s1, s3);
  assert_string_lt(s1, s4);
  assert_string_gt(s2, s1);
  assert_string_gt(s3, s1);
  assert_string_gt(s4, s1);
  assert_string_lte(s1, s1);
  assert_string_lte(s3, s3);
  assert_string_lte(s1, s2);
  assert_string_lte(s1, s3);
  assert_string_lte(s1, s4);
  assert_string_gte(s1, s1);
  assert_string_gte(s3, s3);
  assert_string_gte(s2, s1);
  assert_string_gte(s3, s1);
  assert_string_gte(s4, s1);
}
CTF_GROUP(primitive_success, char_expect_success, char_assert,
          int_expect_success, int_assert, uint_expect_success, uint_assert,
          ptr_expect_success, ptr_assert, string_expect_success, string_assert)

CTF_TEST(array_char_expect_success) {
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
CTF_TEST(array_char_expect_failure) {
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
CTF_TEST(array_char_assert) {
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
CTF_TEST(array_int_expect_success) {
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
CTF_TEST(array_int_expect_failure) {
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
CTF_TEST(array_int_assert) {
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
CTF_TEST(array_uint_expect_success) {
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
CTF_TEST(array_uint_expect_failure) {
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
CTF_TEST(array_uint_assert) {
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
CTF_TEST(array_ptr_expect_success) {
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
CTF_TEST(array_ptr_expect_failure) {
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
CTF_TEST(array_ptr_assert) {
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
CTF_GROUP(array_success, array_char_expect_success, array_char_assert,
          array_int_expect_success, array_int_assert, array_uint_expect_success,
          array_uint_assert, array_ptr_expect_success, array_ptr_assert)

CTF_TEST(memory_char_expect_success) {
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
CTF_TEST(memory_char_expect_failure) {
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
CTF_TEST(memory_char_assert) {
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
CTF_TEST(memory_int_expect_success) {
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
CTF_TEST(memory_int_expect_failure) {
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
CTF_TEST(memory_int_assert) {
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
CTF_TEST(memory_uint_expect_success) {
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
CTF_TEST(memory_uint_expect_failure) {
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
CTF_TEST(memory_uint_assert) {
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
CTF_TEST(memory_ptr_expect_success) {
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
CTF_TEST(memory_ptr_expect_failure) {
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
CTF_TEST(memory_ptr_assert) {
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
CTF_GROUP(memory_success, memory_char_expect_success, memory_char_assert,
          memory_int_expect_success, memory_int_assert,
          memory_uint_expect_success, memory_uint_assert,
          memory_ptr_expect_success, memory_ptr_assert)

CTF_TEST(pass_and_fail) {
  ctf_pass("pass");
  ctf_fail("fail");
}

CTF_GROUP(failure, char_expect_failure, int_expect_failure, uint_expect_failure,
          ptr_expect_failure, string_expect_failure, array_char_expect_failure,
          array_int_expect_failure, array_uint_expect_failure,
          array_ptr_expect_failure, memory_char_expect_failure,
          memory_int_expect_failure, memory_uint_expect_failure,
          memory_ptr_expect_failure, pass_and_fail)

