#include "common/logger.h"

#include <gtest/gtest.h>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <thread>

class LoggerTest : public ::testing::Test {
 protected:
  void SetUp() override {
    // Set test log directory
    testLogDir_ = "test_logs";

    // Clean up any existing test logs BEFORE setting the directory
    if (std::filesystem::exists(testLogDir_)) {
      std::filesystem::remove_all(testLogDir_);
    }
    std::filesystem::create_directories(testLogDir_);

    // Now set the log directory (after cleanup)
    storage::Logger::getInstance().setLogDirectory(testLogDir_);
  }

  void TearDown() override {
    // Clean up test logs
    if (std::filesystem::exists(testLogDir_)) {
      std::filesystem::remove_all(testLogDir_);
    }
  }

  std::string readLogFile() {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    std::tm tm;
    localtime_r(&time, &tm);

    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%d");
    std::string date = oss.str();

    std::string filename = testLogDir_ + "/storage_" + date + ".log";

    if (!std::filesystem::exists(filename)) {
      return "";
    }

    std::ifstream file(filename);
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
  }

  std::string testLogDir_;
};

// Test log file creation
TEST_F(LoggerTest, LogFileCreation) {
  EXPECT_TRUE(std::filesystem::exists(testLogDir_));
  auto& logger = storage::Logger::getInstance();
  logger.setDebugMode(true);

  LOG_INFO("Test file creation");

  // Get today's date for filename
  auto now = std::chrono::system_clock::now();
  auto time = std::chrono::system_clock::to_time_t(now);
  std::tm tm;
  localtime_r(&time, &tm);

  std::ostringstream oss;
  oss << std::put_time(&tm, "%Y-%m-%d");
  std::string date = oss.str();

  std::string expectedFilename = testLogDir_ + "/storage_" + date + ".log";

  EXPECT_TRUE(std::filesystem::exists(expectedFilename));
}

// Test debug mode toggle
TEST_F(LoggerTest, DebugModeToggle) {
  auto& logger = storage::Logger::getInstance();

  EXPECT_FALSE(logger.isDebugMode());  // Default is false

  logger.setDebugMode(true);
  EXPECT_TRUE(logger.isDebugMode());

  logger.setDebugMode(false);
  EXPECT_FALSE(logger.isDebugMode());
}

// Test basic logging functionality
TEST_F(LoggerTest, BasicLogging) {
  auto& logger = storage::Logger::getInstance();

  // Enable debug mode to see all logs
  logger.setDebugMode(true);

  // Log messages at different levels
  logger.info("Test info message");
  logger.warning("Test warning message");
  logger.error("Test error message");

  // Read log file
  std::string logContent = readLogFile();

  // Verify all messages are present
  EXPECT_TRUE(logContent.find("Test info message") != std::string::npos);
  EXPECT_TRUE(logContent.find("Test warning message") != std::string::npos);
  EXPECT_TRUE(logContent.find("Test error message") != std::string::npos);
}

// Test log levels in debug mode
TEST_F(LoggerTest, DebugModeLogsAllLevels) {
  auto& logger = storage::Logger::getInstance();
  logger.setDebugMode(true);

  LOG_INFO("Info in debug mode");
  LOG_WARNING("Warning in debug mode");
  LOG_ERROR("Error in debug mode");

  std::string logContent = readLogFile();

  EXPECT_TRUE(logContent.find("[INFO]") != std::string::npos);
  EXPECT_TRUE(logContent.find("[WARNING]") != std::string::npos);
  EXPECT_TRUE(logContent.find("[ERROR]") != std::string::npos);
}

// Test log levels in non-debug mode
TEST_F(LoggerTest, NonDebugModeFiltersInfo) {
  auto& logger = storage::Logger::getInstance();
  logger.setDebugMode(false);

  LOG_INFO("This should not appear");
  LOG_WARNING("This should appear");
  LOG_ERROR("This should also appear");

  std::string logContent = readLogFile();

  // INFO should not be logged
  EXPECT_TRUE(logContent.find("This should not appear") == std::string::npos);

  // WARNING and ERROR should be logged
  EXPECT_TRUE(logContent.find("This should appear") != std::string::npos);
  EXPECT_TRUE(logContent.find("This should also appear") != std::string::npos);
}

// Test stream-style logging macros
TEST_F(LoggerTest, StreamStyleLogging) {
  auto& logger = storage::Logger::getInstance();
  logger.setDebugMode(true);

  int value = 42;
  std::string name = "test";

  LOG_INFO_STREAM("Value is " << value << " and name is " << name);
  LOG_WARNING_STREAM("Warning: " << value);
  LOG_ERROR_STREAM("Error code: " << value);

  std::string logContent = readLogFile();

  EXPECT_TRUE(logContent.find("Value is 42 and name is test") !=
              std::string::npos);
  EXPECT_TRUE(logContent.find("Warning: 42") != std::string::npos);
  EXPECT_TRUE(logContent.find("Error code: 42") != std::string::npos);
}

// Test timestamp format
TEST_F(LoggerTest, TimestampFormat) {
  auto& logger = storage::Logger::getInstance();
  logger.setDebugMode(true);

  LOG_INFO("Timestamp test");

  std::string logContent = readLogFile();

  // Check for timestamp pattern [YYYY-MM-DD HH:MM:SS.mmm]
  // We'll just check that it contains date pattern and time pattern
  EXPECT_TRUE(logContent.find("[20") !=
              std::string::npos);  // Year starts with 20
  EXPECT_TRUE(logContent.find(":") != std::string::npos);  // Time separator
  EXPECT_TRUE(logContent.find(".") !=
              std::string::npos);  // Milliseconds separator
}

// Test thread safety
TEST_F(LoggerTest, ThreadSafety) {
  auto& logger = storage::Logger::getInstance();
  logger.setDebugMode(true);

  std::vector<std::thread> threads;
  const int numThreads = 10;
  const int messagesPerThread = 50;

  for (int i = 0; i < numThreads; i++) {
    threads.emplace_back([i]() {
      for (int j = 0; j < messagesPerThread; j++) {
        LOG_INFO_STREAM("Thread " << i << " message " << j);
      }
    });
  }

  for (auto& thread : threads) {
    thread.join();
  }

  std::string logContent = readLogFile();

  // Count the number of log entries
  size_t count = 0;
  size_t pos = 0;
  while ((pos = logContent.find("[INFO]", pos)) != std::string::npos) {
    count++;
    pos++;
  }

  // Should have exactly numThreads * messagesPerThread messages
  EXPECT_EQ(count, numThreads * messagesPerThread);
}