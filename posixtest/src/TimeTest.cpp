#include <chrono.hpp>
#include <test/Case.hpp>
#include <thread.hpp>
#include <var.hpp>

#include "TimeTest.hpp"

int TimeTest::signal_value = 0;

TimeTest::TimeTest() : test::Test("posix::time") {}

bool TimeTest::execute_class_api_case() {
  TEST_ASSERT_RESULT(execute_class_timeofday_api_case());
  TEST_ASSERT_RESULT(execute_class_timer_api_case());
  return case_result();
}

bool TimeTest::execute_class_timeofday_api_case() {
  test::Case test_case(this, "timeofday");

  return case_result();
}

bool TimeTest::execute_class_timer_api_case() {
  test::Case test_case(this, "timer");

  Signal::Event event;

  Signal signal_user = Signal(Signal::Number::user1)
                           .set_handler(SignalHandler([](int a) -> void {
                             MCU_UNUSED_ARGUMENT(a);
                             signal_value++;
                           }));

  event.set_notify(Signal::Event::Notify::signal)
      .set_value(2)
      .set_number(signal_user.number());

  {
    api::ErrorScope error_scope;
    Timer timer(event);
    if( is_error() && error().error_number() == ENOTSUP ){
      printer().info("timers not supported -- not testing");
      return true;
    }
  }

  {
    Timer timer(event);
    signal_value = 0;
    timer.set_time(Timer::SetTime().set_value(ClockTime(50_milliseconds)));
    for(u32 i=0; i < 5; i++){
      wait(50_milliseconds);
    }
    TEST_ASSERT(signal_value == 1);
  }

  {
    Timer timer(event);
    signal_value = 0;
    timer.set_time(Timer::SetTime()
                       .set_value(ClockTime(50_milliseconds))
                       .set_interval(ClockTime(50_milliseconds)));
    auto last_value = signal_value;
    while (signal_value < 5) {
      wait(10_milliseconds);
      if (signal_value != last_value) {
        printer().key(NumberString(signal_value), "updated");
        last_value = signal_value;
      }
    }
  }

  signal_user.set_handler(SignalHandler::default_());

  return case_result();
}
