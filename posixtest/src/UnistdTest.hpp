#ifndef UNISTDTEST_HPP
#define UNISTDTEST_HPP

#include <test.hpp>

class UnistdTest : public Test {
public:
  UnistdTest();

  bool execute_class_api_case();

private:
  bool execute_api_access_case();
  bool execute_api_sleep_case();
  bool execute_api_directory_case();
  bool execute_api_file_case();

  static constexpr auto path_too_long =
      "/0123456789/01234567890/123456789/0123/"
      "4567890/123456789012345/67890123/456/789"
      "/0123456789/01234567890/123456789/0123/"
      "4567890/123456789012345/67890123/456/789"
      "/0123456789/01234567890/123456789/0123/"
      "4567890/123456789012345/67890123/456/789"
      "/0123456789/01234567890/123456789/0123/"
      "4567890/123456789012345/67890123/456/789";

  static constexpr auto name_too_long = "/0123456789012345678901234567890123"
                                        "456789012345678901234567890123456789/"
                                        "456789012345678901234567890123456789";
};

#endif // UNISTDTEST_HPP
