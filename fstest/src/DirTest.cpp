#include <chrono.hpp>
#include <fs.hpp>
#include <sys.hpp>
#include <var.hpp>

#include "DirTest.hpp"

DirTest::DirTest(const StringView path) : Test("DirTest") { m_path = path; }

bool DirTest::execute_class_api_case() {
  // see what is supported by the system and report it
  const auto fstest_path = m_path / "fstest";
  {
    api::ErrorScope es;
    FileSystem().remove_directory(fstest_path);
  }

  {
    Case cg(this, "CanCreate?");
    api::ErrorScope es;
    FileSystem().create_directory(fstest_path);
    if (is_error()) {
      if (error().error_number() == ENOTSUP) {
        printer().key("abort",
            "creating directories is not supported on this filesystem");
        return true;
      }
    }
    TEST_ASSERT(is_success());
  }

  {
    Case cg(this, "Remove");
    api::ErrorScope es;
    FileSystem().remove_directory(fstest_path);
    TEST_ASSERT(is_success());
  }

  {
    const auto tmp =
        fstest_path / ClockTime::get_system_time().to_unique_string();
    Case cg(this, "CreateDirectory");
    api::ErrorScope es;
    FileSystem().create_directory(fstest_path).create_directory(tmp);
    TEST_ASSERT(is_success());
    {
      Case cg(this, "CreateFile");
      const auto file_path = tmp / "test.txt";
      File(File::IsOverwrite::yes, file_path).write("Hello\n");
      TEST_ASSERT(FileSystem().exists(file_path));
    }
  }

  return case_result();
}

bool DirTest::execute_class_performance_case() { return case_result(); }

bool DirTest::execute_class_stress_case() { return case_result(); }
