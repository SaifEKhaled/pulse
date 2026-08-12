# ---------------------------------------------------------------------------
# PULSE — root build configuration
# ---------------------------------------------------------------------------

# Minimum CMake version. 3.20 is a safe modern baseline (widely available,
# supports the features we'll need later like FetchContent for dependencies).
cmake_minimum_required(VERSION 3.20)

# Declares the project name and languages used. This also implicitly sets
# up compiler detection for C++.
project(pulse LANGUAGES CXX)

# ---------------------------------------------------------------------------
# Language standard
# ---------------------------------------------------------------------------
# We use C++20: gives us concepts, ranges, and better concurrency primitives
# (std::jthread, std::atomic improvements) — all genuinely useful for a
# storage engine with concurrent readers/writers.
set(CMAKE_CXX_STANDARD 20)
# Require it — don't silently fall back to an older standard if the compiler
# doesn't support C++20.
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# Disable compiler-specific extensions (e.g. GNU extensions) so the code
# stays portable across GCC/Clang/MSVC.
set(CMAKE_CXX_EXTENSIONS OFF)

# ---------------------------------------------------------------------------
# Build type default
# ---------------------------------------------------------------------------
# If no build type is specified (e.g. `cmake ..` with nothing else), default
# to Debug. This matters because CMake's default is actually *no* flags at
# all (no optimization, no debug symbols) unless a build type is set.
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

# ---------------------------------------------------------------------------
# Compiler warnings
# ---------------------------------------------------------------------------
# For a systems project like this, warnings are not optional noise — they
# catch real concurrency and memory bugs early. We turn on a strict set now,
# while the codebase is small, rather than retrofitting discipline later.
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# ---------------------------------------------------------------------------
# Output directories
# ---------------------------------------------------------------------------
# Force all compiled binaries into build/bin regardless of where the source
# file lives in the tree. Keeps output predictable and matches the
# build/ ignore rule we already set up in .gitignore.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# ---------------------------------------------------------------------------
# Subdirectories
# ---------------------------------------------------------------------------
# The actual source code lives in src/, which will have its own
# CMakeLists.txt defining the executable/library targets. Keeping the root
# file thin and delegating to src/ is standard practice — it keeps the root
# focused on project-wide settings only.
add_subdirectory(src)