#include "financial/common/generator.hpp"
#include "financial/common/instrumentation.hpp"
#include "financial/seq/pipeline.hpp"

#include <chrono>
#include <iomanip>
#include <iostream>
#include <string>

int main(int argc, char** argv)
{
    const std::size_t records = argc > 1 ? static_cast<std::size_t>(std::stoull(argv[1])) : 10000;
    const auto reference = financial::common::ReferenceData::make_default();
    const auto workload = financial::common::generate_workload({.records = records});

    financial::common::reset_allocation_counters();
    const financial::common::AllocationScope allocations;
    const auto start = std::chrono::steady_clock::now();
    const auto output = financial::seq::process_records(workload.records, reference);
    const auto stop = std::chrono::steady_clock::now();
    const auto snapshot = allocations.snapshot();
    const auto seconds = std::chrono::duration<double>(stop - start).count();

    std::cout << "solution=01_sequential\n";
    std::cout << "records=" << output.records.size() << '\n';
    std::cout << "input_bytes=" << output.input_bytes << '\n';
    std::cout << "output_bytes=" << output.output_bytes << '\n';
    std::cout << "elapsed_seconds=" << std::fixed << std::setprecision(6) << seconds << '\n';
    std::cout << "records_per_second=" << std::fixed << std::setprecision(2)
              << static_cast<double>(output.records.size()) / seconds << '\n';
    std::cout << "allocations=" << snapshot.allocations << '\n';
    std::cout << "allocated_bytes=" << snapshot.allocated_bytes << '\n';
    std::cout << "checksum=" << output.checksum << '\n';
    return 0;
}
