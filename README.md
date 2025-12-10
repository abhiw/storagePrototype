# Storage Engine

A prototype storage engine implementation in C++17 for learning purposes.

**Note:** This is a learning project to explore storage engine concepts and C++ development practices.


## Requirements

- CMake 3.15 or higher
- C++17 compatible compiler
- Git (for fetching dependencies)

## Build Configuration

### Compiler Flags

- **Base flags:** `-Wall -Wextra -Wpedantic -Werror`
- **Debug flags:** `-g -O0`
- **Release flags:** `-O3 -march=native -DNDEBUG`

### Integrated Libraries

- **GoogleTest v1.15.2** - Unit testing framework
- **Google Benchmark v1.9.1** - Performance benchmarking

## Building the Project

### Debug Build

```bash
mkdir -p build/debug && cd build/debug
cmake -DCMAKE_BUILD_TYPE=Debug ../..
cmake --build .
./storage_engine
```

## Sanitizers

Enable sanitizers for debugging (only use one at a time):

### AddressSanitizer (Memory errors)

```bash
mkdir -p build/asan && cd build/asan
cmake -DCMAKE_BUILD_TYPE=Debug -DENABLE_ASAN=ON ../..
cmake --build .
```

### ThreadSanitizer (Race conditions)

```bash
mkdir -p build/tsan && cd build/tsan
cmake -DCMAKE_BUILD_TYPE=Debug -DENABLE_TSAN=ON ../..
cmake --build .
```

### UndefinedBehaviorSanitizer

```bash
mkdir -p build/ubsan && cd build/ubsan
cmake -DCMAKE_BUILD_TYPE=Debug -DENABLE_UBSAN=ON ../..
cmake --build .
```

**Note:** AddressSanitizer and ThreadSanitizer cannot be enabled simultaneously.


### clean-all

Cleans all build artifacts without removing the build directory:

```bash
cd build/debug
cmake --build . --target clean-all
```

This removes:
- CMakeCache.txt
- CMakeFiles/
- Makefile
- cmake_install.cmake
- _deps/ (downloaded dependencies)
- lib/ (built libraries)
- storage_engine (executable)

After cleaning, reconfigure and rebuild:
```bash
cmake ../..
cmake --build .
```

#### 1. Register the .cpp file in CMakeLists.txt

Edit `CMakeLists.txt` and add the `.cpp` file to the `STORAGE_ENGINE_SOURCES` variable:

```cmake
set(STORAGE_ENGINE_SOURCES
    src/main.cpp
    src/buffer/buffer_pool_manager.cpp  # ← Add this line
)
```

#### 2. Build

```bash
cd build/debug
cmake --build .  # CMake auto-detects CMakeLists.txt changes
```


## Testing

Unit tests have been added using GoogleTest framework.

```bash
# To run tests:
cd build/debug
ctest --output-on-failure
```
