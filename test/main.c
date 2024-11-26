#include <ctf/ctf.h>

#include "ctf/ctf.h"

int main(void) {
  ctf_parallel_start();
  ctf_groups_run(mock, mocked_add, mocked_strcmp, mocked_memcmp,
                 primitive_success, array_success, memory_success);
  ctf_barrier();
  ctf_groups_run(failure, mocked_add_failure, mocked_strcmp_failure,
                 mocked_memcmp_failure);
  ctf_parallel_stop();
  return ctf_exit_code;
}
