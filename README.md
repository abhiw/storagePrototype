# Storage Engine

A high-performance storage engine implementation in C++17.

## Project Structure

```
storageEngine/
├── CMakeLists.txt          # Main CMake configuration
├── src/                    # Source directory
│   ├── main.cpp           # Main application
│   ├── buffer/            # Buffer management module
│   ├── page/              # Page management module
│   ├── tuple/             # Tuple management module
│   ├── schema/            # Schema management module
│   ├── disk/              # Disk I/O module
│   └── engine/            # Storage engine core
├── build/
│   ├── debug/             # Debug build directory
│   └── release/           # Release build directory
└── .gitignore
```

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

### Release Build

```bash
mkdir -p build/release && cd build/release
cmake -DCMAKE_BUILD_TYPE=Release ../..
cmake --build .
./storage_engine
```

### Quick Rebuild

```bash
# From project root
cd build/debug
cmake --build .
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

## CMake Options

| Option | Default | Description |
|--------|---------|-------------|
| `CMAKE_BUILD_TYPE` | Debug | Build type (Debug/Release) |
| `ENABLE_ASAN` | ON | Enable AddressSanitizer |
| `ENABLE_TSAN` | OFF | Enable ThreadSanitizer |
| `ENABLE_UBSAN` | OFF | Enable UndefinedBehaviorSanitizer |
| `ENABLE_TESTING` | ON | Enable unit tests |
| `ENABLE_BENCHMARKS` | ON | Enable performance benchmarks |

### Example: Disable Testing

```bash
cmake -DCMAKE_BUILD_TYPE=Release -DENABLE_TESTING=OFF ../..
```

## Custom CMake Targets

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

## Adding New Files

This project uses **manual source file registration** for explicit control over compiled sources.

### Steps to Add New Files:

#### 1. Create the files

```bash
# Example: Adding a buffer pool manager
touch src/buffer/buffer_pool_manager.h
touch src/buffer/buffer_pool_manager.cpp
```

#### 2. Register the .cpp file in CMakeLists.txt

Edit `CMakeLists.txt` and add the `.cpp` file to the `STORAGE_ENGINE_SOURCES` variable:

```cmake
set(STORAGE_ENGINE_SOURCES
    src/main.cpp
    src/buffer/buffer_pool_manager.cpp  # ← Add this line
)
```

**Important:**
- ✅ Only add `.cpp` files
- ❌ Do NOT add `.h` header files (auto-detected via `#include`)
- Use paths relative to project root: `src/module/file.cpp`

#### 3. Build

```bash
cd build/debug
cmake --build .  # CMake auto-detects CMakeLists.txt changes
```

### Example: Adding Multiple Files

```cmake
set(STORAGE_ENGINE_SOURCES
    src/main.cpp

    # Buffer module
    src/buffer/buffer_pool_manager.cpp
    src/buffer/lru_replacer.cpp

    # Page module
    src/page/page.cpp
    src/page/page_guard.cpp

    # Disk module
    src/disk/disk_manager.cpp
)
```

### Using Headers

Headers are automatically available via the include path. Include them relative to `src/`:

```cpp
// In src/buffer/buffer_pool_manager.cpp
#include "buffer/buffer_pool_manager.h"  // Same module
#include "page/page.h"                   // Different module
#include "disk/disk_manager.h"           // Another module
```

## Development Workflow

### Typical Development Cycle

```bash
# 1. Create new files
touch src/buffer/new_feature.h
touch src/buffer/new_feature.cpp

# 2. Edit CMakeLists.txt
# Add: src/buffer/new_feature.cpp

# 3. Write code
# ... implement new_feature.cpp ...

# 4. Build and test
cd build/debug
cmake --build .
./storage_engine

# 5. Run with sanitizers (optional)
cd ../asan
cmake --build .
./storage_engine
```

### Clean Build

```bash
# Option 1: Use custom CMake target (recommended)
cd build/debug
cmake --build . --target clean-all
cmake ../..  # Reconfigure
cmake --build .  # Rebuild

# Option 2: Manual cleanup - remove entire build directory
rm -rf build/debug
mkdir -p build/debug && cd build/debug
cmake -DCMAKE_BUILD_TYPE=Debug ../..
cmake --build .
```

## IDE Integration

### CLion

CLion automatically detects CMakeLists.txt. Just open the project folder.

- **Debug configurations** are auto-generated
- **Sanitizers** can be enabled in Run/Debug Configurations
- **compile_commands.json** is exported for better code analysis

### VSCode

Install the CMake Tools extension and configure:

```json
{
  "cmake.buildDirectory": "${workspaceFolder}/build/debug",
  "cmake.configureSettings": {
    "CMAKE_BUILD_TYPE": "Debug"
  }
}
```

## Testing

Unit tests will be added using GoogleTest framework.

```bash
# To run tests (when implemented):
cd build/debug
ctest --output-on-failure
```

## Benchmarking

Performance benchmarks will use Google Benchmark.

```bash
# To run benchmarks (when implemented):
cd build/release
./benchmarks/storage_benchmark
```

## License

[Add your license here]
