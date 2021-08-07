#include <unistd.h>

#include <chrono.hpp>
#include <fs.hpp>
#include <sos.hpp>
#include <var.hpp>

#include "UnistdTest.hpp"

#define EXEC_PATH "/app/flash/posixtest"

UnistdTest::UnistdTest()
    : Test("posix::unistd"),
      m_is_home_available(FileSystem().directory_exists("/home")) {}

bool UnistdTest::execute_class_api_case() {

  printer().key_bool("isHomeAvailable", m_is_home_available);

  execute_api_access_case();
  execute_api_directory_case();
  execute_api_file_case();
  execute_api_pid_case();
  execute_api_sleep_case();

  return case_result();
}

bool UnistdTest::execute_api_access_case() {
  test::Case tc(this, "access");
  int fd;
  TEST_EXPECT(access(EXEC_PATH, F_OK) == 0);
  TEST_EXPECT(access("/nomount/some/file", F_OK) < 0 && errno == ENOENT);
  TEST_EXPECT(access("/nomount", F_OK) && errno == ENOENT);
  TEST_EXPECT(access(path_too_long, F_OK) < 0 && errno == ENAMETOOLONG);

  TEST_EXPECT(access(name_too_long, F_OK) < 0 && errno == ENAMETOOLONG);

  TEST_EXPECT(access("/{}", F_OK) < 0 && errno == EINVAL);

  const auto path = get_test_path();

  // create a file in home
  const var::StringView test_string = "test";
  if (m_is_home_available) {
    TEST_EXPECT((fd = open(path, O_RDWR | O_CREAT, 0666)) >= 0);
    TEST_EXPECT(write(fd, test_string.data(), test_string.length()) == 4);
    TEST_EXPECT(close(fd) == 0);
  } else {
    if (FileSystem().exists(path) == false) {
      ViewFile test_string_view_file(test_string);
      TEST_ASSERT(Appfs(Appfs::Construct()
                            .set_name("test.txt")
                            .set_size(test_string.length()))
                      .append(test_string_view_file, nullptr)
                      .is_success());
    }
  }

  if (m_is_home_available) {
    TEST_EXPECT(access(path, W_OK) == 0);
  } else {
    TEST_EXPECT(access(path, W_OK) < 0 && errno == EACCES);
  }
  TEST_EXPECT(access(EXEC_PATH, W_OK) < 0 && errno == EACCES);

  if (m_is_home_available) {
    TEST_EXPECT(unlink(path) >= 0);
  }

  TEST_EXPECT(access("/", W_OK) == 0 && errno == EACCES);
  TEST_EXPECT(access(EXEC_PATH, R_OK) == 0);

  TEST_EXPECT(access("/app/.install", R_OK) < 0 && errno == EACCES);
  TEST_EXPECT(access(EXEC_PATH, X_OK) == 0);
  TEST_EXPECT(access("/app/.install", X_OK) < 0 && errno == EACCES);

  return case_result();
}

bool UnistdTest::execute_api_sleep_case() {
  test::Case tc(this, "sleep");

  auto usleep_test = [](Test *self, u32 start, u32 stop) -> bool {
    test::Case usleep_case(self, "usleep" | NumberString(start) | ":" |
                                     NumberString(stop));
    const auto step = stop / 10;
    for (u32 i = start; i < stop; i += step) {
      ClockTimer timer(ClockTimer::IsRunning::yes);
      usleep(i);
      timer.stop();
      if (timer.microseconds() < i) {
        self->printer().key("underSlept", NumberString(i));
      }
      TEST_SELF_EXPECT(timer.microseconds() >= i);
    }
    return true;
  };

  for (u32 i = 1; i < 100000UL; i *= 10) {
    TEST_EXPECT(usleep_test(this, i, i * 10) == true);
  }

  auto sleep_test = [](Test *self, u32 seconds) -> bool {
    test::Case usleep_case(self, "sleep:" | NumberString(seconds));
    ClockTimer timer(ClockTimer::IsRunning::yes);
    sleep(seconds);
    timer.stop();
    if (timer.seconds() < seconds) {
      self->printer().key("underSlept", NumberString(seconds));
    }
    TEST_SELF_EXPECT(timer.seconds() == seconds);

    return true;
  };

  TEST_EXPECT(sleep_test(this, 1) == true);
  TEST_EXPECT(sleep_test(this, 2) == true);
  TEST_EXPECT(sleep_test(this, 4) == true);

  return case_result();
}

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

  const auto entry_count = FileSystem().read_directory("/").count();

  for (u32 i = 0; i < entry_count; i++) {
    TEST_EXPECT(readdir_r(d, &entry, &result) == 0);
  }

  closedir(d);

  // ENAMETOOLONG
  // ENOTSUP
  // EINVAL

  // if home is available do some read/write tests
  if (m_is_home_available) {
    TEST_ASSERT(mkdir("/home/test", 0777) >= 0);
    TEST_EXPECT(rmdir("/home/test1") < 0 && errno == ENOENT);
  }

  return case_result();
}

