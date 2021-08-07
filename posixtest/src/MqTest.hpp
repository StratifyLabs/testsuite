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
