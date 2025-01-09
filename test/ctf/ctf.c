#include "ctf.h"

#include "add.h"

CTF_MOCK(int, sub, (int a, int b), (a, b)) {
  if(mock_in) {
    mock_check(a);
    mock_check(b);
  } else {
    mock_check(mock_return_value);
  }
}
CTF_MOCK(int, add, (int a, int b), (a, b)) {
  if(mock_in) {
    mock_check(a);
    mock_check(b);
  } else {
    mock_check(mock_return_value);
  }
}
CTF_MOCK_CUSTOM(strcmp, int, wrapped_strcmp, (const char *a, const char *b),
                (a, b)) {
  mock_check_str(a);
  mock_check_str(b);
  if(mock_out) mock_check(mock_return_value);
}
CTF_MOCK_CUSTOM(memcmp, int, wrapped_memcmp,
                (const void *a, const void *b, size_t l), (a, b, l)) {
  mock_check_mem_int(a);
  mock_check_mem_int(b);
  if(mock_in) {
    mock_check(l);
  } else {
    mock_check(mock_return_value);
  }
}

// tests void returns and arguments
void empty_f(void) {};
CTF_MOCK_VOID_CUSTOM(empty_f, empty_wrapper) {}

int mock_add(int a, int b) {
  (void)(a - b);
  return 0;
}
int mock_sub(int a, int b) {
  (void)(a + b);
  return 1;
}
CTF_MOCK_GROUP(add_sub) = {
  CTF_MOCK_BIND(add, mock_add),
  CTF_MOCK_BIND(sub, mock_sub),
};

CTF_TEST(mock_grouped) {
  mock_group(add_sub);
  subtest(first_mock_in_group) mock_select(add) {
    mock_expect(a, ==, 1);
    mock_expect(b, ==, 2);
    expect(0, ==, add(1, 2));
    expect(1, ==, mock_call_count);
  }
  subtest(second_mock_in_group) mock_select(sub) {
    mock_expect(a, ==, 1);
    mock_expect(b, ==, 2);
    expect(1, ==, sub(1, 2));
    expect(1, ==, mock_call_count);
  }
}
CTF_TEST(mock_multiple) {
  mock_spy(add) {
    mock_expect(a, ==, 1);
    mock_expect(b, ==, 2);
    mock_spy(sub) {
      mock_expect(a, ==, 2);
      mock_expect(b, ==, 3);
      expect(-1, ==, sub(2, 3));
      expect(1, ==, mock_call_count);
    }
    expect(3, ==, add(1, 2));
    expect(1, ==, mock_call_count);
  }
}
CTF_TEST(mock_return) {
  mock(add, NULL) {
    mock_return(2);
    mock_return_nth(2, 3);
    expect(2, ==, add(1, 3));
    expect(3, ==, add(1, 3));
  }
}