bool UnistdTest::execute_api_file_case() {
  test::Case tc(this, "file");

  // open close afile at /app/flash
  {
    int fd;
    TEST_EXPECT(open(EXEC_PATH, O_RDWR) < 0 && errno == EROFS);
    TEST_EXPECT(open(EXEC_PATH, O_WRONLY) < 0 && errno == EROFS);
    TEST_EXPECT((fd = open(EXEC_PATH, O_RDONLY)) >= 0);
    TEST_EXPECT(close(fd) == 0);

    // open close a device
    TEST_EXPECT((fd = open("/dev/sys", O_RDWR)) >= 0);
    TEST_EXPECT(ioctl(fd, I_SYS_GETVERSION) == SYS_VERSION);
    {
      sys_id_t sys_id;
      TEST_EXPECT(ioctl(fd, I_SYS_GETID, &sys_id) >= 0);
      printer().key("id", var::StringView(sys_id.id));
    }

    struct stat st = {};
    TEST_EXPECT(fstat(fd, &st) == 0);
    TEST_EXPECT((st.st_mode & S_IFMT) == S_IFCHR);
    TEST_EXPECT((st.st_mode & ACCESSPERMS) == 0666);
    TEST_EXPECT(st.st_dev == 0);
    TEST_EXPECT(st.st_size == 0);
    TEST_EXPECT(st.st_uid == 0);
    printer().key("ino", var::NumberString(st.st_ino));
    TEST_EXPECT(close(fd) == 0);

    TEST_EXPECT((fd = open("/dev/sys", O_RDONLY)) >= 0);
    TEST_EXPECT(ioctl(fd, I_SYS_GETVERSION) == SYS_VERSION);
    TEST_EXPECT(close(fd) == 0);

    TEST_EXPECT((fd = open("/dev/sys", O_WRONLY)) >= 0);
    TEST_EXPECT(ioctl(fd, I_SYS_GETVERSION) == SYS_VERSION);
    TEST_EXPECT(close(fd) == 0);
  }

  char buffer[16];
  TEST_EXPECT(read(1000, buffer, sizeof(buffer)) < 0 && errno == EBADF);
  TEST_EXPECT(read(-1, buffer, sizeof(buffer)) < 0 && errno == EBADF);
  TEST_EXPECT(write(1000, buffer, sizeof(buffer)) < 0 && errno == EBADF);
  TEST_EXPECT(write(-1, buffer, sizeof(buffer)) < 0 && errno == EBADF);
  TEST_EXPECT(close(1000) < 0 && errno == EBADF);
  TEST_EXPECT(close(-1) < 0 && errno == EBADF);
  {
    struct stat st = {};
    TEST_EXPECT(fstat(1000, &st) < 0 && errno == EBADF);
    TEST_EXPECT(fstat(-1, &st) < 0 && errno == EBADF);
    TEST_EXPECT(stat("/no_exist", &st) < 0 && errno == ENOENT);
  }

  TEST_EXPECT(unlink("/no_exist") < 0 && errno == ENOTSUP);
  TEST_EXPECT(unlink("/app/flash/no_exist") < 0 && errno == ENOENT);
  TEST_EXPECT(unlink("/app/flash1") < 0 && errno == ENOENT);
  TEST_EXPECT(unlink(path_too_long) < 0 && errno == ENAMETOOLONG);
  TEST_EXPECT(unlink(name_too_long) < 0 && errno == ENAMETOOLONG);
  TEST_EXPECT(unlink("/{}") < 0 && errno == EINVAL);

  printer().key("openMax", var::NumberString(OPEN_MAX));
  // check for too many open files
  var::Array<int, OPEN_MAX + 1> fd_list;
  for (auto &fd : fd_list) {
    fd = open("/dev/sys", O_RDWR);
  }
  TEST_EXPECT(errno = EMFILE);
  for (auto &fd : fd_list) {
    close(fd);
  }

  TEST_EXPECT(open("/dev/no_exist", O_RDWR) < 0 && errno == ENOENT);
  TEST_EXPECT(open("/dir", O_RDWR | O_CREAT, S_IFDIR) < 0 && errno == EINVAL);

  {
    int fd;
    struct stat st0 = {};
    struct stat st1 = {};
    TEST_ASSERT((fd = open(get_test_path(), O_RDONLY)) >= 0);
    TEST_EXPECT(fstat(fd, &st0) == 0);
    TEST_EXPECT(stat(get_test_path(), &st1) == 0);
    TEST_ASSERT(View(st0) == View(st1));
  }

  if (m_is_home_available) {
    TEST_ASSERT(File(File::IsOverwrite::yes, get_test_path())
                    .write("testfile")
                    .is_success());
    TEST_EXPECT(unlink(get_test_path()) == 0);

    // rename
  }

  return case_result();
}

bool UnistdTest::execute_api_pid_case() {
  test::Case tc(this, "pid");

  const auto is_authenticated = (getuid() == SYSFS_ROOT);

  printer().key("uid", getuid() == SYSFS_USER ? "user" : "root");
  printer().key("euid", geteuid() == SYSFS_USER ? "user" : "root");
  printer().key_bool("isAuthenticated", is_authenticated);
  TEST_EXPECT(getpid() > 0);
  TEST_EXPECT(getppid() == 0);
  TEST_EXPECT(geteuid() != 0);
  if (is_authenticated == false) {
    TEST_EXPECT(getuid() == static_cast<uid_t>(SYSFS_USER));
    TEST_EXPECT(!is_authenticated || getuid() != 0);
    TEST_EXPECT(setuid(static_cast<uid_t>(SYSFS_ROOT)) < 0 && errno == EPERM);
  } else {
    TEST_EXPECT(getuid() == static_cast<uid_t>(SYSFS_ROOT));
    TEST_EXPECT(setuid(static_cast<uid_t>(SYSFS_ROOT)) == 0);
    TEST_EXPECT(geteuid() == static_cast<uid_t>(SYSFS_ROOT));
    TEST_EXPECT(setuid(static_cast<uid_t>(SYSFS_USER)) == 0);
    TEST_EXPECT(geteuid() == static_cast<uid_t>(SYSFS_USER));
  }

  return true;
}
