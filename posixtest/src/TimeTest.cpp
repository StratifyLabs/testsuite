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
    if (is_error() && error().error_number() == ENOTSUP) {
      printer().info("timers not supported -- not testing");
      return true;
    }
  }

  {
    Timer timer(event);
    signal_value = 0;
    {
      test::Test::TimedScope timed_scope(*this, "timerTimedScope",
                                         50_milliseconds, 70_milliseconds);
      timer.set_time(Timer::SetTime().set_value(ClockTime(50_milliseconds)));
      u32 i = 0;
      while ((signal_value == 0) && (i++ < 10)) {
        wait(10_milliseconds);
      }
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
    auto last_time = case_timer().milliseconds();
    while (signal_value < 5) {
      wait(10_milliseconds);
      if (signal_value != last_value) {
        last_value = signal_value;
        const auto now = case_timer().milliseconds();
        const auto duration = now - last_time;
        last_time = now;
        printer().key(NumberString(signal_value), "updated (ms)" | NumberString(duration));
        TEST_EXPECT(duration > 40 && duration < 70);
      }
    }
  }

  {
    Signal alarm_signal(Signal::Number::alarm);
    alarm_signal.set_handler(SignalHandler([](int a) {
      MCU_UNUSED_ARGUMENT(a);
      signal_value += 2;
    }));

    {
      printer::Printer::Object po(printer(), "alarm");
      // alarm uses the same mechanisms as Timer
      signal_value = 0;

      {
        test::Test::TimedScope timed_scope(
            *this, "alarmTimedScope", 1000_milliseconds, 1200_milliseconds);
        Timer::alarm(Timer::Alarm()
                         .set_type(Timer::Alarm::Type::seconds)
                         .set_value(ClockTime(1_seconds)));
        wait(5_seconds);
      }
      printer().key("signalValue", NumberString(signal_value));
      TEST_EXPECT(signal_value == 2);
    }

    {
      // alarm uses the same mechanisms as Timer
      printer::Printer::Object po(printer(), "abortAlarm");
      signal_value = 0;
      {
        test::Test::TimedScope timed_scope(*this, "alarmAbortTimedScope",
                                           2000_milliseconds,
                                           2200_milliseconds);
        //set the alarm
        Timer::alarm(Timer::Alarm()
                         .set_type(Timer::Alarm::Type::seconds)
                         .set_value(ClockTime(1_seconds)));
        // cancel the alarm
        Timer::cancel_alarm();

        wait(2_seconds);
      }

      printer().key("signalValue", NumberString(signal_value));
      TEST_EXPECT(signal_value == 0);
    }
    alarm_signal.set_handler(SignalHandler::default_());
  }

  signal_user.set_handler(SignalHandler::default_());

  return case_result();
}
