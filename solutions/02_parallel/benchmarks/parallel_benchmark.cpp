#include "financial/common/generator.hpp"
#include "financial/common/instrumentation.hpp"
#include "financial/par/pipeline.hpp"

#include <chrono>
#include <iomanip>
#include <iostream>
#include <string>
#include <thread>

int main(int argc, char** argv)
{
    const std::size_t records = argc > 1 ? static_cast<std::size_t>(std::stoull(argv[1])) : 10000;
    const std::size_t workers = argc > 2 ? static_cast<std::size_t>(std::stoull(argv[2])) : std::thread::hardware_concurrency();
    const std::size_t queue_capacity = argc > 3 ? static_cast<std::size_t>(std::stoull(argv[3])) : 256;
    const auto reference = financial::common::ReferenceData::make_default();
    const auto workload = financial::common::generate_workload({.records = records});

    financial::common::reset_allocation_counters();
    const financial::common::AllocationScope allocations;
    const auto start = std::chrono::steady_clock::now();
    const auto output = financial::par::process_records(
        workload.records, reference, {.worker_count = workers == 0 ? 1 : workers, .queue_capacity = queue_capacity});
    const auto stop = std::chrono::steady_clock::now();
    const auto snapshot = allocations.snapshot();
    const auto seconds = std::chrono::duration<double>(stop - start).count();

    std::cout << "solution=02_parallel\n";
    std::cout << "records=" << output.pipeline.records.size() << '\n';
    std::cout << "input_bytes=" << output.pipeline.input_bytes << '\n';
    std::cout << "output_bytes=" << output.pipeline.output_bytes << '\n';
    std::cout << "elapsed_seconds=" << std::fixed << std::setprecision(6) << seconds << '\n';
    std::cout << "records_per_second=" << std::fixed << std::setprecision(2)
              << static_cast<double>(output.pipeline.records.size()) / seconds << '\n';
    std::cout << "allocations=" << snapshot.allocations << '\n';
    std::cout << "allocated_bytes=" << snapshot.allocated_bytes << '\n';
    std::cout << "checksum=" << output.pipeline.checksum << '\n';
    std::cout << "worker_count=" << output.worker_count << '\n';
    std::cout << "queue_capacity=" << output.queue_capacity << '\n';
    std::cout << "queue_max_occupancy=" << output.queue.max_occupancy << '\n';
    std::cout << "producer_waits=" << output.queue.producer_waits << '\n';
    std::cout << "consumer_waits=" << output.queue.consumer_waits << '\n';
    std::cout << "producer_wait_ns=" << output.queue.producer_wait_time.count() << '\n';
    std::cout << "consumer_wait_ns=" << output.queue.consumer_wait_time.count() << '\n';
    return 0;
}
