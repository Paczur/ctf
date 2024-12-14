void ctf_assert_barrier(void) {
  intptr_t thread_index = (intptr_t)pthread_getspecific(ctf__thread_index);
  struct ctf__thread_data *thread_data = ctf__thread_data + thread_index;
  for(uintmax_t i = 0; i < thread_data->states_size; i++) {
    if(thread_data->states[i].status) {
      longjmp(ctf__assert_jmp_buff[thread_index], 1);
    }
  }
}

void ctf__assert_fold(uintmax_t count, const char *msg, int line,
                      const char *file) {
  ctf_assert_hide(count);
  ctf__pass(msg, line, file);
}

void ctf_assert_hide(uintmax_t count) {
  intptr_t thread_index = (intptr_t)pthread_getspecific(ctf__thread_index);
  struct ctf__thread_data *thread_data = ctf__thread_data + thread_index;
  if(thread_data->states_size <= count) {
    for(uintmax_t i = 0; i < thread_data->states_size; i++) {
      if(thread_data->states[i].status) {
        longjmp(ctf__assert_jmp_buff[thread_index], 1);
      }
    }
    thread_data->states_size = 0;
  } else {
    for(uintmax_t i = thread_data->states_size - count;
        i < thread_data->states_size; i++) {
      if(thread_data->states[i].status) {
        longjmp(ctf__assert_jmp_buff[thread_index], 1);
      }
    }
    thread_data->states_size -= count;
  }
}

uintmax_t ctf__fail(const char *m, int line, const char *file, ...) {
  va_list v;
  intptr_t thread_index = (intptr_t)pthread_getspecific(ctf__thread_index);
  struct ctf__thread_data *thread_data = ctf__thread_data + thread_index;
  thread_data_states_increment(thread_data);
  thread_data->states[thread_data->states_size - 1].status = 1;
  assert_copy(thread_data->states + thread_data->states_size - 1, line, file);
  va_start(v, file);
  MSG_VSPRINTF(thread_data->states[thread_data->states_size - 1].msg, m, v);
  va_end(v);
  return 0;
}

uintmax_t ctf__pass(const char *m, int line, const char *file, ...) {
  va_list v;
  intptr_t thread_index = (intptr_t)pthread_getspecific(ctf__thread_index);
  struct ctf__thread_data *thread_data = ctf__thread_data + thread_index;
  thread_data_states_increment(thread_data);
  thread_data->states[thread_data->states_size - 1].status = 0;
  assert_copy(thread_data->states + thread_data->states_size - 1, line, file);
  va_start(v, file);
  MSG_VSPRINTF(thread_data->states[thread_data->states_size - 1].msg, m, v);
  va_end(v);
  return 1;
}

uintmax_t ctf__assert_msg(int status, const char *msg, int line,
                          const char *file, ...) {
  va_list args;
  intptr_t thread_index = (intptr_t)pthread_getspecific(ctf__thread_index);
  struct ctf__thread_data *thread_data = ctf__thread_data + thread_index;
  thread_data_states_increment(thread_data);
  thread_data->states[thread_data->states_size - 1].status = status;
  assert_copy(thread_data->states + thread_data->states_size - 1, line, file);
  thread_data->states[thread_data->states_size - 1].msg_size = 0;
  va_start(args, file);
  if(status) {
    thread_data->stats.asserts_passed++;
  } else {
    thread_data->stats.asserts_failed++;
  }
  MSG_VSPRINTF(thread_data->states[thread_data->states_size - 1].msg, msg,
               args);
  va_end(args);
  return status;
}

