cmake_minimum_required(VERSION 3.22)


#########################
# Global settings
#########################
set(CMAKE_C_COMPILER               "/usr/bin/clang")
set(CMAKE_CXX_COMPILER             "/usr/bin/clang++")
set(CMAKE_CXX_STANDARD              20)
set(CMAKE_CXX_STANDARD_REQUIRED     ON)
set(CMAKE_CXX_FLAGS_RELEASE        "-O3")

option(WITH_TESTS "Build unit tests" ON)
option(WITH_BENCHMARKS "Compare various methods to eachother" ON)
option(WITH_TRACING "Build with tracy enabled for profiling" OFF)

project(evetest)
find_package(OpenCV REQUIRED)
find_package(HWY 1.2.0 REQUIRED)

# Common compile options and include directories
set(COMMON_COMPILE_OPTIONS "-std=c++20" "-fno-omit-frame-pointer" "-march=native")
set(COMMON_INCLUDE_DIRS ${OpenCV_INCLUDE_DIRS} "include" "src")
set(COMMON_LINK_TARGETS fmt::fmt hwy ${OpenCV_LIBS} Halide::Halide)
set(COMMON_TARGET_DEFINES CMAKE_PROJECT_SOURCE_DIR="${PROJECT_SOURCE_DIR}" CMAKE_PROJECT_BUILD_DIR="${CMAKE_BINARY_DIR}")

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

if(WITH_TESTS OR WITH_BENCHMARKS)
    FetchContent_Declare(
        apriltag
        GIT_REPOSITORY https://github.com/AprilRobotics/apriltag.git
        GIT_TAG v3.4.2
        GIT_SHALLOW TRUE
        GIT_PROGRESS TRUE
    )
    FetchContent_MakeAvailable(apriltag)
endif()

#########################
# Build halide deps
#########################

include(cmake/Halide.cmake)
add_halide_generator(halide_generators SOURCES
                    src/halide/halide_threshold.cpp
                    src/halide/halide_gradient_clusters.cpp
                    LINK_LIBRARIES Halide::Halide)

# TODO: Parallelism below is kept to 1 to try and better compare to apriltag
add_halide_library(adaptive_threshold FROM halide_generators
                   AUTOSCHEDULER Halide::Adams2019
                   PARAMS autoscheduler.parallelism=1
                   COMPILER_LOG yes STMT_HTML yes)

add_halide_library(halide_gradient_clusters FROM halide_generators
                   AUTOSCHEDULER Halide::Adams2019
                   PARAMS autoscheduler.parallelism=1
                   COMPILER_LOG yes STMT_HTML yes)

#########################
# Tracy
#########################
if(WITH_TRACING)
    FetchContent_Declare(
        tracy
        GIT_REPOSITORY https://github.com/wolfpld/tracy.git
        GIT_TAG master
        GIT_SHALLOW TRUE
        GIT_PROGRESS TRUE
    )
    FetchContent_MakeAvailable(tracy)

    set(COMMON_LINK_TARGETS ${COMMON_LINK_TARGETS} Tracy::TracyClient)
    set(COMMON_TARGET_DEFINES COMMON_TARGET_DEFINES TRACY_ENABLE TRACY_VERBOSE TRACY_NO_EXIT)
    set(TRACY_SOURCE ${tracy_SOURCE_DIR}/public/TracyClient.cpp)
    add_subdirectory(${tracy_SOURCE_DIR}/profiler ${CMAKE_BINARY_DIR}/tracy)
endif()

#########################
# simdtag
#########################
add_library(simdtag STATIC src/ccl/disjoint_set.cpp src/ccl/bmrs.cpp)
target_compile_options(simdtag PUBLIC)
target_compile_options(simdtag PUBLIC ${COMMON_COMPILE_OPTIONS})
target_include_directories(simdtag PRIVATE ${OpenCV_INCLUDE_DIRS} "include")
target_link_libraries(simdtag ${COMMON_LINK_TARGETS})
target_compile_definitions(simdtag PUBLIC ${COMMON_TARGET_DEFINES})

