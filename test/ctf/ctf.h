#ifndef TEST_CTF_H
#define TEST_CTF_H

#include <ctf/ctf.h>

CTF_GROUP_EXTERN(mock);
CTF_GROUP_EXTERN(mocked_add);
CTF_GROUP_EXTERN(mocked_strcmp);
CTF_GROUP_EXTERN(mocked_memcmp);
CTF_GROUP_EXTERN(primitive_success);
CTF_GROUP_EXTERN(array_success);
CTF_GROUP_EXTERN(memory_success);
CTF_GROUP_EXTERN(failure);
CTF_GROUP_EXTERN(mocked_add_failure);
CTF_GROUP_EXTERN(mocked_strcmp_failure);
CTF_GROUP_EXTERN(mocked_memcmp_failure);

#endif
