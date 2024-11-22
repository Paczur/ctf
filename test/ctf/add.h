#ifndef CTF_TEST_ADD_H
#define CTF_TEST_ADD_H
#include <stddef.h>

int wrapped_strcmp(const char *a, const char *b);
int wrapped_memcmp(const void *a, const void *b, size_t l);
int add(int a, int b);
int sub(int a, int b);

#endif
