cmake_minimum_required(VERSION 3.24)

project(throughput_cpp_lab VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(THROUGHPUT_ENABLE_WARNINGS "Enable project warnings" ON)

enable_testing()

add_library(financial_common
    common/src/checksum.cpp
    common/src/generator.cpp
    common/src/instrumentation.cpp
    common/src/reference_data.cpp
    common/src/wire.cpp
)
target_include_directories(financial_common PUBLIC common/include)

add_library(financial_oracle_lib
    reference/sequential_oracle/src/pipeline.cpp
)
target_include_directories(financial_oracle_lib PUBLIC reference/sequential_oracle/include)
target_link_libraries(financial_oracle_lib PUBLIC financial_common)

add_executable(financial_oracle reference/sequential_oracle/src/main.cpp)
target_link_libraries(financial_oracle PRIVATE financial_oracle_lib)

add_executable(financial_oracle_benchmark reference/sequential_oracle/benchmarks/oracle_benchmark.cpp)
target_link_libraries(financial_oracle_benchmark PRIVATE financial_oracle_lib)

add_executable(financial_oracle_tests reference/sequential_oracle/tests/oracle_tests.cpp)
target_link_libraries(financial_oracle_tests PRIVATE financial_oracle_lib)

set_target_properties(financial_oracle financial_oracle_benchmark financial_oracle_tests PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

if(THROUGHPUT_ENABLE_WARNINGS)
    foreach(target financial_common financial_oracle_lib financial_oracle financial_oracle_benchmark financial_oracle_tests)
        if(MSVC)
            target_compile_options(${target} PRIVATE /W4 /permissive- /EHsc)
        else()
            target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic)
        endif()
    endforeach()
endif()

add_test(NAME financial_oracle_tests COMMAND financial_oracle_tests)

add_library(financial_parallel_batched_lib
    solutions/01_parallel_batched/src/pipeline.cpp
)
target_include_directories(financial_parallel_batched_lib PUBLIC solutions/01_parallel_batched/include)
target_link_libraries(financial_parallel_batched_lib PUBLIC financial_common)

add_executable(financial_parallel_batched solutions/01_parallel_batched/src/main.cpp)
target_link_libraries(financial_parallel_batched PRIVATE financial_parallel_batched_lib)

add_executable(financial_parallel_batched_benchmark solutions/01_parallel_batched/benchmarks/parallel_batched_benchmark.cpp)
target_link_libraries(financial_parallel_batched_benchmark PRIVATE financial_parallel_batched_lib)

add_executable(financial_parallel_batched_tests solutions/01_parallel_batched/tests/parallel_batched_tests.cpp)
target_link_libraries(financial_parallel_batched_tests PRIVATE financial_parallel_batched_lib financial_oracle_lib)

set_target_properties(financial_parallel_batched financial_parallel_batched_benchmark financial_parallel_batched_tests PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

if(THROUGHPUT_ENABLE_WARNINGS)
    foreach(target financial_parallel_batched_lib financial_parallel_batched financial_parallel_batched_benchmark financial_parallel_batched_tests)
        if(MSVC)
            target_compile_options(${target} PRIVATE /W4 /permissive- /EHsc)
        else()
            target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic)
        endif()
    endforeach()
endif()

add_test(NAME financial_parallel_batched_tests COMMAND financial_parallel_batched_tests)

add_library(financial_lifetime_optimized_lib
    solutions/02_lifetime_optimized/src/pipeline.cpp
)
target_include_directories(financial_lifetime_optimized_lib PUBLIC solutions/02_lifetime_optimized/include)
target_link_libraries(financial_lifetime_optimized_lib PUBLIC financial_common)

add_executable(financial_lifetime_optimized solutions/02_lifetime_optimized/src/main.cpp)
target_link_libraries(financial_lifetime_optimized PRIVATE financial_lifetime_optimized_lib)

add_executable(financial_lifetime_optimized_benchmark solutions/02_lifetime_optimized/benchmarks/lifetime_optimized_benchmark.cpp)
target_link_libraries(financial_lifetime_optimized_benchmark PRIVATE financial_lifetime_optimized_lib)

add_executable(financial_lifetime_optimized_tests solutions/02_lifetime_optimized/tests/lifetime_optimized_tests.cpp)
target_link_libraries(financial_lifetime_optimized_tests PRIVATE financial_lifetime_optimized_lib financial_oracle_lib)

set_target_properties(financial_lifetime_optimized financial_lifetime_optimized_benchmark financial_lifetime_optimized_tests PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

if(THROUGHPUT_ENABLE_WARNINGS)
    foreach(target financial_lifetime_optimized_lib financial_lifetime_optimized financial_lifetime_optimized_benchmark financial_lifetime_optimized_tests)
        if(MSVC)
            target_compile_options(${target} PRIVATE /W4 /permissive- /EHsc)
        else()
            target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic)
        endif()
    endforeach()
endif()

add_test(NAME financial_lifetime_optimized_tests COMMAND financial_lifetime_optimized_tests)
