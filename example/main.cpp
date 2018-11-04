#include <iostream>
#include <pqrs/cf_run_loop_thread.hpp>

int main(void) {
  {
    pqrs::cf_run_loop_thread t;

    CFRunLoopPerformBlock(t.get_run_loop(),
                          kCFRunLoopCommonModes,
                          ^{
                            std::cout << "hello" << std::endl;
                          });

    CFRunLoopWakeUp(t.get_run_loop());

    t.terminate();
  }

  return 0;
}
