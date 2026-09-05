cmake_minimum_required(VERSION 3.24)

project(throughput_cpp_lab VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(THROUGHPUT_ENABLE_WARNINGS "Enable project warnings" ON)

add_library(throughput_core
    src/checksum.cpp
    src/domain.cpp
    src/generator.cpp
    src/instrumentation.cpp
    src/pipeline_baseline.cpp
    src/pipeline_reserved.cpp
    src/reference_data.cpp
    src/stages.cpp
)

target_include_directories(throughput_core PUBLIC include)

if(THROUGHPUT_ENABLE_WARNINGS)
    if(MSVC)
        target_compile_options(throughput_core PRIVATE /W4 /permissive-)
    else()
        target_compile_options(throughput_core PRIVATE -Wall -Wextra -Wpedantic)
    endif()
endif()

add_executable(throughput_scenario benchmarks/scenarios/throughput_scenario.cpp)
target_link_libraries(throughput_scenario PRIVATE throughput_core)

add_executable(throughput_tests
    tests/baseline_tests.cpp
    tests/test_main.cpp
)
target_link_libraries(throughput_tests PRIVATE throughput_core)

set_target_properties(throughput_scenario throughput_tests PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

enable_testing()
add_test(NAME throughput_tests COMMAND throughput_tests)
