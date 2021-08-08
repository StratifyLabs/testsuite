#ifndef TIMETEST_HPP
#define TIMETEST_HPP

#include <test/Test.hpp>

class TimeTest : public test::Test
{
public:
  TimeTest();

  bool execute_class_api_case();

private:

  static int signal_value;

  bool execute_class_timeofday_api_case();
  bool execute_class_timer_api_case();

};

#endif // TIMETEST_HPP