#########################
# evetest
#########################
add_executable(evetest main.cpp)
target_compile_options(evetest PUBLIC ${COMMON_COMPILE_OPTIONS})
target_link_libraries(evetest ${COMMON_LINK_TARGETS} simdtag)
target_compile_definitions(evetest PUBLIC ${COMMON_TARGET_DEFINES})
target_include_directories(evetest PRIVATE ${OpenCV_INCLUDE_DIRS} "include")

#########################
# test
#########################
if(WITH_TESTS)
    enable_testing()
    include(GoogleTest)

    file(GLOB_RECURSE test_source test/*_test.cpp test/ccl_samples.cpp)

    add_executable(simdtag_test ${test_source})
    target_compile_options(simdtag_test PUBLIC ${COMMON_COMPILE_OPTIONS})
    target_link_libraries(simdtag_test ${COMMON_LINK_TARGETS} GTest::gtest_main simdtag)
    target_compile_definitions(simdtag_test PUBLIC ${COMMON_TARGET_DEFINES})
    target_include_directories(simdtag_test PRIVATE ${OpenCV_INCLUDE_DIRS} "include" "src")

    gtest_discover_tests(simdtag_test)
endif()

#########################
# benchmarks
#########################

if(WITH_BENCHMARKS)
    # TODO: Merge benchmark executables
    add_executable(
        disjoint_set_bench
        benchmark/disjoint_set.cpp
    )
    target_include_directories(disjoint_set_bench PRIVATE ${OpenCV_INCLUDE_DIRS} "include" "src")
    target_link_libraries(disjoint_set_bench ${COMMON_LINK_TARGETS} simdtag benchmark::benchmark)
    target_compile_definitions(disjoint_set_bench PUBLIC ${COMMON_TARGET_DEFINES})

    add_executable(
        ccl_bench
        benchmark/ccl.cpp
        ${TRACY_SOURCE}
    )
    target_compile_options(ccl_bench PUBLIC -save-temps ${COMMON_COMPILE_OPTIONS})
    target_include_directories(ccl_bench PRIVATE ${OpenCV_INCLUDE_DIRS} "include" "src")
    target_link_libraries(ccl_bench benchmark::benchmark ${COMMON_LINK_TARGETS} simdtag apriltag)

    # add_custom_target(assembly
    #     COMMAND ${CMAKE_CXX_COMPILER} -S -fverbose-asm -g ${CMAKE_CURRENT_SOURCE_DIR}/benchmark/ccl.cpp -o ${CMAKE_BINARY_DIR}/ccl.s
    #     DEPENDS ccl_bench
    # )

    add_executable(
        threshold_bench
        benchmark/threshold_bench.cpp
        ${TRACY_SOURCE}
    )
    target_compile_options(threshold_bench PUBLIC -save-temps ${COMMON_COMPILE_OPTIONS})
    target_include_directories(threshold_bench PRIVATE ${OpenCV_INCLUDE_DIRS} "include" "src")
    target_link_libraries(threshold_bench benchmark::benchmark ${COMMON_LINK_TARGETS} simdtag apriltag adaptive_threshold)

    add_executable(
        gradient_clusters_bench
        benchmark/gradient_clusters_bench.cpp
        ${TRACY_SOURCE}
    )
    target_compile_options(gradient_clusters_bench PUBLIC -save-temps ${COMMON_COMPILE_OPTIONS})
    target_include_directories(gradient_clusters_bench PRIVATE ${OpenCV_INCLUDE_DIRS} "include" "src")
    target_link_libraries(gradient_clusters_bench benchmark::benchmark ${COMMON_LINK_TARGETS} simdtag apriltag adaptive_threshold halide_gradient_clusters)

endif()

#########################
# Flame Graph Generation
#########################

set(FLAMEGRAPH_DIR "${flamegraph_SOURCE_DIR}")
set(FLAMEGRAPH_SCRIPT "${FLAMEGRAPH_DIR}/flamegraph.pl")

add_custom_target(flamegraph
    # COMMAND ${CMAKE_COMMAND} -E echo "Running Flamegraph script..."
    COMMAND bash ${CMAKE_SOURCE_DIR}/script/flamegraph.sh ${FLAMEGRAPH_DIR} ${CMAKE_BINARY_DIR} ${CMAKE_BINARY_DIR}/bmrs
    DEPENDS bmrs
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Running perf and generating a flame graph"
)
