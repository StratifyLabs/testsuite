#ifndef DIRTEST_HPP
#define DIRTEST_HPP

#include <test.hpp>
#include <var.hpp>

class DirTest : public Test {
public:
  DirTest(const StringView path);

  bool execute_class_api_case();
  bool execute_class_performance_case();
  bool execute_class_stress_case();

private:
  PathString m_path;
};

#endif // DIRTEST_HPP
