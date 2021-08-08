#include <chrono.hpp>
#include <fs.hpp>
#include <sos.hpp>
#include <test.hpp>
#include <thread.hpp>
#include <var.hpp>

#include <sys/wait.h>

#include "SignalTest.hpp"

int SignalTest::signal_value = 0;

SignalTest::SignalTest() : Test("posix::signal") {}

bool SignalTest::execute_class_api_case() {

  auto user1_function = [](int) { SignalTest::signal_value++; };

  {
    // self signalling
    Signal signal(Signal::Number::user1);

    signal.set_handler(SignalHandler(
        SignalHandler::Construct().set_signal_function(user1_function)));

    SignalTest::signal_value = 0;
    signal.send(Sched::get_pid());
    TEST_EXPECT(SignalTest::signal_value == 1);

    // this will stack multiple functions --verified with sos_debug_printf()
    Thread(
        Thread::Attributes()
            .set_joinable()
            .set_sched_policy(Sched::Policy::fifo)
            .set_sched_priority(25),
        Thread::Construct().set_function([](void *) -> void * {
          const auto pid = Sched::get_pid();
          Signal(Signal::Number::user1).send(pid).send(pid).send(pid).send(pid);
          return nullptr;
        }))
        .join();

    TEST_EXPECT(SignalTest::signal_value == 5);

    // this may stack one at a time
    Thread(
        Thread::Attributes().set_joinable(),
        Thread::Construct().set_function([](void *) -> void * {
          const auto pid = Sched::get_pid();
          Signal(Signal::Number::user1).send(pid).send(pid).send(pid).send(pid);
          return nullptr;
        }))
        .join();

    TEST_EXPECT(SignalTest::signal_value == 9);

    // this will stack one at a time and allow it to be handled
    Thread(Thread::Attributes().set_joinable(),
           Thread::Construct().set_function([](void *) -> void * {
             const auto pid = Sched::get_pid();
             Signal(Signal::Number::user1).send(pid);
             Sched().yield();
             Signal(Signal::Number::user1).send(pid);
             Sched().yield();
             Signal(Signal::Number::user1).send(pid);
             Sched().yield();
             Signal(Signal::Number::user1).send(pid);
             Sched().yield();
             return nullptr;
           }))
        .join();

    TEST_EXPECT(SignalTest::signal_value == 13);
  }

  {

    constexpr auto signal_process_path = "/app/flash/signalprocess";
    const bool is_process_available = FileSystem().exists(signal_process_path);
    printer().key_bool("isSignalProcessAvailable", is_process_available);

    {
      ClockTimer clock_timer(ClockTimer::IsRunning::yes);
      const auto path = Sos().launch(Sos::Launch()
                                         .set_path(signal_process_path)
                                         .set_arguments("--wait=200"));
      int pid = return_value();
      printer().key("path", path.cstring()).key("pid", NumberString(pid));

      TEST_ASSERT(is_success());

      const auto status = Sos().wait_pid().child_status();
      TEST_ASSERT(is_success());
      const auto duration = clock_timer.stop().milliseconds();
      TEST_EXPECT(duration >= 200);
      TEST_EXPECT(duration < 300);
      printer().key("returnValue", NumberString(status >> 8));
      TEST_EXPECT((status >> 8) == 200);
    }

    {
      const auto path = Sos().launch(
          Sos::Launch().set_path(signal_process_path).set_arguments("--user"));
      int pid = return_value();
      printer().key("path", path.cstring()).key("pid", NumberString(pid));

      TEST_ASSERT(is_success());

      // child sends USER1 which could cause wait() to return errno EINTR
      int status;
      int rv;
      do {
        api::ErrorScope error_scope;
        status = Sos().wait_pid().child_status();
        rv = return_value();
      } while(rv < 0);

      printer().key("returnValue", NumberString(status >> 8));
      TEST_EXPECT((status >> 8) == 190);
      // TEST_EXPECT(SignalTest::signal_value == 14);
    }

    {
      const auto path = Sos().launch(Sos::Launch()
                                         .set_path(signal_process_path)
                                         .set_arguments("--infinite"));
      int pid = return_value();
      printer().key("path", path.cstring()).key("pid", NumberString(pid));

      Signal(Signal::Number::terminate).send(pid);
      TEST_ASSERT(is_success());
      const auto status = Sos().wait_pid().child_status();
      TEST_ASSERT(is_success());
      TEST_EXPECT(status == SIGTERM);
      printer().key("returnValue", NumberString(status, "0x%08x"));
    }
  }

  return case_result();
}
