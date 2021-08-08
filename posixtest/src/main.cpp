// Copyright 2011-2021 Tyler Gilbert and Stratify Labs, Inc; see LICENSE.md

#include "tests.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys.hpp>
#include <test.hpp>
#include <unistd.h>

#include <printer/JsonPrinter.hpp>

#include "MqTest.hpp"
#include "PThreadTest.hpp"
#include "SchedTest.hpp"
#include "SignalTest.hpp"
#include "UnistdTest.hpp"
#include "sl_config.h"

enum {
  STDIO_TEST = 1 << 4,
  AIO_TEST = 1 << 5,
  SEM_TEST = 1 << 6,
  LISTIO_TEST = 1 << 7,
  DIRENT_TEST = 1 << 8,
  MQ_TEST = 1 << 9,
  SCHED_TEST = 1 << 10,
  PTHREAD_TEST = 1 << 11,
  DIRECTORY_TEST = 1 << 12,
  FILE_TEST = 1 << 13,
  ACCESS_TEST = 1 << 14,
  SLEEP_TEST = 1 << 15,
  SIGNAL_TEST = 1 << 16,
  LAUNCH_TEST = 1 << 17,
  UNISTD_TEST = 1 << 18
};

static Test::ExecuteFlags decode_cli(const Cli &cli);
static void show_usage(const Cli &cli);

int main(int argc, char *argv[]) {
  Cli cli(argc, argv);

  auto o_execute_flags = decode_cli(cli);

  auto show_help = [](const Cli & cli){
    cli.show_help(Cli::ShowHelp()
                      .set_publisher("Stratify Labs, Inc")
                      .set_version(SL_CONFIG_VERSION_STRING));
  };

  if( cli.get_option("help") == "true" ){
    show_help(cli);
    exit(1);
  }

  if (o_execute_flags == Test::ExecuteFlags::null) {
    show_help(cli);
    exit(1);
  }

  printer::Printer printer;
  Test::initialize(Test::Initialize()
                       .set_git_hash(SOS_GIT_HASH)
                       .set_name("posixtest")
                       .set_printer(&printer)
                       .set_version(SL_CONFIG_VERSION_STRING));

  if (u32(o_execute_flags) & SCHED_TEST) {
    SchedTest().execute(o_execute_flags);
  }

  if (u32(o_execute_flags) & PTHREAD_TEST) {
    PThreadTest().execute(o_execute_flags);
  }

  if (u32(o_execute_flags) & MQ_TEST) {
    MqTest().execute(o_execute_flags);
  }

  if (u32(o_execute_flags) & UNISTD_TEST) {
    UnistdTest().execute(o_execute_flags);
  }

  if (u32(o_execute_flags) & SIGNAL_TEST) {
    SignalTest().execute(o_execute_flags | Test::ExecuteFlags::api);
  }

  Test::finalize();
  return 0;
}

Test::ExecuteFlags decode_cli(const Cli &cli) {
  u32 o_flags = 0;
  auto exec_flags = Test::parse_execution_flags(cli);
  o_flags |= Test::parse_test(cli, "stdio", STDIO_TEST);
  o_flags |= Test::parse_test(cli, "aio", AIO_TEST);
  o_flags |= Test::parse_test(cli, "sem", SEM_TEST);
  o_flags |= Test::parse_test(cli, "listio", LISTIO_TEST);
  o_flags |= Test::parse_test(cli, "dirent", DIRENT_TEST);
  o_flags |= Test::parse_test(cli, "mq", MQ_TEST);
  o_flags |= Test::parse_test(cli, "sched", SCHED_TEST);
  o_flags |= Test::parse_test(cli, "unistd", UNISTD_TEST);
  o_flags |= Test::parse_test(cli, "pthread", PTHREAD_TEST);
  o_flags |= Test::parse_test(cli, "directory", DIRECTORY_TEST);
  o_flags |= Test::parse_test(cli, "file", FILE_TEST);
  o_flags |= Test::parse_test(cli, "access", ACCESS_TEST);
  o_flags |= Test::parse_test(cli, "sleep", SLEEP_TEST);
  o_flags |= Test::parse_test(cli, "signal", SIGNAL_TEST);
  o_flags |= Test::parse_test(cli, "launch", LAUNCH_TEST);
  return Test::ExecuteFlags(o_flags | u32(exec_flags));
}
