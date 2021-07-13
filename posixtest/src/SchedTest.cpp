#include "SchedTest.hpp"

#include <chrono.hpp>
#include <sched.h>
#include <unistd.h>
#include <var.hpp>

SchedTest::SchedTest() : Test("posix::sched") {}

bool SchedTest::execute_class_api_case() {
  bool result = true;
  struct sched_param param;
  int max_prio;
  int min_prio;
  int i;

  TEST_EXPECT(sched_getparam(getpid(), &param) == 0);
  TEST_EXPECT(param.sched_priority == 0);

  for (int i = 5000; i < 50000; i++) {
    TEST_EXPECT(sched_getparam(i, &param) < 0);
    TEST_EXPECT(errno == ESRCH);
  }

  for (int i = -5000; i < 0; i++) {
    TEST_EXPECT(sched_getparam(i, &param) < 0);
    TEST_EXPECT(errno == ESRCH);
  }

  min_prio = sched_get_priority_min(SCHED_FIFO);
  max_prio = sched_get_priority_max(SCHED_FIFO);
  param.sched_priority = min_prio - 1;
  TEST_EXPECT(sched_setscheduler(getpid(), SCHED_FIFO, &param) < 0);
  TEST_EXPECT(errno == EINVAL);

  for (i = min_prio; i <= max_prio; i++) {
    wait(Microseconds(1000));
    param.sched_priority = i;
    TEST_EXPECT(sched_setscheduler(getpid(), SCHED_FIFO, &param) == 0);
  }
  param.sched_priority = max_prio + 1;
  TEST_EXPECT(sched_setscheduler(getpid(), SCHED_FIFO, &param) < 0);
  TEST_EXPECT(errno == EINVAL);

  min_prio = sched_get_priority_min(SCHED_RR);
  max_prio = sched_get_priority_max(SCHED_RR);
  param.sched_priority = min_prio - 1;
  TEST_EXPECT(sched_setscheduler(getpid(), SCHED_RR, &param) < 0);
  TEST_EXPECT(errno == EINVAL);
  for (i = min_prio; i <= max_prio; i++) {
    wait(Microseconds(1000));
    param.sched_priority = i;
    TEST_EXPECT(sched_setscheduler(getpid(), SCHED_RR, &param) == 0);
  }
  param.sched_priority = max_prio + 1;
  TEST_EXPECT(sched_setscheduler(getpid(), SCHED_RR, &param) < 0);
  TEST_EXPECT(errno == EINVAL);

  min_prio = sched_get_priority_min(SCHED_OTHER);
  max_prio = sched_get_priority_max(SCHED_OTHER);
  param.sched_priority = min_prio - 1;
  TEST_EXPECT(sched_setscheduler(getpid(), SCHED_OTHER, &param) < 0);
  TEST_EXPECT(errno == EINVAL);

  for (i = min_prio; i <= max_prio; i++) {
    wait(Microseconds(1000));
    param.sched_priority = i;
    TEST_EXPECT(sched_setscheduler(getpid(), SCHED_OTHER, &param) == 0);
  }
  param.sched_priority = max_prio + 1;
  TEST_EXPECT(sched_setscheduler(getpid(), SCHED_OTHER, &param) < 0);
  TEST_EXPECT(errno == EINVAL);
  {

    TEST_EXPECT(sched_setparam(-1, &param) < 0);
    TEST_EXPECT(errno == ESRCH);

    param.sched_priority = 100;
    TEST_EXPECT(sched_setparam(getpid(), &param) < 0);
    TEST_EXPECT(errno == EINVAL);

    param.sched_priority = 0;
    TEST_EXPECT(sched_setparam(getpid(), &param) == 0);
  }

  {
    min_prio = sched_get_priority_min(SCHED_FIFO);
    max_prio = sched_get_priority_max(SCHED_FIFO);
    for (i = min_prio; i <= max_prio; i++) {
      wait(Microseconds(1000));
      param.sched_priority = i;
      TEST_EXPECT(sched_setscheduler(getpid(), SCHED_FIFO, &param) == 0);
    }

    min_prio = sched_get_priority_min(SCHED_FIFO);
    max_prio = sched_get_priority_max(SCHED_FIFO);
    for (i = min_prio; i <= max_prio; i++) {
      wait(Microseconds(1000));
      param.sched_priority = i;
      TEST_EXPECT(sched_setscheduler(getpid(), SCHED_RR, &param) == 0);
    }

    min_prio = sched_get_priority_min(SCHED_OTHER);
    max_prio = sched_get_priority_max(SCHED_OTHER);
    for (i = min_prio; i <= max_prio; i++) {
      wait(Microseconds(1000));
      param.sched_priority = i;
      TEST_EXPECT(sched_setscheduler(getpid(), SCHED_OTHER, &param) == 0);
    }
  }

  return result;
}
