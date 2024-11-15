#include <ctf/ctf.h>

#include "ctf/ctf.h"

int main(void) {
  ctf_parallel_start();
  ctf_groups_run(CTF_GROUP_P(mock), CTF_GROUP_P(primitive_success),
                 CTF_GROUP_P(array_success), CTF_GROUP_P(memory_success));
  ctf_barrier();
  ctf_group_run(failure);
  ctf_parallel_stop();
  return ctf_exit_code;
}
