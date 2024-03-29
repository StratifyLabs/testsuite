
#include <printer.hpp>
#include <sys.hpp>
#include <var.hpp>

#include "sl_config.h"

#include "DirTest.hpp"
#include "FileTest.hpp"

int main(int argc, char *argv[]) {
  Cli cli(argc, argv);
  {
    auto scope = Test::Scope<Printer>(Test::Initialize()
                                        .set_git_hash(SOS_GIT_HASH)
                                        .set_name("fstest")
                                        .set_version(SL_CONFIG_VERSION_STRING));

    const auto path_argument = cli.get_option("path");
    const PathString path = path_argument.is_empty() ? "/home" : path_argument;

    DirTest(path).execute(cli);
    FileTest(path).execute(cli);
  }

  return 0;
}
