
#include <cstdio>

#include <sys/Cli.hpp>
#include <test/Test.hpp>

#include "sl_config.h"

#include "dhry.h"

using namespace sys;
using namespace test;
using namespace var;
using namespace printer;

static int (*dmain)() = 0;

class Dhrystone : public Test {
public:
  Dhrystone(const StringView name) : Test(name) {}

  bool execute_class_performance_case() {

    // 1000 seconds is the baseline -- divide this by execution time to get
    // score -- smaller time is a higher score
    const u32 baseline_microseconds = 1000000000UL;

    printer().key(
      "runs",
      GeneralString().format("%ld", NUMBER_OF_RUNS).cstring());
    if (dhry_main() < 0) {
      return false;
    }
    u32 dmips = NUMBER_OF_RUNS * 1000 / (case_timer().milliseconds() * 1757);
    printer().key(
      "score",
      NumberString(baseline_microseconds / case_timer().microseconds()));
    printer().key("dmips", NumberString(dmips));
    return true;
  }
};

int main(int argc, char *argv[]) {
  Cli cli(argc, argv);

  // dmain = (int (*)())kernel_request_api(0);


  {
    auto scope = Test::Scope<Printer>(Test::Initialize()
                               .set_git_hash(SOS_GIT_HASH)
                               .set_name(SL_CONFIG_NAME)
                               .set_version(SL_CONFIG_VERSION_STRING));

    Dhrystone("dhrystone").execute(Test::ExecuteFlags::performance);
  }

  return 0;
}
