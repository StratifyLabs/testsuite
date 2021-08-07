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
  TEST_ASSERT_RESULT(execute_class_mutex_attributes_api_case());
  TEST_ASSERT_RESULT(execute_class_mutex_api_case());
  TEST_ASSERT_RESULT(execute_class_condition_attributes_api_case());
  TEST_ASSERT_RESULT(execute_class_condition_api_case());
  TEST_ASSERT_RESULT(execute_class_cancel_api_case());
  TEST_ASSERT_RESULT(execute_class_thread_attributes_api_case());
  TEST_ASSERT_RESULT(execute_class_thread_api_case());
  TEST_ASSERT_RESULT(execute_class_sem_api_case());
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

    ClockTimer clock_timer(ClockTimer::IsRunning::yes);
    {
      Mutex::Scope ms(mutex);
      Thread(
          Thread::Attributes().set_detach_state(Thread::DetachState::joinable),
          Thread::Construct().set_argument(&mutex).set_function(
              [](void *args) -> void * {
                auto *mutex = reinterpret_cast<Mutex *>(args);
                mutex->lock_timed(ClockTime(100_milliseconds));
                return nullptr;
              }))
          .join();
    }
    clock_timer.stop();
    printer().key("lockTimed (ms)", NumberString(clock_timer.milliseconds()));
    TEST_EXPECT(clock_timer.milliseconds() >= 100);
    TEST_EXPECT(clock_timer.milliseconds() < 500);
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
                // this calls usleep() which is a cancellation point
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
              wait(250_milliseconds);
              Thread::set_cancel_state(Thread::CancelState::enable);
              while (1) {
                wait(1_milliseconds);
              }
              return nullptr;
            }));

    ClockTimer clock_timer(ClockTimer::IsRunning::yes);
    wait(100_milliseconds);
    while (thread.is_running()) {
      thread.cancel();
      wait(10_milliseconds);
    }
    clock_timer.stop();
    const auto duration = clock_timer.milliseconds();
    TEST_EXPECT(duration > 250);
    TEST_EXPECT(duration < 500);
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

bool PThreadTest::execute_class_thread_attributes_api_case() {
  test::Case tc(this, "pthreadattributes");
  {
    Thread::Attributes attributes;
    {
      const auto list = std::initializer_list<Thread::DetachState>{
          Thread::DetachState::detached, Thread::DetachState::joinable};
      for (const auto value : list) {
        TEST_ASSERT(attributes.set_detach_state(value).is_success());
        TEST_EXPECT(attributes.get_detach_state() == value);
      }
    }

    {
      const auto list = std::initializer_list<Thread::IsInherit>{
          Thread::IsInherit::no, Thread::IsInherit::yes};
      for (const auto value : list) {
        TEST_ASSERT(attributes.set_inherit_sched(value).is_success());
        TEST_EXPECT(attributes.get_inherit_sched() == value);
      }
    }

    {
      const auto list = std::initializer_list<Sched::Policy>{
          Sched::Policy::other, Sched::Policy::round_robin,
          Sched::Policy::fifo};
      for (const auto value : list) {
        TEST_ASSERT(attributes.set_sched_policy(value).is_success());
        TEST_ASSERT(attributes.get_sched_policy() == value);
        const auto min = Sched::get_priority_min(value);
        const auto max = Sched::get_priority_max(value);
        for (int i = min; i <= max; i++) {
          TEST_ASSERT(attributes.set_sched_priority(i).is_success());
          TEST_ASSERT(attributes.get_sched_priority() == i);
        }
        {
          api::ErrorScope error_scope;
          TEST_ASSERT(attributes.set_sched_priority(max + 1).is_error() &&
                      error().error_number() == EINVAL);
        }

        {
          api::ErrorScope error_scope;
          TEST_ASSERT(attributes.set_sched_priority(min - 1).is_error() &&
                      error().error_number() == EINVAL);
        }
      }
    }
  }

  {
    api::ErrorScope error_scope;
    Thread::Attributes attributes;
    TEST_ASSERT(
        attributes.set_detach_state(Thread::DetachState(-1)).is_error() &&
        error().error_number() == EINVAL);
  }

  {
    api::ErrorScope error_scope;
    Thread::Attributes attributes;
    TEST_ASSERT(
        attributes.set_inherit_sched(Thread::IsInherit(-1)).is_error() &&
        error().error_number() == EINVAL);
  }

  {
    api::ErrorScope error_scope;
    Thread::Attributes attributes;
    TEST_ASSERT(attributes.set_sched_policy(Sched::Policy(-1)).is_error() &&
                error().error_number() == EINVAL);
  }

  return case_result();
}

