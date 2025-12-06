#pragma once

#include <fstream>
#include <mutex>
#include <sstream>
#include <string>

namespace storage {

enum class LogLevel { INFO, WARNING, ERROR };

class Logger {
 public:
  static Logger& getInstance();

  // Delete copy constructor and assignment operator
  Logger(const Logger&) = delete;
  Logger& operator=(const Logger&) = delete;

  // Set debug mode (logs all levels when true, only WARNING/ERROR when false)
  void setDebugMode(bool debug);

  // Get current debug mode status
  bool isDebugMode() const;

  // Set log directory
  void setLogDirectory(const std::string& dir);

  // Log methods
  void info(const std::string& message);
  void warning(const std::string& message);
  void error(const std::string& message);

  // Generic log method
  void log(LogLevel level, const std::string& message);

 private:
  Logger();
  ~Logger();

  // Internal log method (assumes mutex is already locked)
  void logInternal(LogLevel level, const std::string& message);

  // Check if we need to rotate log file (new day)
  void rotateLogFileIfNeeded();

  // Get formatted timestamp
  std::string getCurrentTimestamp() const;

  // Get current date for filename
  std::string getCurrentDate() const;

  // Get log level as string
  std::string logLevelToString(LogLevel level) const;

  // Check if this log level should be logged
  bool shouldLog(LogLevel level) const;

  // Open log file for current date
  void openLogFile();

  std::ofstream logFile_;
  mutable std::mutex mutex_;
  bool debugMode_;
  std::string logDirectory_;
  std::string currentDate_;
};

// Convenience macros for easy logging from anywhere
#define LOG_INFO(msg) storage::Logger::getInstance().info(msg)
#define LOG_WARNING(msg) storage::Logger::getInstance().warning(msg)
#define LOG_ERROR(msg) storage::Logger::getInstance().error(msg)

// Stream-style logging macros
#define LOG_INFO_STREAM(msg)                        \
  {                                                 \
    std::ostringstream oss;                         \
    oss << msg;                                     \
    storage::Logger::getInstance().info(oss.str()); \
  }

#define LOG_WARNING_STREAM(msg)                        \
  {                                                    \
    std::ostringstream oss;                            \
    oss << msg;                                        \
    storage::Logger::getInstance().warning(oss.str()); \
  }

#define LOG_ERROR_STREAM(msg)                        \
  {                                                  \
    std::ostringstream oss;                          \
    oss << msg;                                      \
    storage::Logger::getInstance().error(oss.str()); \
  }

}  // namespace storage
