#include "UnistdTest.hpp"

#include <fs.hpp>
#include <sos.hpp>

#define EXEC_PATH "/app/flash/posixtest"

UnistdTest::UnistdTest() : Test("UnistdTest") {}

bool UnistdTest::execute_class_api_case() {

  //execute_api_access_case();
  execute_api_directory_case();

  return case_result();
}

bool UnistdTest::execute_api_access_case() {
  test::Case tc(this, "access");
  int fd;
  TEST_EXPECT(access(EXEC_PATH, F_OK) == 0);
  TEST_EXPECT(access("/nomount/some/file", F_OK) < 0 && errno == ENOENT);
  TEST_EXPECT(access("/nomount", F_OK) && errno == ENOENT);
  TEST_EXPECT(access(path_too_long,
                     F_OK) < 0 &&
              errno == ENAMETOOLONG);

  TEST_EXPECT(access(name_too_long,
                     F_OK) < 0 &&
              errno == ENAMETOOLONG);

  const auto is_home = FileSystem().directory_exists("/home");
  const auto path = is_home ? "/home/test.txt" : "/app/flash/test.txt";

  // create a file in home
  const var::StringView test_string = "test";
  if (is_home) {
    TEST_EXPECT((fd = open("/home/test.txt", O_RDWR | O_CREAT, 0666)) >= 0);
    TEST_EXPECT(write(fd, test_string.data(), test_string.length()) == 4);
    TEST_EXPECT(close(fd) == 0);
  } else {
    ViewFile test_string_view_file(test_string);
    TEST_ASSERT(Appfs(Appfs::Construct()
                          .set_name("test.txt")
                          .set_size(test_string.length()))
                    .append(test_string_view_file, nullptr)
                    .is_success());
  }

  if (is_home) {
    TEST_EXPECT(access(path, W_OK) == 0);
  } else {
    TEST_EXPECT(access(path, W_OK) < 0 && errno == EACCES);
  }
  TEST_EXPECT(access(EXEC_PATH, W_OK) < 0 && errno == EACCES);

  TEST_EXPECT(unlink(path) >= 0);

  TEST_EXPECT(access("/", W_OK) == 0 && errno == EACCES);
  TEST_EXPECT(access(EXEC_PATH, R_OK) == 0);

  TEST_EXPECT(access("/app/.install", R_OK) < 0 && errno == EACCES);
  TEST_EXPECT(access(EXEC_PATH, X_OK) == 0);
  TEST_EXPECT(access("/app/.install", X_OK) < 0 && errno == EACCES);

  return case_result();
}

bool UnistdTest::execute_api_sleep_case() { return case_result(); }

bool UnistdTest::execute_api_directory_case() {
  test::Case tc(this, "directory");

  // ENOENT
  TEST_EXPECT(mkdir("/none", 0777) < 0 && errno == ENOTSUP);
  TEST_EXPECT(mkdir(name_too_long, 0777) < 0 && errno == ENAMETOOLONG);
  TEST_EXPECT(mkdir(path_too_long, 0777) < 0 && errno == ENAMETOOLONG);
  TEST_EXPECT(rmdir("/none") < 0 && errno == ENOTSUP);

  auto *d = opendir("/");
  TEST_ASSERT(d != nullptr);

  struct dirent entry;
  struct dirent *result;
  TEST_EXPECT(readdir_r(d, &entry, &result) == 0);
  closedir(d);

  // ENAMETOOLONG
  // ENOTSUP
  // EINVAL

  // if home is available do some read/write tests
  const auto is_home = FileSystem().directory_exists("/home");
  if (is_home){
    TEST_ASSERT(mkdir("/home/test", 0777) >= 0);
    TEST_EXPECT(rmdir("/home/test1") < 0 && errno == ENOENT );

  }

  return case_result();
}

bool UnistdTest::execute_api_file_case() { return case_result(); }
