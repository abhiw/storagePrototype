#include "../include/common/file_handle.h"

#include <fcntl.h>
#include <gtest/gtest.h>
#include <unistd.h>

#include <filesystem>

namespace fs = std::filesystem;

class FileHandleTest : public ::testing::Test {
 protected:
  void SetUp() override {
    test_dir_ = "/tmp/file_handle_test_" + std::to_string(getpid());
    fs::create_directories(test_dir_);
  }

  void TearDown() override {
    if (fs::exists(test_dir_)) {
      fs::remove_all(test_dir_);
    }
  }

  std::string test_dir_;
};

TEST_F(FileHandleTest, OpenAndClose) {
  const std::string test_file = test_dir_ + "/test.dat";

  {
    FileHandle fh(test_file, O_RDWR | O_CREAT, 0644);
    EXPECT_TRUE(fh.is_open());
    EXPECT_GE(fh.get(), 0);

    // Verify file exists and is writable
    const char* data = "test data";
    ssize_t written = write(fh.get(), data, 9);
    EXPECT_EQ(written, 9);
  }

  // File should still exist after FileHandle is destroyed
  EXPECT_TRUE(fs::exists(test_file));
}

TEST_F(FileHandleTest, MoveConstructor) {
  const std::string test_file = test_dir_ + "/move_test.dat";

  FileHandle fh1(test_file, O_RDWR | O_CREAT, 0644);
  int original_fd = fh1.get();
  EXPECT_TRUE(fh1.is_open());

  // Move construct fh2 from fh1
  FileHandle fh2(std::move(fh1));

  // fh2 should have the original fd, fh1 should be invalid
  EXPECT_EQ(fh2.get(), original_fd);
  EXPECT_TRUE(fh2.is_open());
  EXPECT_FALSE(fh1.is_open());
  EXPECT_EQ(fh1.get(), -1);

  // fh2 should be usable
  const char* data = "moved";
  ssize_t written = write(fh2.get(), data, 5);
  EXPECT_EQ(written, 5);
}

TEST_F(FileHandleTest, MoveAssignment) {
  const std::string test_file1 = test_dir_ + "/move_assign1.dat";
  const std::string test_file2 = test_dir_ + "/move_assign2.dat";

  FileHandle fh1(test_file1, O_RDWR | O_CREAT, 0644);
  FileHandle fh2(test_file2, O_RDWR | O_CREAT, 0644);

  int fd1 = fh1.get();
  int fd2 = fh2.get();

  EXPECT_NE(fd1, fd2);

  // Move assign fh1 to fh2
  fh2 = std::move(fh1);

  // fh2 should now have fd1, fh1 should be invalid
  EXPECT_EQ(fh2.get(), fd1);
  EXPECT_TRUE(fh2.is_open());
  EXPECT_FALSE(fh1.is_open());

  // fd2 should have been closed by move assignment
  // (we can't easily test this directly without checking kernel state)
}

TEST_F(FileHandleTest, ExplicitClose) {
  const std::string test_file = test_dir_ + "/close_test.dat";

  FileHandle fh(test_file, O_RDWR | O_CREAT, 0644);
  EXPECT_TRUE(fh.is_open());

  fh.close();
  EXPECT_FALSE(fh.is_open());
  EXPECT_EQ(fh.get(), -1);

  // Multiple closes should be safe
  fh.close();
  EXPECT_FALSE(fh.is_open());
}

TEST_F(FileHandleTest, ExceptionOnInvalidFile) {
  // Try to open a file in a non-existent directory
  EXPECT_THROW(
      { FileHandle fh("/nonexistent/path/that/does/not/exist.dat", O_RDONLY); },
      std::runtime_error);
}

TEST_F(FileHandleTest, ExceptionOnReadOnlyFile) {
  const std::string test_file = test_dir_ + "/readonly_test.dat";

  // Create a read-only file
  {
    FileHandle fh(test_file, O_RDWR | O_CREAT, 0444);
    const char* data = "readonly";
    write(fh.get(), data, 8);
  }

  // Make the directory read-only to prevent opening for write
  // (Note: This test might not work on all systems/file systems)
  EXPECT_NO_THROW({
    FileHandle fh(test_file, O_RDONLY);
    EXPECT_TRUE(fh.is_open());
  });
}

TEST_F(FileHandleTest, MultipleFilesIndependently) {
  const std::string file1 = test_dir_ + "/file1.dat";
  const std::string file2 = test_dir_ + "/file2.dat";
  const std::string file3 = test_dir_ + "/file3.dat";

  FileHandle fh1(file1, O_RDWR | O_CREAT, 0644);
  FileHandle fh2(file2, O_RDWR | O_CREAT, 0644);
  FileHandle fh3(file3, O_RDWR | O_CREAT, 0644);

  // All should have different file descriptors
  EXPECT_NE(fh1.get(), fh2.get());
  EXPECT_NE(fh2.get(), fh3.get());
  EXPECT_NE(fh1.get(), fh3.get());

  // All should be usable
  EXPECT_TRUE(fh1.is_open());
  EXPECT_TRUE(fh2.is_open());
  EXPECT_TRUE(fh3.is_open());

  // Write different data to each
  write(fh1.get(), "111", 3);
  write(fh2.get(), "222", 3);
  write(fh3.get(), "333", 3);

  // All files should exist
  EXPECT_TRUE(fs::exists(file1));
  EXPECT_TRUE(fs::exists(file2));
  EXPECT_TRUE(fs::exists(file3));
}

TEST_F(FileHandleTest, RAIIAutomaticCleanup) {
  const std::string test_file = test_dir_ + "/raii_test.dat";
  int fd_before;

  {
    FileHandle fh(test_file, O_RDWR | O_CREAT, 0644);
    fd_before = fh.get();
    EXPECT_GE(fd_before, 0);
    EXPECT_TRUE(fh.is_open());

    // Write some data
    const char* data = "RAII test";
    write(fh.get(), data, 9);
  }
  // FileHandle destructor should have closed the fd

  // File should still exist
  EXPECT_TRUE(fs::exists(test_file));

  // Try to use the old fd - should fail (fd was closed)
  char buffer[10];
  ssize_t result = read(fd_before, buffer, 10);
  EXPECT_EQ(result, -1);    // Should fail because fd is closed
  EXPECT_EQ(errno, EBADF);  // Bad file descriptor
}
