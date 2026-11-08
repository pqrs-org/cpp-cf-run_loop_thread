#import <Foundation/Foundation.h>

#include "TestAutoreleasePool.h"

#if __has_feature(objc_arc)
#error "TestAutoreleasePool.m must be compiled with -fno-objc-arc"
#endif

void* pqrs_cf_run_loop_thread_test_autorelease_pool_push(void) {
  return [[NSAutoreleasePool alloc] init];
}

void pqrs_cf_run_loop_thread_test_autorelease_pool_pop(void* token) {
  [(NSAutoreleasePool*)token drain];
}
