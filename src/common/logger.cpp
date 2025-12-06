#include "common/logger.h"

#include <chrono>
#include <filesystem>
#include <iomanip>
#include <iostream>

namespace storage {

Logger::Logger() : debugMode_(false), logDirectory_("logs"), currentDate_("") {
  // Read log directory from environment variable, fallback to "logs"
  const char* envLogDir = std::getenv("STORAGE_ENGINE_LOG_DIR");
  if (envLogDir != nullptr && envLogDir[0] != '\0') {
    logDirectory_ = envLogDir;
  }

  // Create logs directory if it doesn't exist
  std::filesystem::create_directories(logDirectory_);
  openLogFile();
}

Logger::~Logger() {
  std::lock_guard<std::mutex> lock(mutex_);
  if (logFile_.is_open()) {
    logFile_.close();
  }
}

Logger& Logger::getInstance() {
  static Logger instance;
  return instance;
}

void Logger::setDebugMode(bool debug) {
  std::lock_guard<std::mutex> lock(mutex_);
  debugMode_ = debug;
}

bool Logger::isDebugMode() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return debugMode_;
}

void Logger::setLogDirectory(const std::string& dir) {
  std::lock_guard<std::mutex> lock(mutex_);
  logDirectory_ = dir;
  std::filesystem::create_directories(logDirectory_);

  // Close current file and open new one in new directory
  if (logFile_.is_open()) {
    logFile_.close();
  }
  logFile_.clear();  // Clear any error state flags
  openLogFile();
}

void Logger::info(const std::string& message) { log(LogLevel::INFO, message); }

void Logger::warning(const std::string& message) {
  log(LogLevel::WARNING, message);
}

void Logger::error(const std::string& message) {
  log(LogLevel::ERROR, message);
}

void Logger::log(LogLevel level, const std::string& message) {
  std::lock_guard<std::mutex> lock(mutex_);
  logInternal(level, message);
}

void Logger::logInternal(LogLevel level, const std::string& message) {
  // Check if we should log this level
  if (!shouldLog(level)) {
    return;
  }

  // Rotate log file if needed (new day)
  rotateLogFileIfNeeded();

  // Format: [TIMESTAMP] [LEVEL] message
  std::string logEntry = "[" + getCurrentTimestamp() + "] " + "[" +
                         logLevelToString(level) + "] " + message + "\n";

  // Write to file
  if (logFile_.is_open()) {
    logFile_ << logEntry;
    logFile_.flush();  // Ensure immediate write
  }

  // Also output to console for errors and warnings
  if (level == LogLevel::ERROR || level == LogLevel::WARNING) {
    std::cerr << logEntry;
  }
}

void Logger::rotateLogFileIfNeeded() {
  std::string today = getCurrentDate();

  if (today != currentDate_) {
    if (logFile_.is_open()) {
      logFile_.close();
    }
    logFile_.clear();  // Clear any error state flags
    openLogFile();
  }
}

std::string Logger::getCurrentTimestamp() const {
  auto now = std::chrono::system_clock::now();
  auto time = std::chrono::system_clock::to_time_t(now);
  auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                now.time_since_epoch()) %
            1000;

  std::tm tm;
  localtime_r(&time, &tm);

  std::ostringstream oss;
  oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
  oss << '.' << std::setfill('0') << std::setw(3) << ms.count();

  return oss.str();
}

std::string Logger::getCurrentDate() const {
  auto now = std::chrono::system_clock::now();
  auto time = std::chrono::system_clock::to_time_t(now);

  std::tm tm;
  localtime_r(&time, &tm);

  std::ostringstream oss;
  oss << std::put_time(&tm, "%Y-%m-%d");

  return oss.str();
}

std::string Logger::logLevelToString(LogLevel level) const {
  switch (level) {
    case LogLevel::INFO:
      return "INFO";
    case LogLevel::WARNING:
      return "WARNING";
    case LogLevel::ERROR:
      return "ERROR";
    default:
      return "UNKNOWN";
  }
}

bool Logger::shouldLog(LogLevel level) const {
  // In debug mode, log everything
  if (debugMode_) {
    return true;
  }

  // In non-debug mode, only log WARNING and ERROR
  return level == LogLevel::WARNING || level == LogLevel::ERROR;
}

void Logger::openLogFile() {
  currentDate_ = getCurrentDate();
  std::string filename = logDirectory_ + "/storage_" + currentDate_ + ".log";

  logFile_.open(filename, std::ios::app);

  if (!logFile_.is_open()) {
    std::cerr << "Failed to open log file: " << filename << std::endl;
  }
}

}  // namespace storage