bool PThreadTest::execute_class_thread_api_case() {
  test::Case tc(this, "pthread");
  {
    Thread(Thread::Attributes().set_joinable(),
           Thread::Construct().set_argument(nullptr).set_function(
               [](void *) -> void * { return nullptr; }))
        .join();
    TEST_ASSERT(is_success());
  }

  {
    auto thread = Thread(Thread::Attributes().set_detached(),
                         Thread::Construct().set_argument(nullptr).set_function(
                             [](void *) -> void * {
                               wait(100_milliseconds);
                               return nullptr;
                             }));
    TEST_EXPECT(pthread_join(thread.id(), nullptr) < 0 && errno == EINVAL);
    errno = 0;
    TEST_ASSERT(thread.is_joinable() == false);
    while (thread.is_running()) {
      wait(100_milliseconds);
    }
    TEST_ASSERT(is_success());
  }

  {
    const auto middle_priority =
        Sched::get_priority_max(Sched::Policy::round_robin) / 2;
    auto thread = Thread(Thread::Attributes()
                             .set_joinable()
                             .set_sched_policy(Sched::Policy::round_robin)
                             .set_sched_priority(middle_priority),
                         Thread::Construct().set_argument(nullptr).set_function(
                             [](void *) -> void * {
                               wait(100_milliseconds);
                               return nullptr;
                             }));

    TEST_EXPECT(thread.get_sched_policy() == Sched::Policy::round_robin);
    TEST_EXPECT(thread.get_sched_priority() == middle_priority);
    TEST_ASSERT(thread.join().is_success());
  }

  {
    const auto middle_priority =
        Sched::get_priority_max(Sched::Policy::fifo) / 2;
    auto thread = Thread(Thread::Attributes()
                             .set_joinable()
                             .set_sched_policy(Sched::Policy::fifo)
                             .set_sched_priority(middle_priority),
                         Thread::Construct().set_argument(nullptr).set_function(
                             [](void *) -> void * {
                               wait(100_milliseconds);
                               return nullptr;
                             }));

    TEST_EXPECT(thread.get_sched_policy() == Sched::Policy::fifo);
    TEST_EXPECT(thread.get_sched_priority() == middle_priority);
    TEST_ASSERT(thread.join().is_success());
  }

  {
    const auto middle_priority =
        Sched::get_priority_max(Sched::Policy::other) / 2;
    auto thread = Thread(Thread::Attributes()
                             .set_joinable()
                             .set_sched_policy(Sched::Policy::other)
                             .set_sched_priority(middle_priority),
                         Thread::Construct().set_argument(nullptr).set_function(
                             [](void *) -> void * {
                               wait(100_milliseconds);
                               return nullptr;
                             }));

    TEST_EXPECT(thread.get_sched_policy() == Sched::Policy::other);
    TEST_EXPECT(thread.get_sched_priority() == middle_priority);
    TEST_ASSERT(thread.join().is_success());
  }

  {
    const auto middle_priority =
        Sched::get_priority_max(Sched::Policy::round_robin) / 2;
    auto thread = Thread(Thread::Attributes()
                             .set_joinable()
                             .set_sched_policy(Sched::Policy::round_robin)
                             .set_sched_priority(middle_priority)
                             .set_inherit_sched(Thread::IsInherit::yes),
                         Thread::Construct().set_argument(nullptr).set_function(
                             [](void *) -> void * {
                               wait(100_milliseconds);
                               return nullptr;
                             }));

    // this will inherit from the caller - it will ignore round robin
    TEST_EXPECT(thread.get_sched_policy() == Sched::Policy::other);
    TEST_EXPECT(thread.get_sched_priority() ==
                Sched::get_priority_min(Sched::Policy::other));
    TEST_ASSERT(thread.join().is_success());
  }

  {
    auto thread =
        Thread(Thread::Attributes().set_stack_size(8192).set_joinable(),
               Thread::Construct().set_argument(nullptr).set_function(
                   [](void *) -> void * {
                     wait(100_milliseconds);
                     return nullptr;
                   }));

    // stack size is not available with regular POSIX calls
    const auto info = TaskManager("/dev/sys").get_info(thread.id());
    TEST_ASSERT(is_success());
    TEST_EXPECT(info.memory_size() == 8192);
    TEST_ASSERT(thread.join().is_success());
  }

  return case_result();
}

