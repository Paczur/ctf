#include "ctf.h"

#include <setjmp.h>
#include <signal.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

include(`base.m4')

#define IS_TTY !isatty(STDOUT_FILENO)
#define HELP_MSG                                                               \
  "Run tests embedded in this executable.\n"                                   \
  "\n"                                                                         \
  "Options:\n"                                                                 \
  "-h, --help       Show this message\n"                                       \
  "-u, --unicode    (off|generic|branded*) display of unicode symbols\n"       \
  "-c, --color      (off|on|auto*) color coding for failed and passed tests\n" \
  "-d, --detail     (off|on|auto*) detailed info about failed tests\n"         \
  "-v, --verbosity  0-3 verbosity of printing (default 1)\n"                   \
  "-s, --statistics Disables printing of statistics at the end\n"              \
  "-j, --jobs       Number of parallel threads to run (default 1)\n"           \
  "--sigsegv        Don't register SIGSEGV handler\n"                          \
  "--cleanup        Free memory allocations\n"                                 \
  "--,              Stop parsing arguments\n"
#define OFF 0
#define ON 1
#define AUTO 2
#define GENERIC 1
#define BRANDED 2

#define TASK_QUEUE_MAX 64

#define DEFAULT_STATE_MSG_CAPACITY 128
#define DEFAULT_THREAD_DATA_TEST_ELEMENTS_CAPACITY 8
#define DEFAULT_THREAD_DATA_MOCK_RESET_STACK_CAPACITY 8
#define DEFAULT_CLEANUP_LIST_CAPACITY 64

#define ISSPACE(c) (((c) >= '\t' && (c) <= '\r') || (c) == ' ')
#define SKIP_SPACE(c)      \
  do {                     \
    while(ISSPACE(*(c))) { \
      (c)++;               \
    }                      \
  } while(0)
#define NEXT_ID(c, i)                                           \
  do {                                                          \
    while(!ISSPACE((c)[i]) && (c)[i] != ',' && (c)[i] != ')') { \
      (i)++;                                                    \
    }                                                           \
  } while(0)
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define STRINGIFY2(x) #x
#define STRINGIFY(x) STRINGIFY2(x)

#define CTF__MALLOC(size, thread_index) \
  ((ctf__opt_cleanup) ? ctf__cleanup_malloc(size, thread_index) : malloc(size))
#define CTF__REALLOC(ptr, size, thread_index)                         \
  ((ctf__opt_cleanup) ? ctf__cleanup_realloc(ptr, size, thread_index) \
                      : realloc(ptr, size))

static unsigned opt_unicode = BRANDED;
static unsigned opt_color = AUTO;
static unsigned opt_detail = AUTO;
static unsigned opt_verbosity = 1;
static unsigned opt_wrap = 60;
int ctf__opt_cleanup = 0;
int ctf__opt_threads = 1;
static int opt_statistics = 1;
static int tty_present = 0;

static pthread_t *restrict parallel_threads;
static int parallel_threads_waiting = 0;
static struct ctf__group parallel_task_queue[TASK_QUEUE_MAX] = {0};
static pthread_mutex_t parallel_task_queue_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t parallel_threads_waiting_all = PTHREAD_COND_INITIALIZER;
static pthread_cond_t parallel_task_queue_non_empty = PTHREAD_COND_INITIALIZER;
static pthread_cond_t parallel_task_queue_non_full = PTHREAD_COND_INITIALIZER;
static uintmax_t parallel_state = 0;
static jmp_buf *restrict ctf__assert_jmp_buff;
static struct {
  void **pointers;
  uintmax_t size;
  uintmax_t capacity;
} *cleanup_list;

char ctf_signal_altstack[CTF_CONST_SIGNAL_STACK_SIZE];
pthread_key_t ctf__thread_index;
int ctf_exit_code = 0;
struct ctf__thread_data *restrict ctf__thread_data;

include(`test_element.c')
include(`print.c')

void *ctf__cleanup_realloc(void *ptr, uintmax_t size, uintptr_t thread_index) {
  void *new = realloc(ptr, size);
  if(new == ptr) return new;
  for(uintmax_t i = 0; i < cleanup_list[thread_index].size; i++) {
    if(cleanup_list[thread_index].pointers[i] == ptr) {
      cleanup_list[thread_index].pointers[i] = new;
      return new;
    }
  }
  return new;
}

void *ctf__cleanup_malloc(uintmax_t size, uintptr_t thread_index) {
  void *new = malloc(size);
  if(cleanup_list[thread_index].size + 1 >=
     cleanup_list[thread_index].capacity) {
    cleanup_list[thread_index].capacity *= 2;
    cleanup_list[thread_index].pointers =
      realloc(cleanup_list[thread_index].pointers,
              sizeof(cleanup_list[0].pointers[0]) *
                cleanup_list[thread_index].capacity);
  }
  cleanup_list[thread_index].pointers[cleanup_list[thread_index].size++] = new;
  return new;
}

static void stats_init(struct ctf__stats *stats) {
  stats->groups_passed = 0;
  stats->groups_failed = 0;
  stats->tests_passed = 0;
  stats->tests_failed = 0;
  stats->asserts_passed = 0;
  stats->asserts_failed = 0;
  stats->expects_passed = 0;
  stats->expects_failed = 0;
  stats->subtests_passed = 0;
  stats->subtests_failed = 0;
}

static void test_element_init(struct ctf__test_element *el) {
  el->el.subtest = NULL;
}

static void thread_data_init(struct ctf__thread_data *data) {
  data->test_elements_size = 0;
  data->subtest_current = NULL;
  data->mock_reset_stack_size = 0;
  data->test_elements_capacity = DEFAULT_THREAD_DATA_TEST_ELEMENTS_CAPACITY;
  data->mock_reset_stack_capacity =
    DEFAULT_THREAD_DATA_MOCK_RESET_STACK_CAPACITY;
  data->test_elements =
    malloc(data->test_elements_capacity * sizeof(*data->test_elements));
  data->mock_reset_stack =
    malloc(data->mock_reset_stack_capacity * sizeof(*data->mock_reset_stack));
  for(uintmax_t k = 0; k < data->test_elements_capacity; k++)
    test_element_init(data->test_elements + k);
  if(opt_statistics) stats_init(&data->stats);
}

static void thread_data_deinit(uintptr_t thread_index) {
  struct ctf__thread_data *thread_data = ctf__thread_data + thread_index;
  test_elements_cleanup(thread_index);
  free(thread_data->test_elements);
  free(thread_data->mock_reset_stack);
}

static void pthread_key_destr(void *v) {
  (void)v;
}

static int get_value(const char *opt) {
  if(!strcmp(opt, "on")) return ON;
  if(!strcmp(opt, "off")) return OFF;
  if(!strcmp(opt, "auto")) return AUTO;
  puts(HELP_MSG);
  exit(0);
  return 0;
}

static void err(void) {
  puts(HELP_MSG);
  exit(0);
}

static void test_cleanup(uintptr_t thread_index) {
  struct ctf__thread_data *thread_data = ctf__thread_data + thread_index;
  for(uintmax_t i = 0; i < thread_data->mock_reset_stack_size; i++) {
    thread_data->mock_reset_stack[i]->states[thread_index].mock_f = NULL;
  }
  thread_data->mock_reset_stack_size = 0;
}

static void group_run_helper(struct ctf__group group, struct buff *buff) {
  uintptr_t thread_index = (uintptr_t)pthread_getspecific(ctf__thread_index);
  struct ctf__thread_data *thread_data = ctf__thread_data + thread_index;
  uintmax_t temp_size;
  uintmax_t test_name_len;
  const uintmax_t group_name_len = strlen(group.name);
  int status;
  volatile int group_status = 0;
  buff->size = 0;

  buff_reserve(buff, print_name_status(NULL, group.name, group_name_len, 2, 0));
  print_name_status(buff, group.name, group_name_len, 2, 0);

  group.setup();
  for(int i = 0; i < CTF_CONST_GROUP_SIZE && group.tests[i].f; i++) {
    test_name_len = strlen(group.tests[i].name);

    temp_size =
      print_name_status(NULL, group.tests[i].name, test_name_len, 2, 1);
    buff_reserve(buff, temp_size);
    print_name_status(buff, group.tests[i].name, test_name_len, 2, 1);

    group.test_setup();
    if(!setjmp(ctf__assert_jmp_buff[thread_index])) group.tests[i].f();
    group.test_teardown();
    test_cleanup(thread_index);
    status = test_status(thread_index);

    if(status) {
      group_status = 1;
      if(opt_statistics) thread_data->stats.tests_failed++;
    } else {
      if(opt_statistics) thread_data->stats.tests_passed++;
    }

    buff->size -= temp_size;
    buff_reserve(
      buff, print_test(NULL, group.tests + i, test_name_len, thread_index));
    print_test(buff, group.tests + i, test_name_len, thread_index);
    test_elements_cleanup(thread_index);
  }
  group.teardown();
  temp_size = buff->size;
  buff->size = 0;
  print_name_status(buff, group.name, group_name_len, group_status, 0);
  buff->size = temp_size;
  if(opt_statistics) {
    if(group_status) {
      thread_data->stats.groups_failed++;
    } else {
      thread_data->stats.groups_passed++;
    }
  }
  if(group_status) ctf_exit_code = 1;
}

static void group_run(struct ctf__group group) {
  uintptr_t thread_index = (uintptr_t)pthread_getspecific(ctf__thread_index);
  group_run_helper(group, print_buff + thread_index);
#pragma GCC diagnostic ignored "-Wunused-result"
  write(STDOUT_FILENO, print_buff[thread_index].buff,
        print_buff[thread_index].size);
#pragma GCC diagnostic pop
}

static void groups_run(uintmax_t count, va_list args) {
  struct ctf__group group;
  uintptr_t thread_index = (uintptr_t)pthread_getspecific(ctf__thread_index);
  for(uintmax_t i = 0; i < count; i++) {
    group = va_arg(args, struct ctf__group);
    group_run_helper(group, print_buff + thread_index);
#pragma GCC diagnostic ignored "-Wunused-result"
    write(STDOUT_FILENO, print_buff[thread_index].buff,
          print_buff[thread_index].size);
#pragma GCC diagnostic pop
  }
}

static uintmax_t parallel_get_thread_index(void) {
  const pthread_t thread = pthread_self();
  for(int i = 0; i < ctf__opt_threads; i++) {
    if(pthread_equal(parallel_threads[i], thread)) {
      return i;
    }
  }
  return -1;
}

static void *parallel_thread_loop(void *data) {
  (void)data;
  struct ctf__group group;
  uintptr_t thread_index = (uintptr_t)data;
  pthread_setspecific(ctf__thread_index, (void *)thread_index);
  while(1) {
    pthread_mutex_lock(&parallel_task_queue_mutex);
    if(parallel_task_queue[0].tests == NULL) {
      if(parallel_threads_waiting == ctf__opt_threads - 1)
        pthread_cond_signal(&parallel_threads_waiting_all);
      parallel_threads_waiting++;
      while(parallel_task_queue[0].tests == NULL && parallel_state) {
        pthread_cond_wait(&parallel_task_queue_non_empty,
                          &parallel_task_queue_mutex);
      }
      parallel_threads_waiting--;
      if(parallel_task_queue[0].tests == NULL && !parallel_state) {
        pthread_mutex_unlock(&parallel_task_queue_mutex);
        return 0;
      }
    }
    group = parallel_task_queue[0];
    for(int i = 0;
        i < TASK_QUEUE_MAX - 1 && parallel_task_queue[i].tests != NULL; i++) {
      parallel_task_queue[i] = parallel_task_queue[i + 1];
    }
    pthread_cond_signal(&parallel_task_queue_non_full);
    pthread_mutex_unlock(&parallel_task_queue_mutex);
    group_run_helper(group, print_buff + thread_index);
    pthread_mutex_lock(&parallel_print_mutex);
#pragma GCC diagnostic ignored "-Wunused-result"
    write(STDOUT_FILENO, print_buff[thread_index].buff,
          print_buff[thread_index].size);
#pragma GCC diagnostic pop
    print_buff[thread_index].size = 0;
    pthread_mutex_unlock(&parallel_print_mutex);
  }
}

static void parallel_group_run(struct ctf__group group) {
  pthread_mutex_lock(&parallel_task_queue_mutex);
  while(parallel_task_queue[TASK_QUEUE_MAX - 1].tests != NULL) {
    pthread_cond_wait(&parallel_task_queue_non_full,
                      &parallel_task_queue_mutex);
  }
  for(int i = 0; i < TASK_QUEUE_MAX; i++) {
    if(parallel_task_queue[i].tests == NULL) {
      parallel_task_queue[i] = group;
      break;
    }
  }
  pthread_cond_signal(&parallel_task_queue_non_empty);
  pthread_mutex_unlock(&parallel_task_queue_mutex);
}

static void parallel_groups_run(int count, va_list args) {
  struct ctf__group group;

  pthread_mutex_lock(&parallel_task_queue_mutex);
  for(int j = 0; j < count; j++) {
    group = va_arg(args, struct ctf__group);
    while(parallel_task_queue[TASK_QUEUE_MAX - 1].tests != NULL) {
      pthread_cond_wait(&parallel_task_queue_non_full,
                        &parallel_task_queue_mutex);
    }
    for(int i = 0; i < TASK_QUEUE_MAX; i++) {
      if(parallel_task_queue[i].tests == NULL) {
        parallel_task_queue[i] = group;
        break;
      }
    }
  }
  pthread_cond_broadcast(&parallel_task_queue_non_empty);
  pthread_mutex_unlock(&parallel_task_queue_mutex);
}

int main(int argc, char *argv[]) {
  int i;
  int handle_sigsegv = 1;
  for(i = 1; i < argc; i++) {
    if(argv[i][0] != '-') err();
    if(!strcmp(argv[i] + 1, "h") || !strcmp(argv[i] + 1, "-help")) {
      err();
    } else if(!strcmp(argv[i] + 1, "u") || !strcmp(argv[i] + 1, "-unicode")) {
      i++;
      if(i >= argc) err();
      if(!strcmp(argv[i], "off")) {
        opt_unicode = OFF;
      } else if(!strcmp(argv[i], "generic")) {
        opt_unicode = GENERIC;
      } else if(!strcmp(argv[i], "branded")) {
        opt_unicode = BRANDED;
      }
    } else if(!strcmp(argv[i] + 1, "c") || !strcmp(argv[i] + 1, "-color")) {
      i++;
      if(i >= argc) err();
      opt_color = get_value(argv[i]);
    } else if(!strcmp(argv[i] + 1, "d") || !strcmp(argv[i] + 1, "-detail")) {
      i++;
      if(i >= argc) err();
      opt_detail = get_value(argv[i]);
    } else if(!strcmp(argv[i] + 1, "v") || !strcmp(argv[i] + 1, "-verbosity")) {
      i++;
      if(i >= argc) err();
      sscanf(argv[i], "%u", &opt_verbosity);
    } else if(!strcmp(argv[i] + 1, "j") || !strcmp(argv[i] + 1, "-jobs")) {
      i++;
      if(i >= argc) err();
      sscanf(argv[i], "%u", &ctf__opt_threads);
      if(ctf__opt_threads < 1) ctf__opt_threads = 1;
    } else if(!strcmp(argv[i] + 1, "w") || !strcmp(argv[i] + 1, "-wrap")) {
      i++;
      if(i >= argc) err();
      sscanf(argv[i], "%u", &opt_wrap);
    } else if(!strcmp(argv[i] + 1, "s") ||
              !strcmp(argv[i] + 1, "-statistics")) {
      opt_statistics = 0;
    } else if(!strcmp(argv[i] + 1, "-sigsegv")) {
      handle_sigsegv = 0;
    } else if(!strcmp(argv[i] + 1, "-cleanup")) {
      ctf__opt_cleanup = 1;
    } else if(!strcmp(argv[i] + 1, "-")) {
      i++;
      break;
    }
  }
  tty_present = !IS_TTY;
  if(opt_color == AUTO) {
    color = tty_present;
  } else {
    color = opt_color;
  }
  if(opt_detail == AUTO) {
    detail = !tty_present;
  } else {
    detail = opt_detail;
  }
  if(handle_sigsegv) {
    sigaction(SIGSEGV,
              &(struct sigaction){.sa_handler = ctf_sigsegv_handler,
                                  .sa_flags = SA_ONSTACK | SA_RESETHAND},
              NULL);
    sigaltstack(&(stack_t){.ss_sp = ctf_signal_altstack,
                           .ss_size = CTF_CONST_SIGNAL_STACK_SIZE},
                NULL);
  }
  pthread_key_create(&ctf__thread_index, pthread_key_destr);
  pthread_setspecific(ctf__thread_index, (void *)0);
  void *data =
    malloc((sizeof(*parallel_threads) + sizeof(*print_buff) +
            sizeof(*ctf__assert_jmp_buff) + sizeof(*ctf__thread_data)) *
           ctf__opt_threads);
  parallel_threads = data;
  print_buff = data + sizeof(*parallel_threads) * ctf__opt_threads;
  ctf__assert_jmp_buff =
    data + (sizeof(*parallel_threads) + sizeof(*print_buff)) * ctf__opt_threads;
  ctf__thread_data = data + (sizeof(*parallel_threads) + sizeof(*print_buff) +
                             sizeof(*ctf__assert_jmp_buff)) *
                              ctf__opt_threads;
  for(int j = 0; j < ctf__opt_threads; j++) {
    thread_data_init(ctf__thread_data + j);
    print_buff[j].buff = malloc(DEFAULT_PRINT_BUFF_SIZE);
    print_buff[j].size = 0;
    print_buff[j].capacity = DEFAULT_PRINT_BUFF_SIZE;
  }
  if(ctf__opt_cleanup) {
    cleanup_list = malloc(sizeof(*cleanup_list) * ctf__opt_threads);
    for(int j = 0; j < ctf__opt_threads; j++) {
      cleanup_list[j].pointers = malloc(sizeof(cleanup_list[0].pointers[0]) *
                                        DEFAULT_CLEANUP_LIST_CAPACITY);
      cleanup_list[j].size = 0;
      cleanup_list[j].capacity = DEFAULT_CLEANUP_LIST_CAPACITY;
    }
  }
  test_elements_init();
  ctf_main(argc - i, argv + i);
  ctf_parallel_stop();
  if(opt_statistics) {
    for(int j = 1; j < ctf__opt_threads; j++) {
      ctf__thread_data[0].stats.groups_passed +=
        ctf__thread_data[j].stats.groups_passed;
      ctf__thread_data[0].stats.groups_failed +=
        ctf__thread_data[j].stats.groups_failed;
      ctf__thread_data[0].stats.tests_passed +=
        ctf__thread_data[j].stats.tests_passed;
      ctf__thread_data[0].stats.tests_failed +=
        ctf__thread_data[j].stats.tests_failed;
      ctf__thread_data[0].stats.asserts_passed +=
        ctf__thread_data[j].stats.asserts_passed;
      ctf__thread_data[0].stats.asserts_failed +=
        ctf__thread_data[j].stats.asserts_failed;
      ctf__thread_data[0].stats.expects_passed +=
        ctf__thread_data[j].stats.expects_passed;
      ctf__thread_data[0].stats.expects_failed +=
        ctf__thread_data[j].stats.expects_failed;
      ctf__thread_data[0].stats.subtests_passed +=
        ctf__thread_data[j].stats.subtests_passed;
      ctf__thread_data[0].stats.subtests_failed +=
        ctf__thread_data[j].stats.subtests_failed;
    }
    print_stats(&ctf__thread_data[0].stats);
  }
  if(ctf__opt_cleanup) {
    for(int j = 0; j < ctf__opt_threads; j++) {
      thread_data_deinit(j);
      for(uintmax_t k = 0; k < cleanup_list[j].size; k++)
        free(cleanup_list[j].pointers[k]);
      free(cleanup_list[j].pointers);
      free(print_buff[j].buff);
    }
    test_elements_deinit();
    pthread_key_delete(ctf__thread_index);
    free(data);
    free(cleanup_list);
  }
  return ctf_exit_code;
}

void ctf_parallel_sync(void) {
  if(ctf__opt_threads == 1) return;
  if(!parallel_state) return;
  pthread_mutex_lock(&parallel_task_queue_mutex);
  while(parallel_threads_waiting != ctf__opt_threads ||
        parallel_task_queue[0].tests != NULL) {
    pthread_cond_wait(&parallel_threads_waiting_all,
                      &parallel_task_queue_mutex);
  }
  fflush(stdout);
  pthread_mutex_unlock(&parallel_task_queue_mutex);
}

void ctf_parallel_start(void) {
  if(ctf__opt_threads == 1) return;
  parallel_state = 1;
  for(intptr_t i = 0; i < ctf__opt_threads; i++) {
    pthread_create(parallel_threads + i, NULL, parallel_thread_loop, (void *)i);
  }
}

void ctf_parallel_stop(void) {
  if(ctf__opt_threads == 1) return;
  parallel_state = 0;
  pthread_mutex_lock(&parallel_task_queue_mutex);
  pthread_cond_broadcast(&parallel_task_queue_non_empty);
  pthread_mutex_unlock(&parallel_task_queue_mutex);
  for(int i = 0; i < ctf__opt_threads; i++) {
    pthread_join(parallel_threads[i], NULL);
  }
}

void ctf_group_run(const struct ctf__group group) {
  if(parallel_state) {
    parallel_group_run(group);
  } else {
    group_run(group);
  }
}

void ctf__groups_run(int count, ...) {
  va_list args;
  va_start(args, count);
  if(parallel_state) {
    parallel_groups_run(count, args);
  } else {
    groups_run(count, args);
  }
  va_end(args);
}

include(`assert.c')
include(`mocks.c')
