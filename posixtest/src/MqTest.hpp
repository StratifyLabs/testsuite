// Copyright 2011-2021 Tyler Gilbert and Stratify Labs, Inc; see LICENSE.md

#ifndef MQTEST_HPP
#define MQTEST_HPP

#include <test/Test.hpp>


class MqTest : public test::Test
{
public:
  MqTest();

  bool execute_class_api_case();

};

#endif // MQTEST_HPP