CTF_TEST(mock_char_expect_success) {
  char a = 'a';
  char b = 'b';
  mock_spy(add) {
    mock_expect(b, >=, a);
    mock_expect(a, >=, a);
    mock_expect(a, <=, b);
    mock_expect_nth(1, a, ==, a);
    mock_expect_nth(1, b, ==, b);
    mock_expect_nth(1, a, !=, b);
    mock_expect_nth(1, b, !=, a);
    mock_expect_nth(1, b, >, a);
    mock_expect_nth(1, a, <, b);
    mock_expect_nth(1, a, <=, a);
    mock_expect_nth(2, b, ==, a);
    mock_expect_nth(2, a, ==, b);
    mock_expect_nth(2, a, !=, a);
    mock_expect_nth(2, b, !=, b);
    mock_expect_nth(2, a, >, a);
    mock_expect_nth(2, b, <, b);
    mock_expect_nth(2, a, >=, b);
    mock_expect_nth(2, a, >=, a);
    mock_expect_nth(2, b, <=, a);
    mock_expect_nth(2, b, <=, b);
    add(a, b);
    add(b, a);
  }
}
CTF_TEST(mock_char_expect_failure) {
  char a = 'a';
  char b = 'b';
  mock_spy(add) {
    mock_expect(a, ==, b);
    mock_expect(b, ==, a);
    mock_expect(a, !=, a);
    mock_expect(b, !=, b);
    mock_expect(a, >, a);
    mock_expect(b, <, a);
    mock_expect(a, <, a);
    mock_expect(a, >, b);
    mock_expect(b, <=, a);
    mock_expect(a, >=, b);
    add(a, b);
  }
}
CTF_TEST(mock_char_assert) {
  char a = 'a';
  char b = 'b';
  mock_spy(add) {
    mock_assert_nth(1, a, ==, a);
    mock_assert_nth(1, b, ==, b);
    mock_assert_nth(1, a, !=, b);
    mock_assert_nth(1, b, !=, a);
    mock_assert_nth(1, b, >, a);
    mock_assert_nth(1, a, <, b);
    mock_assert_nth(1, a, <=, a);
    mock_assert_nth(1, b, >=, a);
    mock_assert_nth(1, a, >=, a);
    mock_assert_nth(1, a, <=, b);
    mock_assert_nth(2, b, ==, a);
    mock_assert_nth(2, a, ==, b);
    mock_assert_nth(2, a, !=, a);
    mock_assert_nth(2, b, !=, b);
    mock_assert_nth(2, a, >, a);
    mock_assert_nth(2, b, <, b);
    mock_assert_nth(2, a, >=, b);
    mock_assert_nth(2, a, >=, a);
    mock_assert_nth(2, b, <=, a);
    mock_assert_nth(2, b, <=, b);
    add(a, b);
    add(b, a);
  }
}
CTF_TEST(mock_int_expect_success) {
  int a = -2;
  int b = -1;
  mock_spy(add) {
    mock_expect(b, >=, a);
    mock_expect(a, >=, a);
    mock_expect(a, <=, b);
    mock_expect_nth(1, a, ==, a);
    mock_expect_nth(1, b, ==, b);
    mock_expect_nth(1, a, !=, b);
    mock_expect_nth(1, b, !=, a);
    mock_expect_nth(1, b, >, a);
    mock_expect_nth(1, a, <, b);
    mock_expect_nth(1, a, <=, a);
    mock_expect_nth(2, b, ==, a);
    mock_expect_nth(2, a, ==, b);
    mock_expect_nth(2, a, !=, a);
    mock_expect_nth(2, b, !=, b);
    mock_expect_nth(2, a, >, a);
    mock_expect_nth(2, b, <, b);
    mock_expect_nth(2, a, >=, b);
    mock_expect_nth(2, a, >=, a);
    mock_expect_nth(2, b, <=, a);
    mock_expect_nth(2, b, <=, b);
    add(a, b);
    add(b, a);
  }
}
CTF_TEST(mock_int_expect_failure) {
  int a = -2;
  int b = -1;
  mock_spy(add) {
    mock_expect(a, ==, b);
    mock_expect(b, ==, a);
    mock_expect(a, !=, a);
    mock_expect(b, !=, b);
    mock_expect(a, >, a);
    mock_expect(b, <, a);
    mock_expect(a, <, a);
    mock_expect(a, >, b);
    mock_expect(b, <=, a);
    mock_expect(a, >=, b);
    add(a, b);
  }
}
CTF_TEST(mock_int_assert) {
  int a = -2;
  int b = -1;
  mock_spy(add) {
    mock_assert_nth(1, a, ==, a);
    mock_assert_nth(1, b, ==, b);
    mock_assert_nth(1, a, !=, b);
    mock_assert_nth(1, b, !=, a);
    mock_assert_nth(1, b, >, a);
    mock_assert_nth(1, a, <, b);
    mock_assert_nth(1, a, <=, a);
    mock_assert_nth(1, b, >=, a);
    mock_assert_nth(1, a, >=, a);
    mock_assert_nth(1, a, <=, b);
    mock_assert_nth(2, b, ==, a);
    mock_assert_nth(2, a, ==, b);
    mock_assert_nth(2, a, !=, a);
    mock_assert_nth(2, b, !=, b);
    mock_assert_nth(2, a, >, a);
    mock_assert_nth(2, b, <, b);
    mock_assert_nth(2, a, >=, b);
    mock_assert_nth(2, a, >=, a);
    mock_assert_nth(2, b, <=, a);
    mock_assert_nth(2, b, <=, b);
    add(a, b);
    add(b, a);
  }
}
CTF_TEST(mock_uint_expect_success) {
  unsigned a = 0;
  unsigned b = 1;
  mock_spy(add) {
    mock_expect(b, >=, a);
    mock_expect(a, >=, a);
    mock_expect(a, <=, b);
    mock_expect_nth(1, a, ==, a);
    mock_expect_nth(1, b, ==, b);
    mock_expect_nth(1, a, !=, b);
    mock_expect_nth(1, b, !=, a);
    mock_expect_nth(1, b, >, a);
    mock_expect_nth(1, a, <, b);
    mock_expect_nth(1, a, <=, a);
    mock_expect_nth(2, b, ==, a);
    mock_expect_nth(2, a, ==, b);
    mock_expect_nth(2, a, !=, a);
    mock_expect_nth(2, b, !=, b);
    mock_expect_nth(2, a, >, a);
    mock_expect_nth(2, b, <, b);
    mock_expect_nth(2, a, >=, b);
    mock_expect_nth(2, a, >=, a);
    mock_expect_nth(2, b, <=, a);
    mock_expect_nth(2, b, <=, b);
    add(a, b);
    add(b, a);
  }
}
CTF_TEST(mock_uint_expect_failure) {
  unsigned a = 0;
  unsigned b = 1;
  mock_spy(add) {
    mock_expect(a, ==, b);
    mock_expect(b, ==, a);
    mock_expect(a, !=, a);
    mock_expect(b, !=, b);
    mock_expect(a, >, a);
    mock_expect(b, <, a);
    mock_expect(a, <, a);
    mock_expect(a, >, b);
    mock_expect(b, <=, a);
    mock_expect(a, >=, b);
    add(a, b);
  }
}
CTF_TEST(mock_uint_assert) {
  unsigned a = 0;
  unsigned b = 1;
  mock_spy(add) {
    mock_assert_nth(1, a, ==, a);
    mock_assert_nth(1, b, ==, b);
    mock_assert_nth(1, a, !=, b);
    mock_assert_nth(1, b, !=, a);
    mock_assert_nth(1, b, >, a);
    mock_assert_nth(1, a, <, b);
    mock_assert_nth(1, a, <=, a);
    mock_assert_nth(1, b, >=, a);
    mock_assert_nth(1, a, >=, a);
    mock_assert_nth(1, a, <=, b);
    mock_assert_nth(2, b, ==, a);
    mock_assert_nth(2, a, ==, b);
    mock_assert_nth(2, a, !=, a);
    mock_assert_nth(2, b, !=, b);
    mock_assert_nth(2, a, >, a);
    mock_assert_nth(2, b, <, b);
    mock_assert_nth(2, a, >=, b);
    mock_assert_nth(2, a, >=, a);
    mock_assert_nth(2, b, <=, a);
    mock_assert_nth(2, b, <=, b);
    add(a, b);
    add(b, a);
  }
}
CTF_TEST(mock_ptr_expect_success) {
  char arr[2] = {'a', 'b'};
  const char *a = arr;
  const char *b = arr + 1;
  mock_spy(wrapped_strcmp) {
    mock_expect_nth(1, a, ==, a);
    mock_expect(b, ==, b);
    mock_expect(a, !=, b);
    mock_expect(b, !=, a);
    mock_expect(b, >, a);
    mock_expect(a, <, b);
    mock_expect(a, <=, a);
    mock_expect(b, >=, a);
    mock_expect(a, >=, a);
    mock_expect(a, <=, b);
    mock_expect_nth(2, b, ==, a);
    mock_expect_nth(2, a, ==, b);
    mock_expect_nth(2, a, !=, a);
    mock_expect_nth(2, b, !=, b);
    mock_expect_nth(2, a, >, a);
    mock_expect_nth(2, b, <, b);
    mock_expect_nth(2, a, >=, b);
    mock_expect_nth(2, a, >=, a);
    mock_expect_nth(2, b, <=, a);
    mock_expect_nth(2, b, <=, b);
    (void)wrapped_strcmp(a, b);
    (void)wrapped_strcmp(b, a);
  }
}
CTF_TEST(mock_ptr_expect_failure) {
  char arr[2] = {'a', 'b'};
  const char *a = arr;
  const char *b = arr + 1;
  mock_spy(wrapped_strcmp) {
    mock_expect(a, ==, b);
    mock_expect(b, ==, a);
    mock_expect(a, !=, a);
    mock_expect(b, !=, b);
    mock_expect(a, >, a);
    mock_expect(b, <, a);
    mock_expect(a, <, a);
    mock_expect(a, >, b);
    mock_expect(b, <=, a);
    mock_expect(a, >=, b);
    (void)wrapped_strcmp(a, b);
  }
}
CTF_TEST(mock_ptr_assert) {
  char arr[2] = {'a', 'b'};
  const char *a = arr;
  const char *b = arr + 1;
  mock_spy(wrapped_strcmp) {
    mock_assert_nth(1, a, ==, a);
    mock_assert(b, ==, b);
    mock_assert(a, !=, b);
    mock_assert(b, !=, a);
    mock_assert(b, >, a);
    mock_assert(a, <, b);
    mock_assert(a, <=, a);
    mock_assert(b, >=, a);
    mock_assert(a, >=, a);
    mock_assert(a, <=, b);
    mock_assert_nth(2, b, ==, a);
    mock_assert_nth(2, a, ==, b);
    mock_assert_nth(2, a, !=, a);
    mock_assert_nth(2, b, !=, b);
    mock_assert_nth(2, a, >, a);
    mock_assert_nth(2, b, <, b);
    mock_assert_nth(2, a, >=, b);
    mock_assert_nth(2, a, >=, a);
    mock_assert_nth(2, b, <=, a);
    mock_assert_nth(2, b, <=, b);
    (void)wrapped_strcmp(a, b);
    (void)wrapped_strcmp(b, a);
  }
}
CTF_TEST(mock_str_expect_success) {
  const char a[] = "a";
  const char b[] = "b";
  mock_spy(wrapped_strcmp) {
    mock_expect_str(b, >=, a);
    mock_expect_str(a, >=, a);
    mock_expect_str(a, <=, b);
    mock_expect_nth_str(1, a, ==, a);
    mock_expect_nth_str(1, b, ==, b);
    mock_expect_nth_str(1, a, !=, b);
    mock_expect_nth_str(1, b, !=, a);
    mock_expect_nth_str(1, b, >, a);
    mock_expect_nth_str(1, a, <, b);
    mock_expect_nth_str(1, a, <=, a);
    mock_expect_nth_str(2, b, ==, a);
    mock_expect_nth_str(2, a, ==, b);
    mock_expect_nth_str(2, a, !=, a);
    mock_expect_nth_str(2, b, !=, b);
    mock_expect_nth_str(2, a, >, a);
    mock_expect_nth_str(2, b, <, b);
    mock_expect_nth_str(2, a, >=, b);
    mock_expect_nth_str(2, a, >=, a);
    mock_expect_nth_str(2, b, <=, a);
    mock_expect_nth_str(2, b, <=, b);
    (void)wrapped_strcmp(a, b);
    (void)wrapped_strcmp(b, a);
  }
}
CTF_TEST(mock_str_expect_failure) {
  const char a[] = "a";
  const char b[] = "b";
  mock_spy(wrapped_strcmp) {
    mock_expect_str(a, ==, b);
    mock_expect_str(b, ==, a);
    mock_expect_str(a, !=, a);
    mock_expect_str(b, !=, b);
    mock_expect_str(a, >, a);
    mock_expect_str(b, <, a);
    mock_expect_str(a, <, a);
    mock_expect_str(a, >, b);
    mock_expect_str(b, <=, a);
    mock_expect_str(a, >=, b);
    (void)wrapped_strcmp(a, b);
  }
}
CTF_TEST(mock_str_assert) {
  const char a[] = "a";
  const char b[] = "b";
  mock_spy(wrapped_strcmp) {
    mock_assert_nth_str(1, a, ==, a);
    mock_assert_nth_str(1, b, ==, b);
    mock_assert_nth_str(1, a, !=, b);
    mock_assert_nth_str(1, b, !=, a);
    mock_assert_nth_str(1, b, >, a);
    mock_assert_nth_str(1, a, <, b);
    mock_assert_nth_str(1, a, <=, a);
    mock_assert_nth_str(1, b, >=, a);
    mock_assert_nth_str(1, a, >=, a);
    mock_assert_nth_str(1, a, <=, b);
    mock_assert_nth_str(2, b, ==, a);
    mock_assert_nth_str(2, a, ==, b);
    mock_assert_nth_str(2, a, !=, a);
    mock_assert_nth_str(2, b, !=, b);
    mock_assert_nth_str(2, a, >, a);
    mock_assert_nth_str(2, b, <, b);
    mock_assert_nth_str(2, a, >=, b);
    mock_assert_nth_str(2, a, >=, a);
    mock_assert_nth_str(2, b, <=, a);
    mock_assert_nth_str(2, b, <=, b);
    (void)wrapped_strcmp(a, b);
    (void)wrapped_strcmp(b, a);
  }
}

