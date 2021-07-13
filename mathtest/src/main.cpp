#include <stdio.h>

#include "num_test.h"
#include "sl_config.h"
#include <sys.hpp>
#include <test.hpp>
#include <var.hpp>

u32 decode_cli(const Cli &cli);
void show_usage(const Cli &cli);

int main(int argc, char *argv[]) {
  Cli cli(argc, argv);
  u32 o_execute_flags;
  u32 idx;

  if (cli.get_option("help") == "true") {
    cli.show_help(Cli::ShowHelp()
                      .set_publisher("Stratify Labs, Inc")
                      .set_version(SL_CONFIG_VERSION_STRING));
  }

  o_execute_flags = decode_cli(cli);

  if (o_execute_flags == 0) {
    printf("Nothing to test\n");
    return 1;
  }

  printer::Printer printer;

  Test::initialize(Test::Initialize()
                       .set_git_hash(SOS_GIT_HASH)
                       .set_name("mathtest")
                       .set_printer(&printer));

  for (idx = 0; idx < num_test_count(); idx++) {
    const test_t *test = num_test_get(idx);
    if ((test->o_execute_flags & o_execute_flags) == test->o_execute_flags) {
      GeneralString name;
      name.format("%d:%s: %s %s %s", idx, test->name, test->a_str, test->op_str,
                  test->b_str);

      printer::Printer::Object case_object(printer, name);
      int result = test->operation(test->a, test->b, test->result);
      printer.key_bool("result", result != 0);

    }
  }

  Test::finalize();

  return 0;
}

u32 decode_cli(const Cli &cli) {
  auto execution_flags = Test::parse_execution_flags(cli);
  u32 o_flags = 0;
  o_flags |= Test::parse_test(cli, "comparison", COMPARISON_TEST_FLAG);
  o_flags |= Test::parse_test(cli, "arithmetic", ARITHMETIC_TEST_FLAG);
  o_flags |= Test::parse_test(cli, "logic", LOGICAL_TEST_FLAG);
  o_flags |= Test::parse_test(cli, "shift", SHIFT_TEST_FLAG);
  o_flags |= Test::parse_test(cli, "typecast", TYPE_CAST_TEST_FLAG);
  o_flags |= Test::parse_test(cli, "u8", U8_TEST_FLAG);
  o_flags |= Test::parse_test(cli, "u16", U16_TEST_FLAG);
  o_flags |= Test::parse_test(cli, "u32", U32_TEST_FLAG);
  o_flags |= Test::parse_test(cli, "u64", U64_TEST_FLAG);
  o_flags |= Test::parse_test(cli, "s8", S8_TEST_FLAG);
  o_flags |= Test::parse_test(cli, "s16", S16_TEST_FLAG);
  o_flags |= Test::parse_test(cli, "s32", S32_TEST_FLAG);
  o_flags |= Test::parse_test(cli, "s64", S64_TEST_FLAG);
  o_flags |= Test::parse_test(cli, "float", FLOAT_TEST_FLAG);
  o_flags |= Test::parse_test(cli, "double", DOUBLE_TEST_FLAG);
  o_flags |= Test::parse_test(cli, "int", INT_TEST_FLAG);
  if (cli.get_option("allTypes") == "true") { // all but double
    o_flags |= U8_TEST_FLAG | U16_TEST_FLAG | U32_TEST_FLAG | U64_TEST_FLAG |
               S8_TEST_FLAG | S16_TEST_FLAG | S32_TEST_FLAG | S64_TEST_FLAG |
               FLOAT_TEST_FLAG | INT_TEST_FLAG;
  }
  return o_flags | u32(execution_flags);
}
