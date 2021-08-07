#include <chrono.hpp>
#include <sos.hpp>
#include <sys.hpp>
#include <test.hpp>
#include <thread.hpp>
#include <var.hpp>

#include "PThreadTest.hpp"

static pthread_mutex_t thread_mutex;

PThreadTest::PThreadTest() : Test("posix::pthread") {}

bool PThreadTest::execute_class_api_case() {
  execute_class_mutex_attributes_api_case();
  execute_class_mutex_api_case();
  execute_class_condition_attributes_api_case();
  execute_class_condition_api_case();
  execute_class_cancel_api_case();
  return true;
}

bool PThreadTest::try_lock_in_thread(thread::Mutex *mutex) {

  class Arguments {
    API_AF(Arguments, Mutex *, mutex, nullptr);
    API_AF(Arguments, test::Test *, test, nullptr);
    API_AB(Arguments, result, false);
  };

  Arguments args;
  args.set_mutex(mutex).set_test(this);
  Thread(Thread::Attributes().set_detach_state(Thread::DetachState::joinable),
         Thread::Construct().set_argument(&args).set_function(
             [](void *args) -> void * {
               auto *arguments = reinterpret_cast<Arguments *>(args);
               arguments->set_result(arguments->mutex()->try_lock());
               if (arguments->is_result()) {
                 arguments->mutex()->unlock();
               }
               return nullptr;
             }))
      .join();

  return args.is_result();
}

bool PThreadTest::execute_class_mutex_attributes_api_case() {
  test::Case tc(this, "mutexattributes");

  Mutex::Attributes attributes;
  TEST_EXPECT(attributes.set_type(Mutex::Type::normal).is_success());
  TEST_EXPECT(attributes.get_type() == Mutex::Type::normal);

  TEST_EXPECT(attributes.set_type(Mutex::Type::recursive).is_success());
  TEST_EXPECT(attributes.get_type() == Mutex::Type::recursive);

  {
    api::ErrorScope error_scope;
    attributes.set_type(Mutex::Type(-1));
    TEST_EXPECT(is_error() && error().error_number() == EINVAL);
  }

  TEST_EXPECT(
      attributes.set_protocol(Mutex::Protocol::priority_none).is_success());
  TEST_EXPECT(attributes.get_protocol() == Mutex::Protocol::priority_none);

  TEST_EXPECT(
      attributes.set_protocol(Mutex::Protocol::priority_inherit).is_success());
  TEST_EXPECT(attributes.get_protocol() == Mutex::Protocol::priority_inherit);

  TEST_EXPECT(
      attributes.set_protocol(Mutex::Protocol::priority_protect).is_success());
  TEST_EXPECT(attributes.get_protocol() == Mutex::Protocol::priority_protect);

  {
    api::ErrorScope error_scope;
    attributes.set_protocol(Mutex::Protocol(-1));
    TEST_EXPECT(is_error() && error().error_number() == EINVAL);
  }

  const auto max_priority = Sched::get_priority_max(Sched::Policy::round_robin);
  const auto min_priority = Sched::get_priority_min(Sched::Policy::other);
  for (int i = min_priority; i <= max_priority; i++) {
    TEST_EXPECT(attributes.set_priority_ceiling(i).is_success());
  }

  {
    api::ErrorScope error_scope;
    TEST_EXPECT(attributes.set_priority_ceiling(max_priority + 1).is_error() &&
                error().error_number() == EINVAL);
  }
  {
    api::ErrorScope error_scope;
    TEST_EXPECT(attributes.set_priority_ceiling(min_priority - 1).is_error() &&
                error().error_number() == EINVAL);
  }

  TEST_EXPECT(
      attributes.set_process_shared(Mutex::ProcessShared::shared).is_success());
  TEST_EXPECT(attributes.get_process_shared() == Mutex::ProcessShared::shared);

  TEST_EXPECT(attributes.set_process_shared(Mutex::ProcessShared::private_)
                  .is_success());
  TEST_EXPECT(attributes.get_process_shared() ==
              Mutex::ProcessShared::private_);

  {
    api::ErrorScope error_scope;
    TEST_EXPECT(
        attributes.set_process_shared(Mutex::ProcessShared(-1)).is_error() &&
        error().error_number() == EINVAL);
  }

  return case_result();
}

