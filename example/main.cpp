#include <iostream>
#include <pqrs/cf/run_loop_thread.hpp>

int main() {
  {
    pqrs::cf::run_loop_thread t;

    t.enqueue(^{
      std::cout << "hello" << std::endl;
    });

    t.terminate();
  }

  return 0;
}
