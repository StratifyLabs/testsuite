// Copyright 2011-2021 Tyler Gilbert and Stratify Labs, Inc; see LICENSE.md

#include <sys/Cli.hpp>
#include <test/Test.hpp>

#include <printer/JsonPrinter.hpp>

#include "MqTest.hpp"
#include "PThreadTest.hpp"
#include "SchedTest.hpp"
#include "SignalTest.hpp"
#include "TimeTest.hpp"
#include "UnistdTest.hpp"
#include "sl_config.h"

using namespace printer;
using namespace sys;
using namespace test;

enum {
  stdio_test = 1 << 4,
  aio_test = 1 << 5,
  sem_test = 1 << 6,
  listio_test = 1 << 7,
  dirent_test = 1 << 8,
  mq_test = 1 << 9,
  sched_test = 1 << 10,
  pthread_test = 1 << 11,
  directory_test = 1 << 12,
  file_test = 1 << 13,
  access_test = 1 << 14,
  sleep_test = 1 << 15,
  signal_test = 1 << 16,
  launch_test = 1 << 17,
  unistd_test = 1 << 18,
  time_test = 1 << 19
};

static Test::ExecuteFlags decode_cli(const Cli &cli);
static void show_usage(const Cli &cli);

int main(int argc, char *argv[]) {
  Cli cli(argc, argv);

  auto o_execute_flags = decode_cli(cli);

  auto show_help = [](const Cli &cli) {
    cli.show_help(Cli::ShowHelp()
                    .set_publisher("Stratify Labs, Inc")
                    .set_version(SL_CONFIG_VERSION_STRING));
  };

  if (cli.get_option("help") == "true") {
    show_help(cli);
    exit(1);
  }

  if (o_execute_flags == Test::ExecuteFlags::null) {
    show_help(cli);
    exit(1);
  }

  {
    auto scope = Test::Scope<Printer>(Test::Initialize()
                                        .set_git_hash(SOS_GIT_HASH)
                                        .set_name("posixtest")
                                        .set_version(SL_CONFIG_VERSION_STRING));

    Test::printer().set_verbose_level(cli.get_option("verbose"));

    if (u32(o_execute_flags) & sched_test) {
      SchedTest().execute(o_execute_flags);
    }

    if (u32(o_execute_flags) & pthread_test) {
      PThreadTest().execute(o_execute_flags);
    }

    if (u32(o_execute_flags) & mq_test) {
      MqTest().execute(o_execute_flags);
    }

    if (u32(o_execute_flags) & unistd_test) {
      UnistdTest(argv[0]).execute(o_execute_flags);
    }

    if (u32(o_execute_flags) & signal_test) {
      SignalTest().execute(o_execute_flags);
    }

    if (u32(o_execute_flags) & time_test) {
      TimeTest().execute(o_execute_flags);
    }
  }
  return 0;
}

Test::ExecuteFlags decode_cli(const Cli &cli) {
  u32 o_flags = 0;
  auto exec_flags = Test::parse_execution_flags(cli);
  o_flags |= Test::parse_test(cli, "stdio", stdio_test);
  o_flags |= Test::parse_test(cli, "aio", aio_test);
  o_flags |= Test::parse_test(cli, "sem", sem_test);
  o_flags |= Test::parse_test(cli, "listio", listio_test);
  o_flags |= Test::parse_test(cli, "dirent", dirent_test);
  o_flags |= Test::parse_test(cli, "mq", mq_test);
  o_flags |= Test::parse_test(cli, "sched", sched_test);
  o_flags |= Test::parse_test(cli, "unistd", unistd_test);
  o_flags |= Test::parse_test(cli, "pthread", pthread_test);
  o_flags |= Test::parse_test(cli, "directory", directory_test);
  o_flags |= Test::parse_test(cli, "file", file_test);
  o_flags |= Test::parse_test(cli, "access", access_test);
  o_flags |= Test::parse_test(cli, "sleep", sleep_test);
  o_flags |= Test::parse_test(cli, "launch", launch_test);
  o_flags |= Test::parse_test(cli, "time", time_test);
  return Test::ExecuteFlags(o_flags | u32(exec_flags));
}
