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

  // sets the timer to epoch
  static constexpr time_t reference_time = 1596252655;
  TEST_EXPECT(DateTime(DateTime::Construct().set_time("2020-8-1 3:30:55"))
                  .set_system_time()
                  .ctime() == reference_time);
  const auto date_time = DateTime().get_system_time();
  const auto now_time = time(nullptr);

  printer().key("ctime", NumberString(now_time));
  TEST_EXPECT(now_time == 1596252655);

  printer().object("dateTime", date_time);

  Date date(date_time);
  TEST_EXPECT(date.year() == 2020);
  TEST_EXPECT(date.month() == Date::Month::august);
  TEST_EXPECT(date.day() == 1);
  TEST_EXPECT(date.hour() == 3);
  TEST_EXPECT(date.minute() == 30);
  TEST_EXPECT(date.second() == 55);

  wait(1_seconds);

  const auto now_time_2 = time(nullptr);
  TEST_EXPECT(now_time_2 == now_time + 1);

  return case_result();
}

bool TimeTest::execute_class_timer_api_case() {
  test::Case test_case(this, "timer");

  Signal::Event event;

  Signal signal_user(Signal::Number::user1);
  Signal::HandlerScope handler_scope(signal_user,
                                     SignalHandler([](int a) -> void {
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
    api::ErrorScope error_scope;
    struct itimerspec input = {};
    TEST_EXPECT(timer_settime(timer_t(-1), 0, &input, nullptr) < 0 &&
                errno == EINVAL);
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
        printer().key(NumberString(signal_value),
                      "updated (ms)" | NumberString(duration));
        TEST_EXPECT(duration > 40 && duration < 70);
      }
    }
  }

  {
    Signal alarm_signal(Signal::Number::alarm);
    Signal::HandlerScope handler_scope(alarm_signal,
                                       SignalHandler([](int a) -> void {
                                         MCU_UNUSED_ARGUMENT(a);
                                         signal_value+=2;
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
        // set the alarm
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

    {
      test::Test::TimedScope timed_scope(*this, "alarmUpdateTimedScope",
                                         1000_milliseconds, 1200_milliseconds);
      // set the alarm
      signal_value = 0;
      Timer::alarm(Timer::Alarm()
                       .set_type(Timer::Alarm::Type::seconds)
                       .set_value(ClockTime(2_seconds)));

      Timer::alarm(Timer::Alarm()
                       .set_type(Timer::Alarm::Type::seconds)
                       .set_value(ClockTime(1_seconds)));

      wait(3_seconds);
      TEST_EXPECT(signal_value == 2);
    }
  }


  return case_result();
}
