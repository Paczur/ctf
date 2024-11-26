#include <ctf/ctf.h>

#include "ctf/ctf.h"

int main(void) {
  ctf_parallel_start();
  ctf_groups_run(P_GROUP(mock), P_GROUP(mocked_add), P_GROUP(mocked_strcmp),
                 P_GROUP(mocked_memcmp), P_GROUP(primitive_success),
                 P_GROUP(array_success), P_GROUP(memory_success));
  ctf_barrier();
  ctf_groups_run(P_GROUP(failure), P_GROUP(mocked_add_failure),
                 P_GROUP(mocked_strcmp_failure),
                 P_GROUP(mocked_memcmp_failure));
  ctf_parallel_stop();
  return ctf_exit_code;
}
