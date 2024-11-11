## C Testing Framework (ctf)
(Unit) Testing framework supporting GNU C99 and newer offering
wide support of asserts and expects while maintaing clutter-free prints.

### Features
- Simple interface
- Wide selection of asserts
- Only useful information about passed tests
- Expects
- (optional) Parallel group execution

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

CTF_TEST(add_signed) {
  assert_int_eq(3, 1+2);
}

CTF_GROUP(add, add_signed)

int main(void) {
    ctf_group_run(&add);
    return ctf_exit_code;
}
```
#### Parallel execution of tests
```
#define CTF_PARALLEL 4 //Number of threads in threadpool
static int add(int a, int b) {
    return a+b;
}

CTF_TEST(add_signed) {
  assert_int_eq(3, 1+2);
}

CTF_GROUP(add, add_signed)

int main(void) {
    ctf_parallel_start();
    ctf_group_run(&add);
    ctf_parallel_stop();
    return ctf_exit_code;
}
```
#### Vim support
Adding following line to .vimrc allows vim to parse detailed test info, and add it
to quickfix list.
```
set errorformat^=%.%#[%f\|%l\|%t]\ %m,
```

### Building
#### Prerequisites
- C99 or newer
- `__thread`(GCC/Clang) or `_Thread_local`(C11) support
- `__attribute__((constructor))`(GCC/Clang) support
- pthreads
#### Notes
Framework is distributed as source and header pair, compiling source requires
linking with pthreads.

### Documentation
#### Declarations and definitions
```
CTF_TEST(name) {} // test creation
CTF_GROUP(group_name, test1, test2, ...) // group creation
CTF_EXTERN_TEST(name); // external definition for test
CTF_EXTERN_GROUP(group_name); // external definition for group
```
#### Execution control
```
ctf_group_run(&group_name); // run singular group
ctf_groups_run(&group_name1, &group_name2, ...); // run multiple groups
ctf_exit_code; // combined group status
ctf_barrier(); // syncs threads and then returns depending on ctf_exit_code

ctf_parallel_start(); // starts threads
ctf_parallel_stop(); // waits for threads and then stops them
ctf_parallel_sync(); // waits for threads to finnish all tasks
```
#### Asserts and expects
```
#define CTF_ASSERT_ALIASES // creates aliasses to asserts and expects without ctf_ prefix

/* valid types:
 *   char - prints character instead of number
 *   int - uses intmax_t for storage and display
 *   uint - uses uintmax_t for storage and display
 *   ptr
 *   string - not present for array/memory asserts
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

ctf_assert_true(a);
ctf_assert_false(a);
ctf_expect_true(a);
ctf_expect_false(a);

ctf_fail(msg); // Failes test with message
ctf_pass(msg); // Adds passed test with message
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
```
