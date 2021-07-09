#ifndef FILETEST_HPP
#define FILETEST_HPP

#include <test.hpp>
#include <var.hpp>

class FileTest : public Test {
public:
  FileTest(const StringView path);

  bool execute_class_api_case();
  bool execute_class_performance_case();
  bool execute_class_stress_case();

private:
  class Stats {
    API_AF(Stats, u32, speed, 0);
    API_AF(Stats, u32, page_size, 0);
  };

  enum class StatsType {
    read, write
  };

  PathString m_path;
  Stats m_best_read;
  Stats m_best_write;

  bool execute_file_append_performance_test(int count, int page_size,
                                            int file_size);

  void show_stats(const StringView name, u32 page_size, u32 file_size, StatsType type);




  static constexpr size_t max_page_size(){
    return 4096;
  }
};

#endif // FILETEST_HPP
