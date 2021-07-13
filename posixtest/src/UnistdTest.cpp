#include "UnistdTest.hpp"

#include <fs.hpp>

#define EXEC_PATH "/app/flash/posix"

UnistdTest::UnistdTest() : Test("UnistdTest") {}

bool UnistdTest::execute_class_api_case() {

  execute_api_access_case();

  return case_result();
}

bool UnistdTest::execute_api_access_case() {
  int fd;

  TEST_EXPECT(access(EXEC_PATH, F_OK) == 0);
  TEST_EXPECT(access("/nomount/some/file", F_OK) < 0 && errno == ENOENT);
  TEST_EXPECT(access("/nomount", F_OK) && errno == ENOENT);
  TEST_EXPECT(access("/01234567890123456789012345678901234567890123456789012345"
                     "67890123456789",
                     F_OK) == 0 &&
              errno == ENAMETOOLONG);

  TEST_EXPECT(access("/0123456789/01234567890/123456789/0123/"
                                   "4567890/123456789012345/67890123/456/789",
                                   F_OK) == 0 && errno == ENAMETOOLONG);

  // create a file in home
  TEST_EXPECT((fd = open("/home/test.txt", O_RDWR | O_CREAT, 0666)) >= 0);
  TEST_EXPECT(write(fd, "test", 4) == 4);
  TEST_EXPECT(close(fd) == 0);

  TEST_EXPECT(access("/home/test.txt", W_OK) == 0);
  TEST_EXPECT(access(EXEC_PATH, W_OK) < 0 && errno == EACCES);
  TEST_EXPECT(access("/", W_OK) == 0 && errno == EACCES);
  TEST_EXPECT(access(EXEC_PATH, R_OK) == 0);

  TEST_EXPECT(access("/app/.install", R_OK) < 0 && errno == EACCES);
  TEST_EXPECT(access(EXEC_PATH, X_OK) == 0);
  TEST_EXPECT(access("/app/.install", X_OK) < 0 && errno == EACCES);

  return case_result();
}

bool UnistdTest::execute_api_sleep_case() { return case_result(); }

bool UnistdTest::execute_api_directory_case() {

  // ENOENT
  // ENAMETOOLONG
  // ENOTSUP
  // EINVAL

  return case_result();
}

bool UnistdTest::execute_api_file_case() { return case_result(); }