// clang-format off
/* EXPECTS AND ASSERTS
   define(`EXPECT_HELPER',
   `int ctf__expect_$1_$2($3 a, $3 b, const char *a_str,
   const char *b_str, int line, const char *file) {
   int status;
   intptr_t thread_index = (intptr_t)pthread_getspecific(ctf__thread_index);
   struct ctf__thread_data *thread_data = ctf__thread_data + thread_index;
   thread_data_states_increment(thread_data);
   thread_data->states[thread_data->states_size-1].status = 2;
   assert_copy(thread_data->states + thread_data->states_size - 1, line,
   file);
   MSG_SPRINTF(thread_data->states[thread_data->states_size -1].msg,
   "%s $4 %s ( "$5" $4 "$5" )", a_str, b_str, a, b);
   status = $6 $4 $7;
   thread_data->states[thread_data->states_size - 1].status = !status;
   if(status) {
   thread_data->stats.expects_passed++;
   } else {
   thread_data->stats.expects_failed++;
   }
   return status;
   }')
   define(`EXPECT_MEMORY_HELPER',
   `int ctf__expect_memory_$1_$2(
   $3(a), $3(b), uintmax_t l, uintmax_t step, int sign,
   const char *a_str, const char *b_str, int line, const char *file) {
   int status;
   intptr_t thread_index = (intptr_t)pthread_getspecific(ctf__thread_index);
   struct ctf__thread_data *thread_data = ctf__thread_data + thread_index;
   thread_data_states_increment(thread_data);
   thread_data->states[thread_data->states_size-1].status = 2;
   assert_copy(thread_data->states + thread_data->states_size - 1, line,
   file);
   print_mem(thread_data->states + thread_data->states_size - 1, a, b, l, l,
   step, sign, a_str, b_str, "$4", $5 ", ");
   status = thread_data->states[thread_data->states_size - 1].status $4 0;
   thread_data->states[thread_data->states_size - 1].status = !status;
   if(status) {
   thread_data->stats.expects_passed++;
   } else {
   thread_data->stats.expects_failed++;
   }
   return status;
   }')
   define(`EXPECT_ARRAY_HELPER',
   `int ctf__expect_array_$1_$2(
   $3(a), $3(b), uintmax_t la, uintmax_t lb, uintmax_t step,
   int sign, const char *a_str, const char *b_str, int line,
   const char *file) {
   int status;
   intptr_t thread_index = (intptr_t)pthread_getspecific(ctf__thread_index);
   struct ctf__thread_data *thread_data = ctf__thread_data + thread_index;
   thread_data_states_increment(thread_data);
   thread_data->states[thread_data->states_size-1].status = 2;
   assert_copy(thread_data->states + thread_data->states_size - 1, line,
   file);
   print_mem(thread_data->states + thread_data->states_size - 1, a, b, la,
   lb, step, sign, a_str, b_str, "$4", $5 ", ");
   if(thread_data->states[thread_data->states_size - 1].status == 0) {
   status = (la $4 lb);
   } else {
   status =
   (thread_data->states[thread_data->states_size - 1].status $4 0);
   }
   thread_data->states[thread_data->states_size - 1].status = !status;
   if(status) {
   thread_data->stats.expects_passed++;
   } else {
   thread_data->stats.expects_failed++;
   }
   return status;
   }')
   define(`ASSERT_HELPER',
   `int ctf__assert_$1_$2($3 a, $3 b, const char *a_str,
   const char *b_str, int line, const char *file) {
   int status;
   intptr_t thread_index = (intptr_t)pthread_getspecific(ctf__thread_index);
   struct ctf__thread_data *thread_data = ctf__thread_data + thread_index;
   thread_data_states_increment(thread_data);
   thread_data->states[thread_data->states_size-1].status = 2;
   assert_copy(thread_data->states + thread_data->states_size - 1, line,
   file);
   MSG_SPRINTF(thread_data->states[thread_data->states_size - 1].msg,
   "%s $4 %s ( "$5" $4 "$5" )", a_str, b_str, a,
   b);
   status = $6 $4 $7;
thread_data->states[thread_data->states_size - 1].status = !status;
if(status) {
thread_data->stats.asserts_passed++;
} else {
thread_data->stats.asserts_failed++;
}
if(!status) longjmp(ctf__assert_jmp_buff[thread_index], 1);
    return status;
}')
define(`ASSERT_MEMORY_HELPER',
       `int ctf__assert_memory_$1_$2(
                                                                           $3(a), $3(b), uintmax_t l, uintmax_t step, int sign,
                                                                           const char *a_str, const char *b_str, int line, const char *file) {
       int status;
       intptr_t thread_index = (intptr_t)pthread_getspecific(ctf__thread_index);
       struct ctf__thread_data *thread_data = ctf__thread_data + thread_index;
       thread_data_states_increment(thread_data);
       thread_data->states[thread_data->states_size-1].status = 2;
       assert_copy(thread_data->states + thread_data->states_size - 1, line,
                   file);
       print_mem(thread_data->states + thread_data->states_size - 1, a, b, l, l,
                 step, sign, a_str, b_str, "$4", $5 ", ");
       status = thread_data->states[thread_data->states_size - 1].status $4 0;
       thread_data->states[thread_data->states_size - 1].status = !status;
       if(status) {
       thread_data->stats.asserts_passed++;
       } else {
       thread_data->stats.asserts_failed++;
       }
       if(!status) longjmp(ctf__assert_jmp_buff[thread_index], 1);
       return status;
       }')
define(`ASSERT_ARRAY_HELPER',
       `int ctf__assert_array_$1_$2(
                                                                         $3(a), $3(b), uintmax_t la, uintmax_t lb, uintmax_t step,     int sign, const char *a_str, const char *b_str, int line,
                                                                         const char *file) {
       int status;
       intptr_t thread_index = (intptr_t)pthread_getspecific(ctf__thread_index);
       struct ctf__thread_data *thread_data = ctf__thread_data + thread_index;
       thread_data_states_increment(thread_data);
       thread_data->states[thread_data->states_size-1].status = 2;
       assert_copy(thread_data->states + thread_data->states_size - 1, line,
                   file);
       print_mem(thread_data->states + thread_data->states_size - 1, a, b, la,
                 lb, step, sign, a_str, b_str, "$4", $5 ", ");
       if(thread_data->states[thread_data->states_size - 1].status == 0) {
       status = (la $4 lb);
       } else {
       status =
       (thread_data->states[thread_data->states_size - 1].status $4 0);
       }
       thread_data->states[thread_data->states_size - 1].status = !status;
       if(status) {
       thread_data->stats.asserts_passed++;
       } else {
       thread_data->stats.asserts_failed++;
       }
       if(!status) longjmp(ctf__assert_jmp_buff[thread_index], 1);
       return status;
       }')
*/
/* DEFINES
   define(`EXPECT_PRIMITIVE', `EXPECT_HELPER(`$1',`$2',TYPE(`$1'),CMP_SYMBOL(`$2'),FORMAT(`$1'),a,b)')
   define(`EXPECT_STRING', `EXPECT_HELPER(`$1',`$2',TYPE(`$1'),CMP_SYMBOL(`$2'),FORMAT(`$1'),strcmp(a,b),0)')
   define(`EXPECT_MEMORY', `EXPECT_MEMORY_HELPER(`$1',`$2',`$3',CMP_SYMBOL(`$2'),FORMAT(`$1'))')
   define(`EXPECT_ARRAY', `EXPECT_ARRAY_HELPER(`$1',`$2',`$3',CMP_SYMBOL(`$2'),FORMAT(`$1'))')
   define(`ASSERT_PRIMITIVE', `ASSERT_HELPER(`$1',`$2',TYPE(`$1'),CMP_SYMBOL(`$2'),FORMAT(`$1'),a,b)')
   define(`ASSERT_STRING', `ASSERT_HELPER(`$1',`$2',TYPE(`$1'),CMP_SYMBOL(`$2'),FORMAT(`$1'),strcmp(a,b),0)')
   define(`ASSERT_MEMORY', `ASSERT_MEMORY_HELPER(`$1',`$2',`$3',CMP_SYMBOL(`$2'),FORMAT(`$1'))')
   define(`ASSERT_ARRAY', `ASSERT_ARRAY_HELPER(`$1',`$2',`$3',CMP_SYMBOL(`$2'),FORMAT(`$1'))')
   */
COMB2(`EXPECT_PRIMITIVE', `(PRIMITIVE_TYPES)', `(CMPS)')
COMB2(`EXPECT_STRING', `(str)', `(CMPS)')
COMB3(`EXPECT_MEMORY', `(char, int, uint)', `(CMPS)', `(const void *)')
COMB3(`EXPECT_MEMORY', `(ptr)', `(CMPS)', `(const void *)')
COMB3(`EXPECT_ARRAY', `(char, int, uint)', `(CMPS)', `(const void *)')
COMB3(`EXPECT_ARRAY', `(ptr)', `(CMPS)', `(const void *)')
COMB2(`ASSERT_PRIMITIVE', `(PRIMITIVE_TYPES)', `(CMPS)')
COMB2(`ASSERT_STRING', `(str)', `(CMPS)')
COMB3(`ASSERT_MEMORY', `(char, int, uint)', `(CMPS)', `(const void *)')
COMB3(`ASSERT_MEMORY', `(ptr)', `(CMPS)', `(const void *)')
COMB3(`ASSERT_ARRAY', `(char, int, uint)', `(CMPS)', `(const void *)')
COMB3(`ASSERT_ARRAY', `(ptr)', `(CMPS)', `(const void *)')
// clang-format on
