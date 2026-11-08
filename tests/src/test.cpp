#include "TestAutoreleasePool.h"
#include <atomic>
#include <boost/ut.hpp>
#include <list>
#include <pqrs/cf/run_loop_thread.hpp>
#include <thread>
#include <vector>

namespace {
using namespace boost::ut;
using namespace boost::ut::literals;

class run_loop_thread_test final {
public:
  run_loop_thread_test() {
    for (int i = 0; i < 10; ++i) {
      count1_ = 0;
      count2_ = 0;

      wait1_ = pqrs::make_thread_wait();
      wait2_ = pqrs::make_thread_wait();

      thread1_ = std::make_shared<pqrs::cf::run_loop_thread>();
      thread2_ = std::make_shared<pqrs::cf::run_loop_thread>();

      // thread1 (loop)

      for (int j = 0; j < 5; ++j) {
        thread1_->enqueue(^{
          std::cout << "." << std::flush;
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

    std::cout << std::endl;
  }

private:
  void enqueue2() {
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

  int count1_{0};
  int count2_{0};

  std::shared_ptr<pqrs::thread_wait> wait1_;
  std::shared_ptr<pqrs::thread_wait> wait2_;
};
} // namespace

int main() {
  "loop_thread"_test = [] {
    run_loop_thread_test();
  };

  "loop_thread (multiple instances)"_test = [] {
    std::list<pqrs::not_null_shared_ptr_t<pqrs::cf::run_loop_thread>> run_loop_threads;
    for (int i = 0; i < 100; ++i) {
      auto t = std::make_shared<pqrs::cf::run_loop_thread>();

      t->enqueue(^{
        std::cout << "." << std::flush;
      });

      run_loop_threads.push_back(t);

      while (run_loop_threads.size() > 10) {
        auto t = run_loop_threads.front();
        t->terminate();

        run_loop_threads.pop_front();
      }
    }

    for (auto&& t : run_loop_threads) {
      t->terminate();
    }

    run_loop_threads.clear();

    std::cout << std::endl;
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

  "add_source and remove_source"_test = [] {
    auto run_loop_thread = std::make_shared<pqrs::cf::run_loop_thread>();
    auto wait1 = pqrs::make_thread_wait();
    auto wait2 = pqrs::make_thread_wait();
    std::atomic<int> count{0};
    struct source_context final {
      std::atomic<int>* count;
      pqrs::not_null_shared_ptr_t<pqrs::thread_wait> wait;
    } source_context{
        .count = &count,
        .wait = wait1,
    };

    auto context = CFRunLoopSourceContext();
    context.info = &source_context;
    context.perform = [](void* _Nullable info) {
      auto source_context = reinterpret_cast<struct source_context*>(info);
      if (source_context->count->fetch_add(1) == 0) {
        source_context->wait->notify();
      }
    };

    auto source = pqrs::cf::adopt_cf_ptr(CFRunLoopSourceCreate(kCFAllocatorDefault,
                                                               0,
                                                               &context));

    run_loop_thread->add_source(*source);
    CFRunLoopSourceSignal(*source);
    run_loop_thread->wake();
    wait1->wait_notice();

    expect(count.load() == 1_i);

    run_loop_thread->remove_source(*source);
    CFRunLoopSourceSignal(*source);
    run_loop_thread->wake();
    run_loop_thread->enqueue(^{
      wait2->notify();
    });
    wait2->wait_notice();

    expect(count.load() == 1_i);

    run_loop_thread->terminate();
  };

  "execution_wrapper"_test = [] {
    std::atomic<bool> inside_wrapper{false};
    std::atomic<bool> block_was_inside_wrapper{false};
    std::atomic<bool> source_was_inside_wrapper{false};
    std::atomic<size_t> wrapper_count{0};
    std::atomic<size_t> processed_count{0};
    auto processed_wait = pqrs::make_thread_wait();

    auto run_loop_thread = std::make_shared<pqrs::cf::run_loop_thread>(
        pqrs::cf::run_loop_thread::failure_policy::abort,
        [&](const std::function<void()>& function) {
          inside_wrapper = true;
          function();
          inside_wrapper = false;
          ++wrapper_count;

          if (processed_count.load() == 2) {
            processed_wait->notify();
          }
        });

    struct source_context final {
      std::atomic<bool>* inside_wrapper;
      std::atomic<bool>* source_was_inside_wrapper;
      std::atomic<size_t>* processed_count;
    } source_context{
        .inside_wrapper = &inside_wrapper,
        .source_was_inside_wrapper = &source_was_inside_wrapper,
        .processed_count = &processed_count,
    };

    auto context = CFRunLoopSourceContext();
    context.info = &source_context;
    context.perform = [](void* _Nullable info) {
      auto source_context = reinterpret_cast<struct source_context*>(info);
      source_context->source_was_inside_wrapper->store(source_context->inside_wrapper->load());
      ++(*source_context->processed_count);
    };

    auto source = pqrs::cf::adopt_cf_ptr(CFRunLoopSourceCreate(kCFAllocatorDefault,
                                                               0,
                                                               &context));

    auto inside_wrapper_ptr = &inside_wrapper;
    auto block_was_inside_wrapper_ptr = &block_was_inside_wrapper;
    auto processed_count_ptr = &processed_count;

    run_loop_thread->add_source(*source);
    run_loop_thread->enqueue(^{
      block_was_inside_wrapper_ptr->store(inside_wrapper_ptr->load());
      ++(*processed_count_ptr);
    });
    CFRunLoopSourceSignal(*source);
    run_loop_thread->wake();

    processed_wait->wait_notice();

    expect(block_was_inside_wrapper.load());
    expect(source_was_inside_wrapper.load());

    run_loop_thread->remove_source(*source);
    run_loop_thread->terminate();

    expect(!inside_wrapper.load());
    expect(wrapper_count.load() > 0);
  };

  "autorelease_pool_execution_wrapper"_test = [] {
    auto run_loop_thread = std::make_shared<pqrs::cf::run_loop_thread>(
        pqrs::cf::run_loop_thread::failure_policy::abort,
        [](const std::function<void()>& function) {
          auto token = pqrs_cf_run_loop_thread_test_autorelease_pool_push();
          function();
          pqrs_cf_run_loop_thread_test_autorelease_pool_pop(token);
        });

    auto wait = pqrs::make_thread_wait();
    std::atomic<int> count{0};
    auto wait_ptr = wait.get();
    auto count_ptr = &count;

    for (int i = 0; i < 10; ++i) {
      run_loop_thread->enqueue(^{
        if (count_ptr->fetch_add(1) == 9) {
          wait_ptr->notify();
        }
      });
    }

    wait->wait_notice();
    run_loop_thread->terminate();

    expect(count == 10);
  };

  "execution_wrapper_is_repeated_after_timeout"_test = [] {
    std::atomic<size_t> wrapper_count{0};
    auto wrapper_count_wait = pqrs::make_thread_wait();

    auto run_loop_thread = std::make_shared<pqrs::cf::run_loop_thread>(
        pqrs::cf::run_loop_thread::failure_policy::abort,
        [&](const std::function<void()>& function) {
          function();

          if (wrapper_count.fetch_add(1) == 1) {
            wrapper_count_wait->notify();
          }
        });

    wrapper_count_wait->wait_notice();
    run_loop_thread->terminate();

    expect(wrapper_count.load() >= 2);
  };

  "concurrent enqueue"_test = [] {
    constexpr auto thread_count = 4;
    constexpr auto enqueue_count = 25;
    constexpr auto total_count = thread_count * enqueue_count;

    auto run_loop_thread = std::make_shared<pqrs::cf::run_loop_thread>();
    auto wait = pqrs::make_thread_wait();
    std::atomic<int> count{0};

    std::vector<std::thread> threads;
    threads.reserve(thread_count);

    for (int i = 0; i < thread_count; ++i) {
      threads.emplace_back([&] {
        for (int j = 0; j < enqueue_count; ++j) {
          run_loop_thread->enqueue(^{
            if (count.fetch_add(1) == total_count - 1) {
              wait->notify();
            }
          });
        }
      });
    }

    for (auto&& thread : threads) {
      thread.join();
    }

    wait->wait_notice();
    run_loop_thread->terminate();

    expect(count == total_count);
  };

  "concurrent terminate"_test = [] {
    auto run_loop_thread = std::make_shared<pqrs::cf::run_loop_thread>();

    std::vector<std::thread> threads;
    threads.reserve(8);

    for (int i = 0; i < 8; ++i) {
      threads.emplace_back([run_loop_thread] {
        run_loop_thread->terminate();
      });
    }

    for (auto&& thread : threads) {
      thread.join();
    }

    run_loop_thread->terminate();
    run_loop_thread->wake();
  };

  "shared_run_loop_thread"_test = [] {
    expect(!pqrs::cf::run_loop_thread::extra::get_shared_run_loop_thread());

    pqrs::cf::run_loop_thread::extra::initialize_shared_run_loop_thread();

    int __block count = 0;

    pqrs::cf::run_loop_thread::extra::get_shared_run_loop_thread()->enqueue(^{
      ++count;
    });

    pqrs::cf::run_loop_thread::extra::terminate_shared_run_loop_thread();

    expect(count == 1_i);
    expect(!pqrs::cf::run_loop_thread::extra::get_shared_run_loop_thread());

    pqrs::cf::run_loop_thread::extra::initialize_shared_run_loop_thread();
    expect(pqrs::cf::run_loop_thread::extra::get_shared_run_loop_thread());
    pqrs::cf::run_loop_thread::extra::terminate_shared_run_loop_thread();

    std::atomic<bool> inside_wrapper{false};
    std::atomic<bool> block_was_inside_wrapper{false};

    pqrs::cf::run_loop_thread::extra::initialize_shared_run_loop_thread(
        pqrs::cf::run_loop_thread::failure_policy::abort,
        [&](const std::function<void()>& function) {
          inside_wrapper = true;
          function();
          inside_wrapper = false;
        });

    auto inside_wrapper_ptr = &inside_wrapper;
    auto block_was_inside_wrapper_ptr = &block_was_inside_wrapper;
    pqrs::cf::run_loop_thread::extra::get_shared_run_loop_thread()->enqueue(^{
      block_was_inside_wrapper_ptr->store(inside_wrapper_ptr->load());
    });

    pqrs::cf::run_loop_thread::extra::terminate_shared_run_loop_thread();

    expect(block_was_inside_wrapper.load());
    expect(!inside_wrapper.load());
    expect(!pqrs::cf::run_loop_thread::extra::get_shared_run_loop_thread());
  };
}
