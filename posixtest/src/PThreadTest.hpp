#ifndef PTHREADTEST_HPP
#define PTHREADTEST_HPP

#include <test/Test.hpp>
#include <thread/Mutex.hpp>

class PThreadTest : public test::Test {
public:
    PThreadTest();

    bool execute_class_api_case();
    bool execute_class_performance_case();
    bool execute_class_stress_case();

private:

  bool execute_class_mutex_attributes_api_case();
  bool execute_class_mutex_api_case();
  bool execute_class_condition_attributes_api_case();
  bool execute_class_condition_api_case();
  bool execute_class_cancel_api_case();

  bool try_lock_in_thread(thread::Mutex * mutex);
};

#endif // PTHREADTEST_HPP
