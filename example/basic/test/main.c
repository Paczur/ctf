#include <ctf/ctf.h>

#include "module/module.h"

void ctf_main(int argc, char *argv[]) {
  (void)argc;
  (void)argv;
  ctf_group_run(add_tests);
}
