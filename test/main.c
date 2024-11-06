#include <ctf/ctf.h>

#include "ctf/ctf.h"

int main(void) {
  ctf_parallel_start();
  ctf_groups_run(&mock, &primitive_success, &array_success, &memory_success);
  ctf_barrier();
  ctf_group_run(&failure);
  ctf_parallel_stop();
  return ctf_exit_code;
}
