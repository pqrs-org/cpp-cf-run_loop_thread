#include <boost/ut.hpp>
#include <pqrs/cf/run_loop_thread.hpp>

namespace {
using namespace boost::ut;
using namespace boost::ut::literals;

class run_loop_thread_test final {
public:
  run_loop_thread_test(void) : count1_(0),
                               count2_(0) {
    for (int i = 0; i < 5000; ++i) {
      count1_ = 0;
      count2_ = 0;

      wait1_ = pqrs::make_thread_wait();
      wait2_ = pqrs::make_thread_wait();

      thread1_ = std::make_shared<pqrs::cf::run_loop_thread>();
      thread2_ = std::make_shared<pqrs::cf::run_loop_thread>();

      // thread1 (loop)

      for (int j = 0; j < 5; ++j) {
        thread1_->enqueue(^{
          ++count1_;
        });
      }
      thread1_->enqueue(^{
        wait1_->notify();
      });

      // thread2 (recursive)

      enqueue2();

      // Verify counts

      wait1_->wait_notice();
      wait2_->wait_notice();

      thread1_->terminate();
      thread1_ = nullptr;
      thread2_->terminate();
      thread2_ = nullptr;

      expect(count1_ == 5);
      expect(count2_ == 3);
    }
  }

private:
  void enqueue2(void) {
    thread2_->enqueue(^{
      ++count2_;
      if (count2_ < 3) {
        enqueue2();
      } else {
        wait2_->notify();
      }
    });
  }

private:
  std::shared_ptr<pqrs::cf::run_loop_thread> thread1_;
  std::shared_ptr<pqrs::cf::run_loop_thread> thread2_;

  int count1_;
  int count2_;

  std::shared_ptr<pqrs::thread_wait> wait1_;
  std::shared_ptr<pqrs::thread_wait> wait2_;
};
} // namespace

int main(void) {
  "loop_thread"_test = [] {
    run_loop_thread_test();
  };

  "call methods after terminate"_test = [] {
    auto run_loop_thread = std::make_shared<pqrs::cf::run_loop_thread>();
    int __block count = 0;

    run_loop_thread->enqueue(^{
      ++count;
    });

    run_loop_thread->terminate();

    expect(count == 1_i);

    run_loop_thread->wake();

    run_loop_thread->enqueue(^{
      std::cout << "never reached" << std::endl;
    });
  };
}