CTF_GROUP(mocked_add) = {
  mock_char_expect_success, mock_char_assert,         mock_int_expect_success,
  mock_int_assert,          mock_uint_expect_success, mock_uint_assert,
};
CTF_GROUP(mocked_strcmp) = {
  mock_ptr_expect_success,
  mock_ptr_assert,
  mock_str_expect_success,
  mock_str_assert,
};

CTF_TEST(mock_memory_char_expect_success) {
  const char a[] = {'a'};
  const char b[] = {'b'};
  mock_spy(wrapped_memcmp) {
    mock_expect_nth_mem(1, a, ==, a, 1);
    mock_expect_nth_mem(1, a, !=, b, 1);
    mock_expect_mem(b, >, a, 1);
    mock_expect_mem(a, <, b, 1);
    mock_expect_mem(a, >=, a, 1);
    mock_expect_mem(b, >=, a, 1);
    mock_expect_mem(a, <=, a, 1);
    mock_expect_mem(a, <=, b, 1);
    wrapped_memcmp(a, b, 1);
    wrapped_memcmp(b, a, 1);
  }
}
CTF_TEST(mock_memory_char_expect_failure) {
  const char a[] = {'a'};
  const char b[] = {'b'};
  mock_spy(wrapped_memcmp) {
    mock_expect_mem(a, ==, b, 1);
    mock_expect_mem(a, !=, a, 1);
    mock_expect_mem(a, <, a, 1);
    mock_expect_mem(b, <, a, 1);
    mock_expect_mem(a, >, a, 1);
    mock_expect_mem(a, >, b, 1);
    mock_expect_mem(b, <=, a, 1);
    mock_expect_mem(a, >=, b, 1);
    wrapped_memcmp(a, b, 1);
  }
}
CTF_TEST(mock_memory_char_assert) {
  const char a[] = {'a'};
  const char b[] = {'b'};
  mock_spy(wrapped_memcmp) {
    mock_assert_nth_mem(1, a, ==, a, 1);
    mock_assert_nth_mem(1, a, !=, b, 1);
    mock_assert_mem(b, >, a, 1);
    mock_assert_mem(a, <, b, 1);
    mock_assert_mem(a, >=, a, 1);
    mock_assert_mem(b, >=, a, 1);
    mock_assert_mem(a, <=, a, 1);
    mock_assert_mem(a, <=, b, 1);
    wrapped_memcmp(a, b, 1);
    wrapped_memcmp(b, a, 1);
  }
}

CTF_TEST(mock_array_char_expect_success) {
  const char a[] = {'a'};
  const char b[] = {'b'};
  mock_spy(wrapped_memcmp) {
    mock_expect_arr(a, ==, a);
    mock_expect_arr(a, !=, b);
    mock_expect_arr(b, >, a);
    mock_expect_arr(a, <, b);
    mock_expect_arr(a, >=, a);
    mock_expect_arr(b, >=, a);
    mock_expect_arr(a, <=, a);
    mock_expect_arr(a, <=, b);
    wrapped_memcmp(a, b, 1);
    wrapped_memcmp(b, a, 1);
  }
}
CTF_TEST(mock_array_char_expect_failure) {
  const char a[] = {'a'};
  const char b[] = {'b'};
  mock_spy(wrapped_memcmp) {
    mock_expect_arr(a, ==, b);
    mock_expect_arr(a, !=, a);
    mock_expect_arr(a, <, a);
    mock_expect_arr(b, <, a);
    mock_expect_arr(a, >, a);
    mock_expect_arr(a, >, b);
    mock_expect_arr(b, <=, a);
    mock_expect_arr(a, >=, b);
    wrapped_memcmp(a, b, 1);
  }
}
CTF_TEST(mock_array_char_assert) {
  const char a[] = {'a'};
  const char b[] = {'b'};
  mock_spy(wrapped_memcmp) {
    mock_assert_arr(a, ==, a);
    mock_assert_arr(a, !=, b);
    mock_assert_arr(b, >, a);
    mock_assert_arr(a, <, b);
    mock_assert_arr(a, >=, a);
    mock_assert_arr(b, >=, a);
    mock_assert_arr(a, <=, a);
    mock_assert_arr(a, <=, b);
    wrapped_memcmp(a, b, 1);
    wrapped_memcmp(b, a, 1);
  }
}