bool PThreadTest::execute_class_sem_api_case() {
  test::Case tc(this, "sem");

  {
    struct Arguments {
      Arguments() : semaphore(UnnamedSemaphore::ProcessShared::no, 5) {}
      UnnamedSemaphore semaphore;

    private:
      API_AF(Arguments, PThreadTest *, test, nullptr);
    };

    {
      Arguments arguments;

      arguments.semaphore.wait();
      TEST_EXPECT(arguments.semaphore.get_value() == 4);
      arguments.semaphore.wait();
      TEST_EXPECT(arguments.semaphore.get_value() == 3);
      arguments.semaphore.wait();
      TEST_EXPECT(arguments.semaphore.get_value() == 2);
      arguments.semaphore.wait();
      TEST_EXPECT(arguments.semaphore.get_value() == 1);
      arguments.semaphore.wait();
      TEST_EXPECT(arguments.semaphore.get_value() == 0);

      {
        ClockTimer clock_timer(ClockTimer::IsRunning::yes);
        {
          api::ErrorScope error_scope;
          TEST_EXPECT(
              arguments.semaphore.wait_timed(ClockTime(100_milliseconds))
                  .is_error() &&
              error().error_number() == ETIMEDOUT);
        }
        clock_timer.stop();
        printer().key("waitTimed (ms)",
                      NumberString(clock_timer.milliseconds()));
        TEST_EXPECT(clock_timer.milliseconds() >= 100);
        TEST_EXPECT(clock_timer.milliseconds() < 500);
      }

      {
        api::ErrorScope error_scope;
        TEST_EXPECT(arguments.semaphore.try_wait().is_error() &&
                    error().error_number() == EAGAIN);
      }

      auto thread = Thread(Thread::Attributes().set_detached(),
                           Thread::Construct()
                               .set_argument(&arguments)
                               .set_function([](void *args) -> void * {
                                 auto *arguments =
                                     reinterpret_cast<Arguments *>(args);
                                 arguments->semaphore.wait().post();
                                 return nullptr;
                               }));

      // let the thread have the semaphore so it can complete
      TEST_ASSERT(arguments.semaphore.post().is_success());

      while (thread.is_running()) {
        wait(10_milliseconds);
      }
    }
  }

  {
    Semaphore semaphore(5, Semaphore::IsExclusive::yes, "sem0");


    {
      api::ErrorScope error_scope;
      TEST_ASSERT(Semaphore(5, Semaphore::IsExclusive::yes, "sem0").is_error() && error().error_number() == EEXIST);
    }

    {
      TEST_ASSERT(Semaphore(5, Semaphore::IsExclusive::no, "sem0").is_success());
    }

    semaphore.wait();
    TEST_EXPECT(semaphore.get_value() == 4);
    semaphore.wait();
    TEST_EXPECT(semaphore.get_value() == 3);
    semaphore.wait();
    TEST_EXPECT(semaphore.get_value() == 2);
    semaphore.wait();
    TEST_EXPECT(semaphore.get_value() == 1);
    semaphore.wait();
    TEST_EXPECT(semaphore.get_value() == 0);

    {
      ClockTimer clock_timer(ClockTimer::IsRunning::yes);
      {
        api::ErrorScope error_scope;
        TEST_EXPECT(
            semaphore.wait_timed(ClockTime(100_milliseconds))
                .is_error() &&
            error().error_number() == ETIMEDOUT);
      }
      clock_timer.stop();
      printer().key("waitTimed (ms)",
                    NumberString(clock_timer.milliseconds()));
      TEST_EXPECT(clock_timer.milliseconds() >= 100);
      TEST_EXPECT(clock_timer.milliseconds() < 500);
    }

    {
      api::ErrorScope error_scope;
      TEST_EXPECT(semaphore.try_wait().is_error() &&
                  error().error_number() == EAGAIN);
    }

    {
      api::ErrorScope error_scope;
      TEST_EXPECT(Semaphore("sem1").is_error() &&
                  error().error_number() == ENOENT);
    }

    auto thread = Thread(Thread::Attributes().set_joinable(),
                         Thread::Construct()
                             .set_argument(nullptr)
                             .set_function([](void *) -> void * {
                               Semaphore("sem0").wait().post();
                               if( is_error() ){
                                 return (void*)-1;
                               }
                               return nullptr;
                             }));

    TEST_ASSERT(semaphore.post().is_success());
    TEST_ASSERT(semaphore.unlink().is_success());
    void * result = nullptr;
    TEST_ASSERT(thread.join(&result).is_success());
    TEST_ASSERT(result == nullptr);
  }

  return case_result();
}

bool PThreadTest::execute_class_performance_case() { return true; }

bool PThreadTest::execute_class_stress_case() { return true; }
