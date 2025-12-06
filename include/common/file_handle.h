#ifndef STORAGEENGINE_FILE_HANDLE_H
#define STORAGEENGINE_FILE_HANDLE_H

#include <string>
#include <utility>

// RAII wrapper for POSIX file descriptors
// Automatically closes file descriptor in destructor, ensuring no fd leaks
// Move-only to prevent fd duplication
class FileHandle {
 public:
  // Constructor: opens file with given flags and mode
  // Throws std::runtime_error if file cannot be opened
  explicit FileHandle(const std::string& filename, int flags,
                      mode_t mode = 0644);

  // Destructor: automatically closes file descriptor
  ~FileHandle();

  // Move constructor: transfer ownership of file descriptor
  FileHandle(FileHandle&& other) noexcept;

  // Move assignment: transfer ownership of file descriptor
  FileHandle& operator=(FileHandle&& other) noexcept;

  // Delete copy operations - file descriptors shouldn't be copied
  FileHandle(const FileHandle&) = delete;
  FileHandle& operator=(const FileHandle&) = delete;

  // Get underlying file descriptor
  int get() const { return fd_; }

  // Check if file is open (fd >= 0)
  bool is_open() const { return fd_ >= 0; }

  // Explicit close (destructor also closes, but this allows error handling)
  void close();

 private:
  int fd_;  // POSIX file descriptor (-1 if closed)
};

#endif  // STORAGEENGINE_FILE_HANDLE_H
