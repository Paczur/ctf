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
Framework is distributed as source and header pair,
Enabling thread supports requires defining `CTF_PARALLEL` as number of threads
used and linking library with pthreads `-lpthreads`.

### Documentation
#### Basic
```
CTF_TEST(name) {} // test creation
CTF_GROUP(group_name, test1, test2, ...) // group creation
CTF_EXTERN_TEST(name); // external definition for test
CTF_EXTERN_GROUP(group_name); // external definition for group
ctf_group_run(&group_name); // run singular group
ctf_groups_run(&group_name1, &group_name2, ...); // run multiple groups
ctf_exit_code; // combined group status
ctf_barrier(); // if(ctf_exit_code) return ctf_exit_code;
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

ctf_fail(msg); // Adds failed test with message
ctf_pass(msg); // Adds passed test with message
```
#### Parallel
```
ctf_parallel_start(); // starts threads (changes following functions to parallel versions)
ctf_parallel_stop(); // waits for threads and then stops them (changes following functions to sequential versions)
ctf_parallel_sync(); // waits for threads to finnish all tasks

// Parallel versions of sequential functions
ctf_group_run(&group_name); // adds group to queue
ctf_groups_run(&group_name1, &group_name2, ...); // adds groups to queue
ctf_barrier(); // syncs threads and then returns depending on ctf_exit_code
```
#### Configuration
```
// Creates aliases to assert/expect macros without `ctf_` prefix
#define CTF_ASSERT_ALIASES
// Enables parallel functions for current file
#define CTF_PARALLEL
/* Changes symbols used for passed and failed tests, valid options:
 * CTF_OFF - "P", "F"
 * CTF_UNICODE_GENERIC - "✓", "✗"
 * CTF_UNICODE_BRANDED - "✓", "⚑"
 * CTF_USER_DEFINED - requires user to define CTF_PRINT_PASS and CTF_PRINT_FAIL
 */
#define CTF_UNICODE CTF_UNICODE_BRANDED
/* Changes printing detailed, parsable info in tests (file,line,status), valid options:
 * CTF_OFF - Always displays simple test passed/failed information
 * CTF_ON - Always displays full test information
 * CTF_AUTO - Display detailed information only on failed tests and when output is not terminal
 */
#define CTF_DETAIL CTF_AUTO
/* Changes color coding failed and passed tests, valid options:
 * CTF_OFF - Never print color codes
 * CTF_ON - Always print color codes
 * CTF_AUTO - Print color codes only when outputing to terminal
 */
#define CTF_COLOR CTF_AUTO

// Macros used for specifing internal details, use only when encountering a bug

// Size of buffer used to hold name of group
#define CTF_CONST_GROUP_NAME_SIZE CTF_CONST_GROUP_NAME_SIZE_DEFAULT
// Size of buffer used for printing test status for group
#define CTF_CONST_PRINT_BUFF_SIZE CTF_CONST_PRINT_BUFF_SIZE_DEFAULT
// Size of buffer used to hold file name for detailed test info
#define CTF_CONST_STATE_FILE_SIZE CTF_CONST_PRINT_FILE_SIZE_DEFAULT
// Size of buffer used to hold test messages
#define CTF_CONST_STATE_MSG_SIZE CTF_CONST_PRINT_MSG_SIZE_DEFAULT
// Number of expects + 1 allowed for every test
#define CTF_CONST_STATES_PER_TEST CTF_CONST_STATES_PER_TEST_DEFAULT
// Size of queue used to hold groups to run in parallel
#define CTF_PARALLEL_CONST_TASK_QUEUE_MAX CTF_PARALLEL_CONST_TASK_QUEUE_MAX_DEFAULT
```
