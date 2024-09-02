cmake_minimum_required(VERSION 3.22)


#########################
# Global settings
#########################
set(CMAKE_C_COMPILER               "/usr/bin/clang")
set(CMAKE_CXX_COMPILER             "/usr/bin/clang++")
set(CMAKE_CXX_STANDARD              20)
set(CMAKE_CXX_STANDARD_REQUIRED     ON)
set(CMAKE_CXX_FLAGS_RELEASE        "-O3")

project(evetest)
find_package(OpenCV REQUIRED)
find_package(HWY 1.2.0 REQUIRED)

# Common compile options and include directories
set(COMMON_COMPILE_OPTIONS "-std=c++20" "-O3" "-fno-omit-frame-pointer" "-march=native")
set(COMMON_INCLUDE_DIRS ${OpenCV_INCLUDE_DIRS} "include" "src")

#########################
# Fetch all dependencies
#########################
include(FetchContent)

FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
)

# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

FetchContent_Declare(
  googlebenchmark
  URL https://github.com/google/benchmark/archive/refs/tags/v1.9.0.zip
)
FetchContent_MakeAvailable(googlebenchmark)

FetchContent_Declare(
  fmt
  GIT_REPOSITORY https://github.com/fmtlib/fmt
  GIT_TAG        e69e5f977d458f2650bb346dadf2ad30c5320281) # 10.2.1
FetchContent_MakeAvailable(fmt)

FetchContent_Declare(
  flamegraph
  GIT_REPOSITORY https://github.com/brendangregg/FlameGraph.git
  GIT_TAG        cd9ee4c4449775a2f867acf31c84b7fe4b132ad5)
FetchContent_MakeAvailable(flamegraph)

#########################
# apriltag
#########################
add_library(apriltag SHARED src/ccl/disjoint_set.cpp)
target_compile_options(apriltag PUBLIC)
target_link_libraries(apriltag fmt::fmt hwy ${OpenCV_LIBS})
target_include_directories(apriltag PRIVATE ${OpenCV_INCLUDE_DIRS} "include")

#########################
# evetest
#########################
add_executable(evetest main.cpp)
target_compile_options(evetest PUBLIC ${COMMON_COMPILE_OPTIONS})
target_link_libraries(evetest fmt::fmt hwy ${OpenCV_LIBS} apriltag)
target_include_directories(evetest PRIVATE ${OpenCV_INCLUDE_DIRS} "include")

#########################
# test
#########################
enable_testing()

add_executable(
  disjoint_set_test
  test/disjoint_set.cpp
)
target_compile_options(disjoint_set_test PUBLIC ${COMMON_COMPILE_OPTIONS})
target_link_libraries(
  disjoint_set_test
  GTest::gtest_main
  apriltag
)

target_include_directories(disjoint_set_test PRIVATE ${OpenCV_INCLUDE_DIRS} "include" "src")

include(GoogleTest)
gtest_discover_tests(disjoint_set_test)

#########################
# benchmarks
#########################

add_executable(
  disjoint_set_bench
  benchmark/ccl/disjoint_set.cpp
)
target_include_directories(disjoint_set_bench PRIVATE ${OpenCV_INCLUDE_DIRS} "include" "src")
target_link_libraries(disjoint_set_bench benchmark::benchmark ${OpenCV_LIBS} apriltag)

add_executable(
  bmrs
  benchmark/ccl/bmrs.cpp
)
target_compile_options(bmrs PUBLIC -save-temps ${COMMON_COMPILE_OPTIONS})
target_include_directories(bmrs PRIVATE ${OpenCV_INCLUDE_DIRS} "include" "src")
target_link_libraries(bmrs benchmark::benchmark ${OpenCV_LIBS} apriltag)
target_compile_definitions(bmrs PUBLIC CMAKE_PROJECT_SOURCE_DIR="${PROJECT_SOURCE_DIR}")

add_custom_target(assembly
    COMMAND ${CMAKE_CXX_COMPILER} -S -fverbose-asm -g ${CMAKE_CURRENT_SOURCE_DIR}/benchmark/ccl/bmrs.cpp -o ${CMAKE_BINARY_DIR}/bmrs.s
    DEPENDS bmrs
)

#########################
# Flame Graph Generation
#########################

set(FLAMEGRAPH_DIR "${flamegraph_SOURCE_DIR}")
set(FLAMEGRAPH_SCRIPT "${FLAMEGRAPH_DIR}/flamegraph.pl")

add_custom_target(flamegraph
    COMMAND ${CMAKE_COMMAND} -E echo "Running benchmark and generating flame graph..."
    COMMAND perf record -F 99 -a -g -- ${CMAKE_BINARY_DIR}/bmrs
    COMMAND perf script | ${FLAMEGRAPH_DIR}/stackcollapse-perf.pl > out.perf-folded
    COMMAND ${FLAMEGRAPH_SCRIPT} out.perf-folded > flamegraph.svg
    COMMAND ${CMAKE_COMMAND} -E echo "Flame graph generated: ${CMAKE_BINARY_DIR}/flamegraph.svg"
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/flamegraph
    DEPENDS bmrs
    COMMENT "Generating flame graph for bmrs benchmark"
)
