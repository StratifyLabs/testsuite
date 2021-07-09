#include <fs.hpp>
#include <var.hpp>

#include "FileTest.hpp"

FileTest::FileTest(const StringView path) : Test("FileTest") { m_path = path; }

bool FileTest::execute_class_api_case() {

  const PathString test_file_path = m_path / "test.txt";
  const StringView hello_string = "HelloFile";
  {
    api::ErrorScope es;
    FileSystem().remove(test_file_path);
  }

  {
    Case cg(this, "Create");
    TEST_ASSERT(FileSystem().exists(test_file_path) == false);
    File(File::IsOverwrite::yes, test_file_path).write(hello_string);
    TEST_ASSERT(is_success());
    TEST_ASSERT(FileSystem().exists(test_file_path) == true);
  }

  {
    Case cg(this, "verify");
    File f(test_file_path);
    TEST_ASSERT(is_success());
    Array<char, 64> buffer;

    TEST_ASSERT(f.read(buffer).return_value() ==
                static_cast<int>(hello_string.length()));
  }
  TEST_ASSERT(is_success());

  {
    Case cg(this, "cleanup");
    TEST_ASSERT(FileSystem().remove(test_file_path).is_success());
    TEST_ASSERT(FileSystem().exists(test_file_path) == false);
  }

  return case_result();
}

bool FileTest::execute_class_performance_case() {
  for (u32 i = 4; i < 32; i+=2) {
    execute_file_append_performance_test(i, i * 128, i * 128 * 100);
  }
  printer()
      .open_object("bestWrite")
      .key("speed", NumberString(m_best_write.speed(), "%d KB/s"))
      .key("pageSize", NumberString(m_best_write.page_size()))
      .close_object()
      .open_object("bestRead")
      .key("speed", NumberString(m_best_read.speed(), "%d KB/s"))
      .key("pageSize", NumberString(m_best_read.page_size()))
      .close_object();

  return case_result();
}

bool FileTest::execute_class_stress_case() { return case_result(); }

bool FileTest::execute_file_append_performance_test(int count, int page_size,
                                                    int file_size) {

  const PathString file_path = m_path / NumberString(count, "test%ld.txt");
  Case cg(this, file_path);

  const auto name = Path::name(file_path);

  File f(File::IsOverwrite::yes, file_path);
  Data buffer(page_size);

  {
    Case cg(this, "append");
    for (int i = 0; i < file_size; i += page_size) {
      f.write(View(buffer).fill<u8>(i));
      TEST_ASSERT(return_value() == page_size);
    }
    show_stats(name & "Append", page_size, file_size, StatsType::write);
  }

  {
    Case cg(this, "read");
    TEST_ASSERT(f.seek(0).is_success());
    for (int i = 0; i < file_size; i += page_size) {
      TEST_ASSERT(f.read(View(buffer)).return_value() == page_size);
    }

    show_stats(name & "Read", page_size, file_size, StatsType::read);
  }

  TEST_ASSERT(FileSystem().remove(file_path).is_success());

  return case_result();
}

void FileTest::show_stats(const var::StringView name, u32 page_size,
                          u32 file_size, StatsType type) {
  constexpr u32 factor = 1000000UL / 1024UL;
  u32 duration_us = case_timer().microseconds();
  const u32 speed = file_size * factor / duration_us;

  Stats *const stats = type == StatsType::write ? &m_best_write : &m_best_read;

  if (speed > stats->speed()) {
    stats->set_speed(speed).set_page_size(page_size);
  }

  printer()
      .open_object(name)
      .key("pageSize", NumberString(page_size, "%d bytes"))
      .key("size", NumberString(file_size, "%d bytes"))
      .key("duration", NumberString(duration_us, "%d us"))
      .key("speed", NumberString(file_size * factor / duration_us, "%d KB/s"))
      .close_object();
}
