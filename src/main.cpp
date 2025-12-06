#include <iostream>

#include "common/logger.h"

int main() {
  // Initialize logger
  auto& logger = storage::Logger::getInstance();
  logger.setDebugMode(true);

  LOG_INFO("========================================");
  LOG_INFO("Storage Engine Starting");
  LOG_INFO("========================================");

  std::cout << "Hello, World!" << std::endl;

  LOG_INFO("Storage Engine Shutting Down");
  LOG_INFO("========================================");

  return 0;
}