cmake_minimum_required(VERSION 3.14)
project(NumaPortfolio CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Treat warnings as errors for production readiness
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -march=native -pthread")

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/include)

find_library(NUMA_LIBRARIES numa)
if(NOT NUMA_LIBRARIES)
    message(WARNING "NUMA library not found. NumaAllocator might fail to link if it uses libnuma.")
endif()

# Core Library (Static)
add_library(core_lib STATIC
    src/NumaAllocator.cpp
    src/OrderBook.cpp
    src/TSCUtils.cpp
    src/ThreadAffinity.cpp
    src/ITCH5Parser.cpp
    src/OUCH42.cpp
    src/PreTradeRisk.cpp
    src/AsyncLogger.cpp
)
target_link_libraries(core_lib PUBLIC ${NUMA_LIBRARIES})

# Fetch GoogleTest
include(FetchContent)
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG        v1.14.0
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()

# Add Subdirectories
add_subdirectory(tests)
add_subdirectory(benchmarks)