CTF_GROUP(mocked_memcmp) = {
  mock_memory_char_expect_success,
  mock_memory_char_assert,
};

CTF_GROUP(mock) = {
  mock_grouped,
  mock_return,
};

CTF_TEST(msg_limit) {
  ctf_pass("%8192s", "long formated msg");
  ctf_pass(
    "veryyyy longgg messageeeeee                                               "
    "                                                                          "
    "                                                                          "
    "                                                                          "
    "                                                                          "
    "                                                                          "
    "                                                                          "
    "                                                                          "
    "                                                                          "
    "                                                                          "
    "                                                                          "
    "                                                                          "
    "                                                                          "
    "                                                                          "
    "                ");
}
CTF_TEST(print_buff_limit) { ctf_pass("%1048576s", "long"); }
CTF_TEST(not_mocked) { expect(3, ==, add(1, 2)); }
CTF_GROUP(misc) = {
  msg_limit,
  print_buff_limit,
  not_mocked,
};

CTF_TEST(char_expect_success) {
  char a = 'a';
  char b = 'b';
  expect(a, ==, a);
  expect(a, !=, b);
  expect(a, <, b);
  expect(b, >, a);
  expect(a, <=, a);
  expect(a, <=, b);
  expect(a, >=, a);
  expect(b, >=, a);
}
CTF_TEST(char_expect_failure) {
  char a = 'a';
  char b = 'b';
  expect(a, ==, b);
  expect(a, !=, a);
  expect(a, <, a);
  expect(b, <, a);
  expect(a, >, a);
  expect(a, >, b);
  expect(b, <=, a);
  expect(a, >=, b);
}
CTF_TEST(char_assert) {
  char a = 'a';
  char b = 'b';
  assert(a, ==, a);
  assert(a, !=, b);
  assert(a, <, b);
  assert(b, >, a);
  assert(a, <=, a);
  assert(a, <=, b);
  assert(a, >=, a);
  assert(b, >=, a);
}
CTF_TEST(int_expect_success) {
  int a = -1;
  int b = 0;
  expect(a, ==, a);
  expect(a, !=, b);
  expect(a, <, b);
  expect(b, >, a);
  expect(a, <=, a);
  expect(a, <=, b);
  expect(a, >=, a);
  expect(b, >=, a);
}
CTF_TEST(int_expect_failure) {
  int a = -1;
  int b = 0;
  expect(a, ==, b);
  expect(a, !=, a);
  expect(a, <, a);
  expect(b, <, a);
  expect(a, >, a);
  expect(a, >, b);
  expect(b, <=, a);
  expect(a, >=, b);
}
CTF_TEST(int_assert) {
  int a = -1;
  int b = 0;
  assert(a, ==, a);
  assert(a, !=, b);
  assert(a, <, b);
  assert(b, >, a);
  assert(a, <=, a);
  assert(a, <=, b);
  assert(a, >=, a);
  assert(b, >=, a);
}
CTF_TEST(bool_expect_success) {
  expect_true(1);
  expect_true(2);
  expect_true(-1);
  expect_false(0);
}
CTF_TEST(bool_expect_failure) {
  expect_false(1);
  expect_false(2);
  expect_false(-1);
  expect_true(0);
}
CTF_TEST(bool_assert) {
  assert_true(1);
  assert_true(2);
  assert_true(-1);
  assert_false(0);
}
CTF_TEST(uint_expect_success) {
  unsigned a = 0;
  unsigned b = -1;
  expect(a, ==, a);
  expect(a, !=, b);
  expect(a, <, b);
  expect(b, >, a);
  expect(a, <=, a);
  expect(a, <=, b);
  expect(a, >=, a);
  expect(b, >=, a);
}
CTF_TEST(uint_expect_failure) {
  unsigned a = 0;
  unsigned b = -1;
  expect(a, ==, b);
  expect(a, !=, a);
  expect(a, <, a);
  expect(b, <, a);
  expect(a, >, a);
  expect(a, >, b);
  expect(b, <=, a);
  expect(a, >=, b);
}
CTF_TEST(uint_assert) {
  unsigned a = 0;
  unsigned b = -1;
  assert(a, ==, a);
  assert(a, !=, b);
  assert(a, <, b);
  assert(b, >, a);
  assert(a, <=, a);
  assert(a, <=, b);
  assert(a, >=, a);
  assert(b, >=, a);
}
CTF_TEST(float_expect_success) {
  float a = -0.3;
  float b = 0.9;
  expect(a, ==, a);
  expect(a, !=, b);
  expect(a, <, b);
  expect(b, >, a);
  expect(a, <=, a);
  expect(a, <=, b);
  expect(a, >=, a);
  expect(b, >=, a);
}
CTF_TEST(float_expect_failure) {
  float a = -0.3;
  float b = 0.9;
  expect(a, ==, b);
  expect(a, !=, a);
  expect(a, <, a);
  expect(b, <, a);
  expect(a, >, a);
  expect(a, >, b);
  expect(b, <=, a);
  expect(a, >=, b);
}
CTF_TEST(float_assert) {
  float a = -0.3;
  float b = 0.9;
  assert(a, ==, a);
  assert(a, !=, b);
  assert(a, <, b);
  assert(b, >, a);
  assert(a, <=, a);
  assert(a, <=, b);
  assert(a, >=, a);
  assert(b, >=, a);
}
CTF_TEST(ptr_expect_success) {
  int arr[2];
  int *a = arr;
  int *b = arr + 1;
  expect(a, ==, a);
  expect(a, !=, b);
  expect(a, <, b);
  expect(b, >, a);
  expect(a, <=, a);
  expect(a, <=, b);
  expect(a, >=, a);
  expect(b, >=, a);
}
CTF_TEST(ptr_expect_failure) {
  int arr[2];
  int *a = arr;
  int *b = arr + 1;
  expect(a, ==, b);
  expect(a, !=, a);
  expect(a, <, a);
  expect(b, <, a);
  expect(a, >, a);
  expect(a, >, b);
  expect(b, <=, a);
  expect(a, >=, b);
}
CTF_TEST(ptr_assert) {
  int arr[2];
  int *a = arr;
  int *b = arr + 1;
  assert(a, ==, a);
  assert(a, !=, b);
  assert(a, <, b);
  assert(b, >, a);
  assert(a, <=, a);
  assert(a, <=, b);
  assert(a, >=, a);
  assert(b, >=, a);
}
CTF_TEST(null_expect_success) {
  char a[2];
  expect_null(NULL);
  expect_non_null(a);
  expect_non_null(a + 1);
}
CTF_TEST(null_expect_failure) {
  char a[2];
  expect_non_null(NULL);
  expect_null(a);
  expect_null(a + 1);
}
CTF_TEST(null_assert) {
  char a[2];
  assert_null(NULL);
  assert_non_null(a);
  assert_non_null(a + 1);
}
CTF_TEST(str_expect_success) {
  const char s1[] = "a";
  const char s2[] = "b";
  const char s3[] = "ab";
  const char s4[] = "ba";
  expect_str(s1, ==, s1);
  expect_str(s1, !=, s2);
  expect_str(s1, !=, s3);
  expect_str(s2, !=, s4);
  expect_str(s1, <, s2);
  expect_str(s1, <, s3);
  expect_str(s1, <, s4);
  expect_str(s2, >, s1);
  expect_str(s3, >, s1);
  expect_str(s4, >, s1);
  expect_str(s1, <=, s1);
  expect_str(s3, <=, s3);
  expect_str(s1, <=, s2);
  expect_str(s1, <=, s3);
  expect_str(s1, <=, s4);
  expect_str(s1, >=, s1);
  expect_str(s3, >=, s3);
  expect_str(s2, >=, s1);
  expect_str(s3, >=, s1);
  expect_str(s4, >=, s1);
}
CTF_TEST(str_expect_failure) {
  const char s1[] = "a\na";
  const char s2[] = "b\nb";
  const char s3[] = "ab\rab";
  const char s4[] = "ba\rba";
  expect_str(s1, ==, s2);
  expect_str(s1, ==, s3);
  expect_str(s2, ==, s4);
  expect_str(s1, !=, s1);
  expect_str(s1, <, s1);
  expect_str(s3, <, s3);
  expect_str(s2, <, s1);
  expect_str(s3, <, s1);
  expect_str(s4, <, s1);
  expect_str(s1, >, s1);
  expect_str(s3, >, s3);
  expect_str(s1, >, s2);
  expect_str(s1, >, s3);
  expect_str(s1, >, s4);
  expect_str(s2, <=, s1);
  expect_str(s3, <=, s1);
  expect_str(s4, <=, s1);
  expect_str(s1, >=, s2);
  expect_str(s1, >=, s3);
  expect_str(s1, >=, s4);
}
CTF_TEST(str_assert) {
  const char s1[] = "a";
  const char s2[] = "b";
  const char s3[] = "ab";
  const char s4[] = "ba";
  assert_str(s1, ==, s1);
  assert_str(s1, !=, s2);
  assert_str(s1, !=, s3);
  assert_str(s2, !=, s4);
  assert_str(s1, <, s2);
  assert_str(s1, <, s3);
  assert_str(s1, <, s4);
  assert_str(s2, >, s1);
  assert_str(s3, >, s1);
  assert_str(s4, >, s1);
  assert_str(s1, <=, s1);
  assert_str(s3, <=, s3);
  assert_str(s1, <=, s2);
  assert_str(s1, <=, s3);
  assert_str(s1, <=, s4);
  assert_str(s1, >=, s1);
  assert_str(s3, >=, s3);
  assert_str(s2, >=, s1);
  assert_str(s3, >=, s1);
  assert_str(s4, >=, s1);
}
CTF_GROUP(primitive_success) = {
  char_expect_success,  char_assert,  int_expect_success,  int_assert,
  bool_expect_success,  bool_assert,  uint_expect_success, uint_assert,
  float_expect_success, float_assert, ptr_expect_success,  ptr_assert,
  null_expect_success,  null_assert,  str_expect_success,  str_assert,
};

