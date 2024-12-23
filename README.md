## C Testing Framework (ctf)
(Unit) Testing framework supporting GNU C99 and newer offering
wide support of asserts and expects while maintaing clutter-free prints.

### Features
- Simple interface
- Wide selection of asserts
- Only useful information about passed tests
- Expects
- Parallel group execution
- Mocks

Not yet implemented:
- Support for other operating systems than linux
- Testing on mingw and clang
- Different output formats

### Usage
#### Simple test for function
```
static int add(int a, int b) {
    return a+b;
}

CTF_TEST(add_test) {
  assert(3, ==, 1+2);
}

CTF_GROUP(add, add_test)

int main(void) {
    ctf_group_run(&add);
    return ctf_exit_code;
}
```

### Installation
#### Requirements
- C99 (optional C11 macros)
- pthreads
- typeof support
#### Instructions
```
make && sudo make install
```

### Notes
#### Vim support
Adding following line to .vimrc allows vim to parse detailed test info, and add it
to quickfix list.
```
set errorformat^=%.%#[%f\|%l\|%t]\ %m,
```

### Documentation
#### Declarations and definitions
```
#define CTF_ALIASES // creates aliasses to commonly used functions and macros without ctf_ prefix
CTF_TEST(name) {} // test creation
CTF_P_TEST(name) //test pointer (used in making group)
CTF_GROUP(group_name, test1, test2, ...) // group creation
CTF_EXTERN_TEST(name); // external definition for test
CTF_EXTERN_GROUP(group_name); // external definition for group
CTF_P_GROUP(name) // group pointer (used in groups_run)

CTF_MOCK(ret_type, fn_name, (typed_args), (untyped_args))
CTF_EXTERN_MOCK(ret_type, fn_name, (typed_args))
CTF_MOCK_BIND(fn_to_mock, mock) // Binding of mock to functions (used in MOCK_GROUP)
CTF_MOCK_GROUP(name)
CTF_EXTERN_MOCK_GROUP(name)
```
#### Execution control
```
ctf_group_run(&group_name); // run singular group
ctf_groups_run(&group_name1, &group_name2, ...); // run multiple groups
ctf_exit_code; // combined group status
ctf_barrier(); // syncs threads and then returns depending on ctf_exit_code
ctf_sigsegv_handler(unused); // used only when compiled with CTF_NO_SIGNAL

ctf_parallel_start(); // starts threads
ctf_parallel_stop(); // waits for threads and then stops them
ctf_parallel_sync(); // waits for threads to finnish all tasks
```
#### Asserts and expects
```
/* valid types:
 *   char - prints character instead of number
 *   int - uses intmax_t for storage and display
 *   uint - uses uintmax_t for storage and display
 *   ptr
 *   str - not present for array/memory asserts
 * valid comparisons:
 *   eq - equal
 *   neq - not equal
 *   lt - less than
 *   gt - greater than
 *   lte - less than or equal to
 *   gte - greater than or equal to
 */
ctf_assert_[type]_[comparison](a, b);
ctf_assert_memory_[type]_[comparison](a, b, length);
ctf_assert_array_[type]_[comparison](a, b, length);

ctf_expect_[type]_[comparison](a, b);
ctf_expect_memory_[type]_[comparison](a, b, length);
ctf_expect_array_[type]_[comparison](a, b, length);

ctf_assert_msg(test, msg);
ctf_assert(test);
ctf_expect_msg(test, msg);
ctf_expect(test);

ctf_fail(msg); // Failes test with message
ctf_pass(msg); // Adds passed test with message
```
#### Mocks
```
ctf_mock(fn_to_mock, fn);
ctf_unmock(fn_mocked);
ctf_mock_call_count(mocked_fn)
ctf_mock_will_return(mocked_fn, val);
ctf_mock_will_return_once(mocked_fn, val);

ctf_mock_check_[type]_[comparison](fn_mocked, id);
ctf_mock_check_memory_[type]_[comparison](fn_mocked, id);
// Using ctf_mock_check_memory or ctf_mock_check_str creates ctf_mock_check_ptr

ctf_mock_assert_[type]_[comparison](fn_mocked, id, value);
ctf_mock_assert_memory_[type]_[comparison](fn_mocked, id, value, length);
ctf_mock_assert_array_[type]_[comparison](fn_mocked, id, value);
ctf_mock_assert_once_[type]_[comparison](fn_mocked, id, value);
ctf_mock_assert_once_memory_[type]_[comparison](fn_mocked, id, value, length);
ctf_mock_assert_once_array_[type]_[comparison](fn_mocked, id, value);

ctf_mock_expect_[type]_[comparison](fn_mocked, id, value);
ctf_mock_expect_memory_[type]_[comparison](fn_mocked, id, value, length);
ctf_mock_expect_array_[type]_[comparison](fn_mocked, id, value);
ctf_mock_expect_once_[type]_[comparison](fn_mocked, id, value);
ctf_mock_expect_once_memory_[type]_[comparison](fn_mocked, id, value, length);
ctf_mock_expect_once_array_[type]_[comparison](fn_mocked, id, value);
```
#### Configuration
Specified when:
- including library
  * `#define CTF_ASSERT_ALIASES CTF_OFF` - removes aliases to assert/expect macros
- running binary
```
-h, --help     Show this help
-u, --unicode  (off|generic|branded*) display of unicode symbols
-c, --color    (off|on|auto*) color coding for failed and passed tests
-d, --detail   (off|on|auto*) detailed info about failed tests
-f, --failed   Print only groups that failed
-j, --jobs     Number of parallel threads to run (default 1)
-s, --sigsegv  Don't register SIGSEGV handler
```
