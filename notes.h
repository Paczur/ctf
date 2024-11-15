#define CTF_MOCK_BEGIN(return, name, ...)                                 \
  struct ctf_internal_mock_data                                           \
    ctf_internal_mock_data_##name[CTF_CONST_MAX_THREADS];                 \
  return __real_##name(__VA_ARGS__);                                      \
  return __wrap_##name(__VA_ARGS__) {                                     \
    return (*const real)(__VA_ARGS__) = __real_##name;                    \
    struct ctf_internal_mock_data *data =                                 \
      ctf_internal_mock_data_##name + ctf_internal_parallel_thread_index; \
    return (*const mock)(__VA_ARGS__) = data->mock_f;
#define CTF_MOCK_CALL_ARGS(...) \
  if(mock == NULL) {            \
    return real(__VA_ARGS__);   \
  } else {                      \
    data->call_count++;         \
    return mock(__VA_ARGS__);   \
  }
#define CTF_MOCK_END }

void *ctf_internal_mock_group[] = {
  ctf_internal_mock_data_add,
  mock_add,
};