bool PThreadTest::execute_class_mutex_api_case() {
  test::Case tc(this, "mutex");

  auto test_mutex = [](PThreadTest *self, Mutex &mutex) {
    TEST_SELF_EXPECT(self->try_lock_in_thread(&mutex) == true);
    TEST_SELF_ASSERT(mutex.lock().is_success());
    TEST_SELF_EXPECT(self->try_lock_in_thread(&mutex) == false);
    TEST_SELF_ASSERT(mutex.unlock().is_success());
    TEST_SELF_EXPECT(self->try_lock_in_thread(&mutex) == true);
    return true;
  };

  {
    Mutex mutex;
    TEST_ASSERT(test_mutex(this, mutex));
  }

  {
    Mutex mutex(
        Mutex::Attributes().set_process_shared(Mutex::ProcessShared::private_));
    TEST_ASSERT(test_mutex(this, mutex));
  }

  {
    Mutex mutex(
        Mutex::Attributes().set_process_shared(Mutex::ProcessShared::shared));
    TEST_ASSERT(test_mutex(this, mutex));
  }

  {
    Mutex mutex(
        Mutex::Attributes().set_process_shared(Mutex::ProcessShared::shared));
    TEST_ASSERT(test_mutex(this, mutex));
  }

  {
    // const auto minimum_priority =
    // Sched::get_priority_min(Sched::Policy::other);
    const auto maximum_priority =
        Sched::get_priority_max(Sched::Policy::round_robin);
    const auto middle_priority = maximum_priority / 2;
    Mutex mutex(Mutex::Attributes().set_priority_ceiling(middle_priority));
    TEST_ASSERT(test_mutex(this, mutex));
    TEST_ASSERT(mutex.lock().is_success());
    // the elevated priority isn't available via POSIX
    // get it via /dev/sys
    const auto info = TaskManager("/dev/sys").get_info(Thread::self());
    TEST_EXPECT(info.priority() == middle_priority);
    mutex.unlock();
  }

  {
    Mutex mutex(Mutex::Attributes().set_type(Mutex::Type::recursive));
    TEST_ASSERT(test_mutex(this, mutex));
    int recursive_count = 0;
    {
      api::ErrorScope error_scope;
      do {
        mutex.lock();
        recursive_count++;
      } while (is_success() && recursive_count < 5000);
      printer().key("recursiveCount", NumberString(recursive_count - 1));
      TEST_EXPECT(recursive_count < 5000);
      TEST_EXPECT(is_error() && error().error_number() == EAGAIN);
    }

    TEST_EXPECT(try_lock_in_thread(&mutex) == false);
    {
      api::ErrorScope error_scope;
      for (int i = 0; i < recursive_count - 1; i++) {
        mutex.unlock_with_error_check();
      }
      TEST_EXPECT(is_success());
      TEST_EXPECT(mutex.unlock_with_error_check().is_error() &&
                  error().error_number() == EACCES);
    }
  }

  return case_result();
}

