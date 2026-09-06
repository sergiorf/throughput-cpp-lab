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
        target_compile_options(throughput_core PRIVATE /W4 /permissive- /EHsc)
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

add_library(financial_common
    common/src/checksum.cpp
    common/src/generator.cpp
    common/src/instrumentation.cpp
    common/src/reference_data.cpp
    common/src/wire.cpp
)
target_include_directories(financial_common PUBLIC common/include)

add_library(financial_seq_lib
    solutions/01_sequential/src/pipeline.cpp
)
target_include_directories(financial_seq_lib PUBLIC solutions/01_sequential/include)
target_link_libraries(financial_seq_lib PUBLIC financial_common)

add_executable(financial_seq solutions/01_sequential/src/main.cpp)
target_link_libraries(financial_seq PRIVATE financial_seq_lib)

add_executable(financial_seq_benchmark solutions/01_sequential/benchmarks/sequential_benchmark.cpp)
target_link_libraries(financial_seq_benchmark PRIVATE financial_seq_lib)

add_executable(financial_seq_tests solutions/01_sequential/tests/sequential_tests.cpp)
target_link_libraries(financial_seq_tests PRIVATE financial_seq_lib)

set_target_properties(financial_seq financial_seq_benchmark financial_seq_tests PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

if(THROUGHPUT_ENABLE_WARNINGS)
    foreach(target financial_common financial_seq_lib financial_seq financial_seq_benchmark financial_seq_tests)
        if(MSVC)
            target_compile_options(${target} PRIVATE /W4 /permissive- /EHsc)
        else()
            target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic)
        endif()
    endforeach()
endif()

add_test(NAME financial_seq_tests COMMAND financial_seq_tests)

add_library(financial_par_lib
    solutions/02_parallel/src/pipeline.cpp
)
target_include_directories(financial_par_lib PUBLIC solutions/02_parallel/include)
target_link_libraries(financial_par_lib PUBLIC financial_common)

add_executable(financial_par solutions/02_parallel/src/main.cpp)
target_link_libraries(financial_par PRIVATE financial_par_lib)

add_executable(financial_par_benchmark solutions/02_parallel/benchmarks/parallel_benchmark.cpp)
target_link_libraries(financial_par_benchmark PRIVATE financial_par_lib)

add_executable(financial_par_tests solutions/02_parallel/tests/parallel_tests.cpp)
target_link_libraries(financial_par_tests PRIVATE financial_par_lib financial_seq_lib)

set_target_properties(financial_par financial_par_benchmark financial_par_tests PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

if(THROUGHPUT_ENABLE_WARNINGS)
    foreach(target financial_par_lib financial_par financial_par_benchmark financial_par_tests)
        if(MSVC)
            target_compile_options(${target} PRIVATE /W4 /permissive- /EHsc)
        else()
            target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic)
        endif()
    endforeach()
endif()

add_test(NAME financial_par_tests COMMAND financial_par_tests)

add_library(financial_tp_lib
    solutions/03_throughput/src/pipeline.cpp
)
target_include_directories(financial_tp_lib PUBLIC solutions/03_throughput/include)
target_link_libraries(financial_tp_lib PUBLIC financial_common)

add_executable(financial_tp solutions/03_throughput/src/main.cpp)
target_link_libraries(financial_tp PRIVATE financial_tp_lib)

add_executable(financial_tp_benchmark solutions/03_throughput/benchmarks/throughput_benchmark.cpp)
target_link_libraries(financial_tp_benchmark PRIVATE financial_tp_lib)

add_executable(financial_tp_tests solutions/03_throughput/tests/throughput_tests.cpp)
target_link_libraries(financial_tp_tests PRIVATE financial_tp_lib financial_seq_lib)

set_target_properties(financial_tp financial_tp_benchmark financial_tp_tests PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

if(THROUGHPUT_ENABLE_WARNINGS)
    foreach(target financial_tp_lib financial_tp financial_tp_benchmark financial_tp_tests)
        if(MSVC)
            target_compile_options(${target} PRIVATE /W4 /permissive- /EHsc)
        else()
            target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic)
        endif()
    endforeach()
endif()

add_test(NAME financial_tp_tests COMMAND financial_tp_tests)