CTF_TEST(array_char_expect_success) {
  const char a1[] = {'a'};
  const char a2[] = {'b'};
  const char a3[] = {'a', 'b'};
  const char a4[] = {'b', 'a'};
  expect_arr(a1, ==, a1);
  expect_arr(a1, !=, a2);
  expect_arr(a1, !=, a3);
  expect_arr(a2, !=, a4);
  expect_arr(a1, <, a2);
  expect_arr(a1, <, a3);
  expect_arr(a1, <, a4);
  expect_arr(a2, >, a1);
  expect_arr(a3, >, a1);
  expect_arr(a4, >, a1);
  expect_arr(a1, <=, a1);
  expect_arr(a3, <=, a3);
  expect_arr(a1, <=, a2);
  expect_arr(a1, <=, a3);
  expect_arr(a1, <=, a4);
  expect_arr(a1, >=, a1);
  expect_arr(a3, >=, a3);
  expect_arr(a2, >=, a1);
  expect_arr(a3, >=, a1);
  expect_arr(a4, >=, a1);
}
CTF_TEST(array_char_expect_failure) {
  const char a1[] = {'a'};
  const char a2[] = {'b'};
  const char a3[] = {'a', 'b'};
  const char a4[] = {'b', 'a'};
  expect_arr(a1, ==, a2);
  expect_arr(a1, ==, a3);
  expect_arr(a2, ==, a4);
  expect_arr(a1, !=, a1);
  expect_arr(a1, <, a1);
  expect_arr(a3, <, a3);
  expect_arr(a2, <, a1);
  expect_arr(a3, <, a1);
  expect_arr(a4, <, a1);
  expect_arr(a1, >, a1);
  expect_arr(a3, >, a3);
  expect_arr(a1, >, a2);
  expect_arr(a1, >, a3);
  expect_arr(a1, >, a4);
  expect_arr(a2, <=, a1);
  expect_arr(a3, <=, a1);
  expect_arr(a4, <=, a1);
  expect_arr(a1, >=, a2);
  expect_arr(a1, >=, a3);
  expect_arr(a1, >=, a4);
}
CTF_TEST(array_char_assert) {
  const char a1[] = {'a'};
  const char a2[] = {'b'};
  const char a3[] = {'a', 'b'};
  const char a4[] = {'b', 'a'};
  assert_arr(a1, ==, a1);
  assert_arr(a1, !=, a2);
  assert_arr(a1, !=, a3);
  assert_arr(a2, !=, a4);
  assert_arr(a1, <, a2);
  assert_arr(a1, <, a3);
  assert_arr(a1, <, a4);
  assert_arr(a2, >, a1);
  assert_arr(a3, >, a1);
  assert_arr(a4, >, a1);
  assert_arr(a1, <=, a1);
  assert_arr(a3, <=, a3);
  assert_arr(a1, <=, a2);
  assert_arr(a1, <=, a3);
  assert_arr(a1, <=, a4);
  assert_arr(a1, >=, a1);
  assert_arr(a3, >=, a3);
  assert_arr(a2, >=, a1);
  assert_arr(a3, >=, a1);
  assert_arr(a4, >=, a1);
}
CTF_TEST(array_int_expect_success) {
  const int a1[] = {-2};
  const int a2[] = {0};
  const int a3[] = {-2, 0};
  const int a4[] = {0, -2};
  expect_arr(a1, ==, a1);
  expect_arr(a1, !=, a2);
  expect_arr(a1, !=, a3);
  expect_arr(a2, !=, a4);
  expect_arr(a1, <, a2);
  expect_arr(a1, <, a3);
  expect_arr(a1, <, a4);
  expect_arr(a2, >, a1);
  expect_arr(a3, >, a1);
  expect_arr(a4, >, a1);
  expect_arr(a1, <=, a1);
  expect_arr(a3, <=, a3);
  expect_arr(a1, <=, a2);
  expect_arr(a1, <=, a3);
  expect_arr(a1, <=, a4);
  expect_arr(a1, >=, a1);
  expect_arr(a3, >=, a3);
  expect_arr(a2, >=, a1);
  expect_arr(a3, >=, a1);
  expect_arr(a4, >=, a1);
}
CTF_TEST(array_int_expect_failure) {
  const int a1[] = {-2};
  const int a2[] = {-1};
  const int a3[] = {-2, -1};
  const int a4[] = {-1, -2};
  expect_arr(a1, ==, a2);
  expect_arr(a1, ==, a3);
  expect_arr(a2, ==, a4);
  expect_arr(a1, !=, a1);
  expect_arr(a1, <, a1);
  expect_arr(a3, <, a3);
  expect_arr(a2, <, a1);
  expect_arr(a3, <, a1);
  expect_arr(a4, <, a1);
  expect_arr(a1, >, a1);
  expect_arr(a3, >, a3);
  expect_arr(a1, >, a2);
  expect_arr(a1, >, a3);
  expect_arr(a1, >, a4);
  expect_arr(a2, <=, a1);
  expect_arr(a3, <=, a1);
  expect_arr(a4, <=, a1);
  expect_arr(a1, >=, a2);
  expect_arr(a1, >=, a3);
  expect_arr(a1, >=, a4);
}
CTF_TEST(array_int_assert) {
  const int a1[] = {-2};
  const int a2[] = {-1};
  const int a3[] = {-2, -1};
  const int a4[] = {-1, -2};
  assert_arr(a1, ==, a1);
  assert_arr(a1, !=, a2);
  assert_arr(a1, !=, a3);
  assert_arr(a2, !=, a4);
  assert_arr(a1, <, a2);
  assert_arr(a1, <, a3);
  assert_arr(a1, <, a4);
  assert_arr(a2, >, a1);
  assert_arr(a3, >, a1);
  assert_arr(a4, >, a1);
  assert_arr(a1, <=, a1);
  assert_arr(a3, <=, a3);
  assert_arr(a1, <=, a2);
  assert_arr(a1, <=, a3);
  assert_arr(a1, <=, a4);
  assert_arr(a1, >=, a1);
  assert_arr(a3, >=, a3);
  assert_arr(a2, >=, a1);
  assert_arr(a3, >=, a1);
  assert_arr(a4, >=, a1);
}
CTF_TEST(array_uint_expect_success) {
  const unsigned a1[] = {1};
  const unsigned a2[] = {2};
  const unsigned a3[] = {1, 2};
  const unsigned a4[] = {2, 1};
  expect_arr(a1, ==, a1);
  expect_arr(a1, !=, a2);
  expect_arr(a1, !=, a3);
  expect_arr(a2, !=, a4);
  expect_arr(a1, <, a2);
  expect_arr(a1, <, a3);
  expect_arr(a1, <, a4);
  expect_arr(a2, >, a1);
  expect_arr(a3, >, a1);
  expect_arr(a4, >, a1);
  expect_arr(a1, <=, a1);
  expect_arr(a3, <=, a3);
  expect_arr(a1, <=, a2);
  expect_arr(a1, <=, a3);
  expect_arr(a1, <=, a4);
  expect_arr(a1, >=, a1);
  expect_arr(a3, >=, a3);
  expect_arr(a2, >=, a1);
  expect_arr(a3, >=, a1);
  expect_arr(a4, >=, a1);
}
CTF_TEST(array_uint_expect_failure) {
  const unsigned a1[] = {1};
  const unsigned a2[] = {2};
  const unsigned a3[] = {1, 2};
  const unsigned a4[] = {2, 1};
  expect_arr(a1, ==, a2);
  expect_arr(a1, ==, a3);
  expect_arr(a2, ==, a4);
  expect_arr(a1, !=, a1);
  expect_arr(a1, <, a1);
  expect_arr(a3, <, a3);
  expect_arr(a2, <, a1);
  expect_arr(a3, <, a1);
  expect_arr(a4, <, a1);
  expect_arr(a1, >, a1);
  expect_arr(a3, >, a3);
  expect_arr(a1, >, a2);
  expect_arr(a1, >, a3);
  expect_arr(a1, >, a4);
  expect_arr(a2, <=, a1);
  expect_arr(a3, <=, a1);
  expect_arr(a4, <=, a1);
  expect_arr(a1, >=, a2);
  expect_arr(a1, >=, a3);
  expect_arr(a1, >=, a4);
}
CTF_TEST(array_uint_assert) {
  const unsigned a1[] = {1};
  const unsigned a2[] = {2};
  const unsigned a3[] = {1, 2};
  const unsigned a4[] = {2, 1};
  assert_arr(a1, ==, a1);
  assert_arr(a1, !=, a2);
  assert_arr(a1, !=, a3);
  assert_arr(a2, !=, a4);
  assert_arr(a1, <, a2);
  assert_arr(a1, <, a3);
  assert_arr(a1, <, a4);
  assert_arr(a2, >, a1);
  assert_arr(a3, >, a1);
  assert_arr(a4, >, a1);
  assert_arr(a1, <=, a1);
  assert_arr(a3, <=, a3);
  assert_arr(a1, <=, a2);
  assert_arr(a1, <=, a3);
  assert_arr(a1, <=, a4);
  assert_arr(a1, >=, a1);
  assert_arr(a3, >=, a3);
  assert_arr(a2, >=, a1);
  assert_arr(a3, >=, a1);
  assert_arr(a4, >=, a1);
}
CTF_TEST(array_float_expect_success) {
  const float a1[] = {-0.1};
  const float a2[] = {0.1};
  const float a3[] = {-0.1, 0.1};
  const float a4[] = {0.1, -0.1};
  expect_arr(a1, ==, a1);
  expect_arr(a1, !=, a2);
  expect_arr(a1, !=, a3);
  expect_arr(a2, !=, a4);
  expect_arr(a1, <, a2);
  expect_arr(a1, <, a3);
  expect_arr(a1, <, a4);
  expect_arr(a2, >, a1);
  expect_arr(a3, >, a1);
  expect_arr(a4, >, a1);
  expect_arr(a1, <=, a1);
  expect_arr(a3, <=, a3);
  expect_arr(a1, <=, a2);
  expect_arr(a1, <=, a3);
  expect_arr(a1, <=, a4);
  expect_arr(a1, >=, a1);
  expect_arr(a3, >=, a3);
  expect_arr(a2, >=, a1);
  expect_arr(a3, >=, a1);
  expect_arr(a4, >=, a1);
}
CTF_TEST(array_float_expect_failure) {
  const float a1[] = {-0.1};
  const float a2[] = {0.1};
  const float a3[] = {-0.1, 0.1};
  const float a4[] = {0.1, -0.1};
  expect_arr(a1, ==, a2);
  expect_arr(a1, ==, a3);
  expect_arr(a2, ==, a4);
  expect_arr(a1, !=, a1);
  expect_arr(a1, <, a1);
  expect_arr(a3, <, a3);
  expect_arr(a2, <, a1);
  expect_arr(a3, <, a1);
  expect_arr(a4, <, a1);
  expect_arr(a1, >, a1);
  expect_arr(a3, >, a3);
  expect_arr(a1, >, a2);
  expect_arr(a1, >, a3);
  expect_arr(a1, >, a4);
  expect_arr(a2, <=, a1);
  expect_arr(a3, <=, a1);
  expect_arr(a4, <=, a1);
  expect_arr(a1, >=, a2);
  expect_arr(a1, >=, a3);
  expect_arr(a1, >=, a4);
}
CTF_TEST(array_float_assert) {
  const float a1[] = {-0.1};
  const float a2[] = {0.1};
  const float a3[] = {-0.1, 0.1};
  const float a4[] = {0.1, -0.1};
  assert_arr(a1, ==, a1);
  assert_arr(a1, !=, a2);
  assert_arr(a1, !=, a3);
  assert_arr(a2, !=, a4);
  assert_arr(a1, <, a2);
  assert_arr(a1, <, a3);
  assert_arr(a1, <, a4);
  assert_arr(a2, >, a1);
  assert_arr(a3, >, a1);
  assert_arr(a4, >, a1);
  assert_arr(a1, <=, a1);
  assert_arr(a3, <=, a3);
  assert_arr(a1, <=, a2);
  assert_arr(a1, <=, a3);
  assert_arr(a1, <=, a4);
  assert_arr(a1, >=, a1);
  assert_arr(a3, >=, a3);
  assert_arr(a2, >=, a1);
  assert_arr(a3, >=, a1);
  assert_arr(a4, >=, a1);
}
CTF_TEST(array_ptr_expect_success) {
  int arr[2];
  const int *const a1[] = {arr};
  const int *const a2[] = {arr + 1};
  const int *const a3[] = {arr, arr + 1};
  const int *const a4[] = {arr + 1, arr};
  expect_arr(a1, ==, a1);
  expect_arr(a1, !=, a2);
  expect_arr(a1, !=, a3);
  expect_arr(a2, !=, a4);
  expect_arr(a1, <, a2);
  expect_arr(a1, <, a3);
  expect_arr(a1, <, a4);
  expect_arr(a2, >, a1);
  expect_arr(a3, >, a1);
  expect_arr(a4, >, a1);
  expect_arr(a1, <=, a1);
  expect_arr(a3, <=, a3);
  expect_arr(a1, <=, a2);
  expect_arr(a1, <=, a3);
  expect_arr(a1, <=, a4);
  expect_arr(a1, >=, a1);
  expect_arr(a3, >=, a3);
  expect_arr(a2, >=, a1);
  expect_arr(a3, >=, a1);
  expect_arr(a4, >=, a1);
}
CTF_TEST(array_ptr_expect_failure) {
  int arr[2];
  const int *const a1[] = {arr};
  const int *const a2[] = {arr + 1};
  const int *const a3[] = {arr, arr + 1};
  const int *const a4[] = {arr + 1, arr};
  expect_arr(a1, ==, a2);
  expect_arr(a1, ==, a3);
  expect_arr(a2, ==, a4);
  expect_arr(a1, !=, a1);
  expect_arr(a1, <, a1);
  expect_arr(a3, <, a3);
  expect_arr(a2, <, a1);
  expect_arr(a3, <, a1);
  expect_arr(a4, <, a1);
  expect_arr(a1, >, a1);
  expect_arr(a3, >, a3);
  expect_arr(a1, >, a2);
  expect_arr(a1, >, a3);
  expect_arr(a1, >, a4);
  expect_arr(a2, <=, a1);
  expect_arr(a3, <=, a1);
  expect_arr(a4, <=, a1);
  expect_arr(a1, >=, a2);
  expect_arr(a1, >=, a3);
  expect_arr(a1, >=, a4);
}
CTF_TEST(array_ptr_assert) {
  int arr[2];
  const int *const a1[] = {arr};
  const int *const a2[] = {arr + 1};
  const int *const a3[] = {arr, arr + 1};
  const int *const a4[] = {arr + 1, arr};
  assert_arr(a1, ==, a1);
  assert_arr(a1, !=, a2);
  assert_arr(a1, !=, a3);
  assert_arr(a2, !=, a4);
  assert_arr(a1, <, a2);
  assert_arr(a1, <, a3);
  assert_arr(a1, <, a4);
  assert_arr(a2, >, a1);
  assert_arr(a3, >, a1);
  assert_arr(a4, >, a1);
  assert_arr(a1, <=, a1);
  assert_arr(a3, <=, a3);
  assert_arr(a1, <=, a2);
  assert_arr(a1, <=, a3);
  assert_arr(a1, <=, a4);
  assert_arr(a1, >=, a1);
  assert_arr(a3, >=, a3);
  assert_arr(a2, >=, a1);
  assert_arr(a3, >=, a1);
  assert_arr(a4, >=, a1);
}
CTF_GROUP(array_success) = {
  array_char_expect_success,  array_char_assert,
  array_int_expect_success,   array_int_assert,
  array_uint_expect_success,  array_uint_assert,
  array_float_expect_success, array_float_assert,
  array_ptr_expect_success,   array_ptr_assert,
};