bool PThreadTest::execute_class_condition_attributes_api_case() {
  test::Case tc(this, "conditionattributes");

  {
    Cond::Attributes attributes;
    TEST_ASSERT(
        attributes.set_pshared(Cond::ProcessShared::shared).is_success());
    TEST_EXPECT(attributes.get_pshared() == Cond::ProcessShared::shared);
  }

  {
    Cond::Attributes attributes;
    TEST_ASSERT(
        attributes.set_pshared(Cond::ProcessShared::private_).is_success());
    TEST_EXPECT(attributes.get_pshared() == Cond::ProcessShared::private_);
  }

  {
    api::ErrorScope error_scope;
    Cond::Attributes attributes;
    TEST_ASSERT(attributes.set_pshared(Cond::ProcessShared(-1)).is_error() &&
                error().error_number() == EINVAL);
  }

  return case_result();
}
bool PThreadTest::execute_class_condition_api_case() {
  test::Case tc(this, "condition");

  struct Arguments {
    Arguments(PThreadTest *test) : cond(mutex), m_test(test) {}
    Mutex mutex;
    Cond cond;
    bool condition = true;

  private:
    API_AF(Arguments, PThreadTest *, test, nullptr);
  };

  {
    auto wait_for_condition = [](void *args) -> void * {
      auto *arguments = reinterpret_cast<Arguments *>(args);
      {
        // wait for broadcast
        Mutex::Scope ms(arguments->mutex);
        while (arguments->condition) {
          arguments->test()->printer().key("waitThread",
                                           NumberString(Thread::self()));
          arguments->cond.wait();
        }
      }
      {
        // wait for signal
        Mutex::Scope ms(arguments->mutex);
        while (arguments->condition == false) {
          arguments->test()->printer().key("waitThread",
                                           NumberString(Thread::self()));
          arguments->cond.wait();
        }
      }
      return nullptr;
    };

    // basic use case
    Arguments arguments(this);
    TEST_ASSERT(is_success());
    auto thread = Thread(Thread::Attributes().set_joinable(),
                         Thread::Construct()
                             .set_argument(&arguments)
                             .set_function(wait_for_condition));

    auto other_thread = Thread(Thread::Attributes().set_joinable(),
                               Thread::Construct()
                                   .set_argument(&arguments)
                                   .set_function(wait_for_condition));

    TEST_ASSERT(is_success());
    wait(250_milliseconds);
    {
      Mutex::Scope ms(arguments.mutex);
      arguments.condition = false;
      TEST_ASSERT(arguments.cond.broadcast().is_success());
    }

    // release one thread
    wait(100_milliseconds);
    {
      Mutex::Scope ms(arguments.mutex);
      arguments.condition = true;

      // releas one thread
      TEST_ASSERT(arguments.cond.signal().is_success());
      Sched().yield();

      // release the other thread
      TEST_ASSERT(arguments.cond.signal().is_success());
    }

    TEST_ASSERT(is_success());
    TEST_EXPECT(thread.join().is_success());
    TEST_EXPECT(other_thread.join().is_success());
  }

  return case_result();
}

bool PThreadTest::execute_class_cancel_api_case() {
  test::Case tc(this, "cancel");

  {
    auto thread = Thread(
        Thread::Attributes().set_detach_state(Thread::DetachState::joinable),
        Thread::Construct().set_argument(nullptr).set_function(
            [](void *) -> void * {
              while (1) {
                wait(100_milliseconds);
              }
              return nullptr;
            }));

    TEST_ASSERT(thread.cancel().join().is_success());
  }

  {
    TEST_EXPECT(Thread::set_cancel_type(Thread::CancelType::deferred) ==
                Thread::CancelType::deferred);
    auto thread = Thread(
        Thread::Attributes().set_detach_state(Thread::DetachState::detached),
        Thread::Construct().set_argument(nullptr).set_function(
            [](void *) -> void * {
              Thread::set_cancel_state(Thread::CancelState::disable);
              wait(500_milliseconds);
              Thread::set_cancel_state(Thread::CancelState::enable);
              while (1) {
                wait(1_milliseconds);
              }
              return nullptr;
            }));

    ClockTimer clock_timer(ClockTimer::IsRunning::yes);
    while (thread.is_running()) {
      thread.cancel();
      wait(10_milliseconds);
    }
    clock_timer.stop();
    const auto duration = clock_timer.milliseconds();
    TEST_EXPECT(duration > 500);
    printer().key("disabledDuration ms", NumberString(duration));
  }
  {
    api::ErrorScope error_scope;
    TEST_EXPECT(Thread::set_cancel_type(Thread::CancelType(-1)) ==
                    Thread::CancelType::deferred &&
                error().error_number() == EINVAL);
  }

  {
    api::ErrorScope error_scope;
    TEST_EXPECT(Thread::set_cancel_type(Thread::CancelType::asynchronous) ==
                    Thread::CancelType::deferred &&
                error().error_number() == ENOTSUP);
  }

  return case_result();
}

bool PThreadTest::execute_class_performance_case() { return true; }

bool PThreadTest::execute_class_stress_case() { return true; }
