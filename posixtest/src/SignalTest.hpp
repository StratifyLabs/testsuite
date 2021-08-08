// Copyright 2011-2021 Tyler Gilbert and Stratify Labs, Inc; see LICENSE.md

#ifndef SIGNALTEST_HPP
#define SIGNALTEST_HPP

#include "test/Test.hpp"

class SignalTest : public test::Test
{
public:
  SignalTest();

  bool execute_class_api_case();

private:
  static int signal_value;

};

#endif // SIGNALTEST_HPP
