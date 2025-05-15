cmake_minimum_required(VERSION 3.11 FATAL_ERROR)
project(CCTHarmonicsOptimizer VERSION 1.0.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Set build type
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug)
endif()

# Set output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Add include directories
include_directories(include)

# Ensure Python library path is included
link_directories(${Python3_LIBRARY_DIRS})

# Find and link external libraries
find_package(RatModels 2.017.7 REQUIRED)
find_package(Armadillo REQUIRED)
find_package(Boost REQUIRED COMPONENTS filesystem)
find_package(Eigen3 REQUIRED)

# find ZLIB
if(POLICY CMP0074)
  cmake_policy(SET CMP0074 NEW)
endif()
find_package(ZLIB REQUIRED)

# Fetch CCTools at configure time
include(FetchContent)

FetchContent_Declare(
  CCTools
  GIT_REPOSITORY https://github.com/olekuhlmann/CCTools.git
  GIT_TAG        main
)

FetchContent_MakeAvailable(CCTools)

# Fetch googletest
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/release-1.12.1.tar.gz
)

# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(googletest)

# Get all source files in src directory
file(GLOB CCTHARMONICSOPTIMIZER_SOURCES "src/*.cpp")
list(REMOVE_ITEM CCTHARMONICSOPTIMIZER_SOURCES "${CMAKE_SOURCE_DIR}/src/main.cpp")

# Create the main executable
add_executable(cct-harmonics-optimizer_main ${CMAKE_SOURCE_DIR}/src/main.cpp ${CCTHARMONICSOPTIMIZER_SOURCES})
target_include_directories(cct-harmonics-optimizer_main PRIVATE ${EIGEN3_INCLUDE_DIR})
target_link_libraries(cct-harmonics-optimizer_main PRIVATE CCTools)

# Get all test files in test directory
file(GLOB CCTHARMONICSOPTIMIZER_TEST_SOURCES "test/*.cpp")

# Enable testing
enable_testing()

# Create the test executable
add_executable(cct-harmonics-optimizer_test ${CCTHARMONICSOPTIMIZER_SOURCES} ${CCTHARMONICSOPTIMIZER_TEST_SOURCES})
target_include_directories(cct-harmonics-optimizer_test PRIVATE ${EIGEN3_INCLUDE_DIR})
target_link_libraries(cct-harmonics-optimizer_test PRIVATE GTest::gtest_main CCTools)

# Add tests
add_test(NAME cct-harmonics-optimizer_test COMMAND cct-harmonics-optimizer_test)

# Suppress DartConfiguration.tcl warnings
set(CMAKE_SUPPRESS_REGENERATION true)

# Disable CDash
set(CTEST_USE_LAUNCHERS OFF)
