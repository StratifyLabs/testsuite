#include <sys.hpp>
#include <var.hpp>
#include <thread.hpp>

#include "PThreadTest.hpp"

static pthread_mutex_t thread_mutex;

PThreadTest::PThreadTest() : Test("posix::pthread") { m_count = 0; }

bool PThreadTest::execute_class_api_case() {
  return true;
}



bool PThreadTest::execute_class_performance_case(){
  return true;
}

bool PThreadTest::execute_class_stress_case(){
  return true;
}