CTF_TEST(memory_char_expect_success) {
  const char a1[] = {'a'};
  const char a2[] = {'b'};
  expect_mem(a1, ==, a1, 1);
  expect_mem(a1, !=, a2, 1);
  expect_mem(a1, <, a2, 1);
  expect_mem(a2, >, a1, 1);
  expect_mem(a1, <=, a1, 1);
  expect_mem(a1, <=, a2, 1);
  expect_mem(a1, >=, a1, 1);
  expect_mem(a2, >=, a1, 1);
}
CTF_TEST(memory_char_expect_failure) {
  const char a1[] = {'a'};
  const char a2[] = {'b'};
  expect_mem(a1, ==, a2, 1);
  expect_mem(a1, !=, a1, 1);
  expect_mem(a1, <, a1, 1);
  expect_mem(a2, <, a1, 1);
  expect_mem(a1, >, a1, 1);
  expect_mem(a1, >, a2, 1);
  expect_mem(a2, <=, a1, 1);
  expect_mem(a1, >=, a2, 1);
}
CTF_TEST(memory_char_assert) {
  const char a1[] = {'a'};
  const char a2[] = {'b'};
  assert_mem(a1, ==, a1, 1);
  assert_mem(a1, !=, a2, 1);
  assert_mem(a1, <, a2, 1);
  assert_mem(a2, >, a1, 1);
  assert_mem(a1, <=, a1, 1);
  assert_mem(a1, <=, a2, 1);
  assert_mem(a1, >=, a1, 1);
  assert_mem(a2, >=, a1, 1);
}
CTF_TEST(memory_int_expect_success) {
  const int a1[] = {-2};
  const int a2[] = {0};
  expect_mem(a1, ==, a1, 1);
  expect_mem(a1, !=, a2, 1);
  expect_mem(a1, <, a2, 1);
  expect_mem(a2, >, a1, 1);
  expect_mem(a1, <=, a1, 1);
  expect_mem(a1, <=, a2, 1);
  expect_mem(a1, >=, a1, 1);
  expect_mem(a2, >=, a1, 1);
}
CTF_TEST(memory_int_expect_failure) {
  const int a1[] = {-2};
  const int a2[] = {-1};
  expect_mem(a1, ==, a2, 1);
  expect_mem(a1, !=, a1, 1);
  expect_mem(a1, <, a1, 1);
  expect_mem(a2, <, a1, 1);
  expect_mem(a1, >, a1, 1);
  expect_mem(a1, >, a2, 1);
  expect_mem(a2, <=, a1, 1);
  expect_mem(a1, >=, a2, 1);
}
CTF_TEST(memory_int_assert) {
  const int a1[] = {-2};
  const int a2[] = {-1};
  assert_mem(a1, ==, a1, 1);
  assert_mem(a1, !=, a2, 1);
  assert_mem(a1, <, a2, 1);
  assert_mem(a2, >, a1, 1);
  assert_mem(a1, <=, a1, 1);
  assert_mem(a1, <=, a2, 1);
  assert_mem(a1, >=, a1, 1);
  assert_mem(a2, >=, a1, 1);
}
CTF_TEST(memory_uint_expect_success) {
  const unsigned a1[] = {1};
  const unsigned a2[] = {2};
  expect_mem(a1, ==, a1, 1);
  expect_mem(a1, !=, a2, 1);
  expect_mem(a1, <, a2, 1);
  expect_mem(a2, >, a1, 1);
  expect_mem(a1, <=, a1, 1);
  expect_mem(a1, <=, a2, 1);
  expect_mem(a1, >=, a1, 1);
  expect_mem(a2, >=, a1, 1);
}
CTF_TEST(memory_uint_expect_failure) {
  const unsigned a1[] = {1};
  const unsigned a2[] = {2};
  expect_mem(a1, ==, a2, 1);
  expect_mem(a1, !=, a1, 1);
  expect_mem(a1, <, a1, 1);
  expect_mem(a2, <, a1, 1);
  expect_mem(a1, >, a1, 1);
  expect_mem(a1, >, a2, 1);
  expect_mem(a2, <=, a1, 1);
  expect_mem(a1, >=, a2, 1);
}
CTF_TEST(memory_uint_assert) {
  const unsigned a1[] = {1};
  const unsigned a2[] = {2};
  assert_mem(a1, ==, a1, 1);
  assert_mem(a1, !=, a2, 1);
  assert_mem(a1, <, a2, 1);
  assert_mem(a2, >, a1, 1);
  assert_mem(a1, <=, a1, 1);
  assert_mem(a1, <=, a2, 1);
  assert_mem(a1, >=, a1, 1);
  assert_mem(a2, >=, a1, 1);
}
CTF_TEST(memory_float_expect_success) {
  const float a1[] = {-0.1};
  const float a2[] = {0.1};
  expect_mem(a1, ==, a1, 1);
  expect_mem(a1, !=, a2, 1);
  expect_mem(a1, <, a2, 1);
  expect_mem(a2, >, a1, 1);
  expect_mem(a1, <=, a1, 1);
  expect_mem(a1, <=, a2, 1);
  expect_mem(a1, >=, a1, 1);
  expect_mem(a2, >=, a1, 1);
}
CTF_TEST(memory_float_expect_failure) {
  const float a1[] = {-0.1};
  const float a2[] = {0.1};
  expect_mem(a1, ==, a2, 1);
  expect_mem(a1, !=, a1, 1);
  expect_mem(a1, <, a1, 1);
  expect_mem(a2, <, a1, 1);
  expect_mem(a1, >, a1, 1);
  expect_mem(a1, >, a2, 1);
  expect_mem(a2, <=, a1, 1);
  expect_mem(a1, >=, a2, 1);
}
CTF_TEST(memory_float_assert) {
  const float a1[] = {-0.1};
  const float a2[] = {0.1};
  assert_mem(a1, ==, a1, 1);
  assert_mem(a1, !=, a2, 1);
  assert_mem(a1, <, a2, 1);
  assert_mem(a2, >, a1, 1);
  assert_mem(a1, <=, a1, 1);
  assert_mem(a1, <=, a2, 1);
  assert_mem(a1, >=, a1, 1);
  assert_mem(a2, >=, a1, 1);
}
CTF_TEST(memory_ptr_expect_success) {
  int arr[2];
  const int *const a1[] = {arr};
  const int *const a2[] = {arr + 1};
  expect_mem(a1, ==, a1, 1);
  expect_mem(a1, !=, a2, 1);
  expect_mem(a1, <, a2, 1);
  expect_mem(a2, >, a1, 1);
  expect_mem(a1, <=, a1, 1);
  expect_mem(a1, <=, a2, 1);
  expect_mem(a1, >=, a1, 1);
  expect_mem(a2, >=, a1, 1);
}
CTF_TEST(memory_ptr_expect_failure) {
  int arr[2];
  const int *const a1[] = {arr};
  const int *const a2[] = {arr + 1};
  expect_mem(a1, ==, a2, 1);
  expect_mem(a1, !=, a1, 1);
  expect_mem(a1, <, a1, 1);
  expect_mem(a2, <, a1, 1);
  expect_mem(a1, >, a1, 1);
  expect_mem(a1, >, a2, 1);
  expect_mem(a2, <=, a1, 1);
  expect_mem(a1, >=, a2, 1);
}
CTF_TEST(memory_ptr_assert) {
  int arr[2];
  const int *const a1[] = {arr};
  const int *const a2[] = {arr + 1};
  assert_mem(a1, ==, a1, 1);
  assert_mem(a1, !=, a2, 1);
  assert_mem(a1, <, a2, 1);
  assert_mem(a2, >, a1, 1);
  assert_mem(a1, <=, a1, 1);
  assert_mem(a1, <=, a2, 1);
  assert_mem(a1, >=, a1, 1);
  assert_mem(a2, >=, a1, 1);
}
CTF_GROUP(memory_success) = {
  memory_char_expect_success,  memory_char_assert,
  memory_int_expect_success,   memory_int_assert,
  memory_uint_expect_success,  memory_uint_assert,
  memory_float_expect_success, memory_float_assert,
  memory_ptr_expect_success,   memory_ptr_assert,
};

