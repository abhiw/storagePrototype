#include "../../include/common/file_handle.h"

#include <fcntl.h>
#include <unistd.h>

#include <cerrno>
#include <stdexcept>

#include "../../include/common/logger.h"

FileHandle::FileHandle(const std::string& filename, int flags, mode_t mode)
    : fd_(-1) {
  fd_ = open(filename.c_str(), flags, mode);
  if (fd_ < 0) {
    LOG_ERROR_STREAM(
        "FileHandle: Failed to open file: " << filename << " errno: " << errno);
    throw std::runtime_error("Failed to open file: " + filename);
  }
  LOG_INFO_STREAM("FileHandle: Opened file: " << filename << " (fd=" << fd_
                                              << ")");
}

FileHandle::~FileHandle() { close(); }

FileHandle::FileHandle(FileHandle&& other) noexcept : fd_(other.fd_) {
  other.fd_ = -1;
}

FileHandle& FileHandle::operator=(FileHandle&& other) noexcept {
  if (this != &other) {
    close();
    fd_ = other.fd_;
    other.fd_ = -1;
  }
  return *this;
}

void FileHandle::close() {
  if (fd_ >= 0) {
    LOG_INFO_STREAM("FileHandle: Closing file descriptor: " << fd_);
    ::close(fd_);
    fd_ = -1;
  }
}
