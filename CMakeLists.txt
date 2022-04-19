cmake_minimum_required(VERSION 3.20)
project(spindle CXX)

set(CMAKE_CXX_STANDARD 14)

enable_testing()

# Directories
set(SPINDLE_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(SPINDLE_TEST_DIR ${CMAKE_CURRENT_SOURCE_DIR}/test)
set(SPINDLE_BENCHMARK_DIR ${CMAKE_CURRENT_SOURCE_DIR}/bench)

set(SPINDLE_THIRD_PARTY_DIR ${CMAKE_CURRENT_SOURCE_DIR}/third_party)
set(SPINDLE_GOOGLETEST_DIR ${SPINDLE_THIRD_PARTY_DIR}/googletest)
set(SPINDLE_GOOGLEBENCHMARK_DIR ${SPINDLE_THIRD_PARTY_DIR}/benchmark)

# Dependencies
add_subdirectory(${SPINDLE_GOOGLETEST_DIR} EXCLUDE_FROM_ALL)
add_subdirectory(${SPINDLE_GOOGLEBENCHMARK_DIR} EXCLUDE_FROM_ALL)

# Project version
file(READ VERSION SPINDLE_PROJECT_VERSION)
string(STRIP ${SPINDLE_PROJECT_VERSION} SPINDLE_PROJECT_VERSION)

# and commit
execute_process(COMMAND git describe --always --dirty
    OUTPUT_VARIABLE SPINDLE_GIT_COMMIT_HASH
    ERROR_VARIABLE SPINDLE_GIT_COMMIT_ERROR
    OUTPUT_STRIP_TRAILING_WHITESPACE
    RESULT_VARIABLE SPINDLE_GIT_COMMIT_RESULT
)
if (NOT ${SPINDLE_GIT_COMMIT_RESULT} STREQUAL 0)
    message(WARNING "Could not retrieve git commit: " ${SPINDLE_GIT_COMMIT_ERROR})
    set(SPINDLE_GIT_COMMIT_HASH unknown)
endif()

message("spindle version information: " ${SPINDLE_PROJECT_VERSION} " - " ${SPINDLE_GIT_COMMIT_HASH})
configure_file(${SPINDLE_SRC_DIR}/version.h.in ${CMAKE_CURRENT_BINARY_DIR}/gen/version.h @ONLY)

# Source files
set(SPINDLE_SRC_LIST
    ${SPINDLE_SRC_DIR}/latch.cpp
    ${SPINDLE_SRC_DIR}/spindle.cpp
    ${SPINDLE_SRC_DIR}/thread_pool.cpp
    ${SPINDLE_SRC_DIR}/worker.cpp
)

set(SPINDLE_BENCHMARK_LIST
    ${SPINDLE_BENCHMARK_DIR}/thread_pool_bench.cpp
)

# Test files
set(SPINDLE_TEST_LIST
    ${SPINDLE_TEST_DIR}/latch_test.cpp
    ${SPINDLE_TEST_DIR}/spindle_test.cpp
    ${SPINDLE_TEST_DIR}/thread_pool_test.cpp
    ${SPINDLE_TEST_DIR}/worker_test.cpp
)

add_library(spindle-lib ${SPINDLE_SRC_LIST})
target_include_directories(spindle-lib PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/gen)
set_target_properties(spindle-lib PROPERTIES OUTPUT_NAME spindle)

add_executable(spindle-tests ${SPINDLE_TEST_LIST})
target_include_directories(spindle-tests PRIVATE ${SPINDLE_SRC_DIR})
target_link_libraries(spindle-tests spindle-lib gtest_main)

add_test(unit-tests spindle-tests)

add_executable(spindle-benchmarks ${SPINDLE_BENCHMARK_LIST})
target_include_directories(spindle-benchmarks PRIVATE ${SPINDLE_SRC_DIR})
target_link_libraries(spindle-benchmarks spindle-lib benchmark_main)

add_test(spindle-benchmarks spindle-benchmarks)