CTF_TEST(pass_and_fail) {
  ctf_pass("pass");
  ctf_fail("fail");
}

CTF_TEST(subtests) {
  subtest(sub_fail) ctf_fail("fail");
  subtest(sub_pass) ctf_pass("pass");
  subtest(sub_with_nesting) {
    subtest(nested_pass) ctf_pass("pass");
    subtest(nested_fail) ctf_fail("fail");
  }
  subtest(sub_pass_with_nesting) { subtest(nested_pass) ctf_pass("pass"); }
}

CTF_GROUP(failure) = {
  char_expect_failure,
  int_expect_failure,
  bool_expect_failure,
  uint_expect_failure,
  float_expect_failure,
  ptr_expect_failure,
  null_expect_failure,
  str_expect_failure,
  array_char_expect_failure,
  array_int_expect_failure,
  array_uint_expect_failure,
  array_float_expect_failure,
  array_ptr_expect_failure,
  memory_char_expect_failure,
  memory_int_expect_failure,
  memory_uint_expect_failure,
  memory_float_expect_failure,
  memory_ptr_expect_failure,
  pass_and_fail,
  subtests,
};
CTF_GROUP(mocked_add_failure) = {
  mock_char_expect_failure,
  mock_int_expect_failure,
  mock_uint_expect_failure,
};
CTF_GROUP(mocked_strcmp_failure) = {
  mock_ptr_expect_failure,
  mock_str_expect_failure,
};
CTF_GROUP(mocked_memcmp_failure) = {
  mock_memory_char_expect_failure,
  mock_array_char_expect_failure,
};
