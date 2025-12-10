cmake_minimum_required(VERSION 3.15)
project(StorageEngine CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Export compile commands for IDE integration
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(ENABLE_CLANG_TIDY "Enable clang-tidy during build" OFF)

# Configure clang-tidy only when explicitly enabled and available on the system.
if(ENABLE_CLANG_TIDY)
    find_program(CLANG_TIDY_EXE NAMES clang-tidy clang-tidy-15 clang-tidy-14)
    if(CLANG_TIDY_EXE)
        # Use a simple checks configuration by default. Avoid turning all warnings
        # into errors to prevent unexpected build failures. Users can override
        # `CMAKE_CXX_CLANG_TIDY` on the command line for custom flags.
        set(CMAKE_CXX_CLANG_TIDY "${CLANG_TIDY_EXE};-checks=*" CACHE STRING "clang-tidy executable and args")
        message(STATUS "clang-tidy integration enabled: ${CLANG_TIDY_EXE}")
    else()
        message(WARNING "clang-tidy requested (ENABLE_CLANG_TIDY=ON) but not found; skipping integration")
    endif()
endif()

# Build type configuration
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Build type (Debug or Release)" FORCE)
endif()

# Compiler flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic -Werror")

# Optimization flags
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -march=native -DNDEBUG")

# Optional sanitizer flags
option(ENABLE_ASAN "Enable AddressSanitizer" ON)
option(ENABLE_TSAN "Enable ThreadSanitizer" OFF)
option(ENABLE_UBSAN "Enable UndefinedBehaviorSanitizer" OFF)

if(ENABLE_ASAN)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address -fno-omit-frame-pointer")
    set(CMAKE_LINKER_FLAGS "${CMAKE_LINKER_FLAGS} -fsanitize=address")
endif()

if(ENABLE_TSAN)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=thread -fno-omit-frame-pointer")
    set(CMAKE_LINKER_FLAGS "${CMAKE_LINKER_FLAGS} -fsanitize=thread")
endif()

if(ENABLE_UBSAN)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=undefined -fno-omit-frame-pointer")
    set(CMAKE_LINKER_FLAGS "${CMAKE_LINKER_FLAGS} -fsanitize=undefined")
endif()

# Ensure mutually exclusive sanitizers
if(ENABLE_ASAN AND ENABLE_TSAN)
    message(FATAL_ERROR "AddressSanitizer and ThreadSanitizer cannot be enabled simultaneously")
endif()

# Include directories
include_directories(${PROJECT_SOURCE_DIR}/include)
include_directories(${PROJECT_SOURCE_DIR}/src)

# Source files
set(STORAGE_ENGINE_SOURCES
        src/main.cpp
        include/common/config.h
        include/common/types.h
        include/common/checksum.h
        src/common/checksum.cpp
        include/common/logger.h
        src/common/logger.cpp
        include/common/file_handle.h
        src/common/file_handle.cpp
        include/page/page.h
        src/page/page.cpp
        include/page/page_view.h
        src/page/page_view.cpp
        include/schema/alignment.h
        src/schema/alignment.cpp
        include/schema/schema.h
        src/schema/schema.cpp
        include/storage/disk_manager.h
        src/storage/disk_manager.cpp
        include/storage/free_space_map.h
        src/storage/free_space_map.cpp
        include/storage/page_manager.h
        src/storage/page_manager.cpp
        include/tuple/field_value.h
        src/tuple/field_value.cpp
        include/tuple/tuple_header.h
        src/tuple/tuple_header.cpp
        include/tuple/tuple_builder.h
        src/tuple/tuple_builder.cpp
        include/tuple/tuple_accessor.h
        src/tuple/tuple_accessor.cpp
        include/tuple/tuple_serializer.h
        src/tuple/tuple_serializer.cpp
        src/tuple/tuple_serializer.cpp
)

# Main executable
add_executable(storage_engine ${STORAGE_ENGINE_SOURCES})

# GoogleTest integration
option(ENABLE_TESTING "Enable unit tests" ON)

if(ENABLE_TESTING)
    enable_testing()

    # Fetch GoogleTest
    include(FetchContent)
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG v1.15.2
    )

    # For Windows: Prevent overriding the parent project's compiler/linker settings
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

    FetchContent_MakeAvailable(googletest)

    # Test subdirectory
    add_subdirectory(test)
endif()

# Google Benchmark integration
option(ENABLE_BENCHMARKS "Enable performance benchmarks" OFF)

if(ENABLE_BENCHMARKS)
    # Fetch Google Benchmark
    include(FetchContent)
    FetchContent_Declare(
        benchmark
        GIT_REPOSITORY https://github.com/google/benchmark.git
        GIT_TAG v1.9.1
    )

    set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
    set(BENCHMARK_ENABLE_GTEST_TESTS OFF CACHE BOOL "" FORCE)

    FetchContent_MakeAvailable(benchmark)

    # Benchmark subdirectory (if benchmarks exist)
    # add_subdirectory(benchmarks)
endif()

# Custom target to clean build directory
add_custom_target(clean-all
    COMMAND ${CMAKE_COMMAND} -E rm -rf
        ${CMAKE_BINARY_DIR}/CMakeCache.txt
        ${CMAKE_BINARY_DIR}/CMakeFiles
        ${CMAKE_BINARY_DIR}/Makefile
        ${CMAKE_BINARY_DIR}/cmake_install.cmake
        ${CMAKE_BINARY_DIR}/_deps
        ${CMAKE_BINARY_DIR}/lib
        ${CMAKE_BINARY_DIR}/storage_engine
    COMMENT "Cleaning build directory..."
)
