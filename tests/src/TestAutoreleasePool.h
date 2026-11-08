#pragma once

#ifdef __cplusplus
extern "C" {
#endif

void* _Nonnull pqrs_cf_run_loop_thread_test_autorelease_pool_push(void);
void pqrs_cf_run_loop_thread_test_autorelease_pool_pop(void* _Nonnull token);

#ifdef __cplusplus
}
#endif
