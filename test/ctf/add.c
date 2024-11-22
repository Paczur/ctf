#include "add.h"

#include <string.h>

int wrapped_strcmp(const char *a, const char *b) { return strcmp(a, b); }
int wrapped_memcmp(const void *a, const void *b, size_t l) {
  return memcmp(a, b, l);
}
int add(int a, int b) { return a + b; }
int sub(int a, int b) { return a - b; }
