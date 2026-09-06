#include "financial/common/generator.hpp"
#include "financial/optimized/pipeline.hpp"

#include <iostream>
#include <thread>

int main()
{
    const auto reference = financial::common::ReferenceData::make_default();
    const auto workload = financial::common::generate_workload({.records = 1000});
    const auto output = financial::optimized::process_records(
        workload.records,
        reference,
        {.worker_count = std::max(1u, std::thread::hardware_concurrency()), .queue_capacity = 16, .batch_size = 64});
    std::cout << "solution=02_lifetime_optimized\n";
    std::cout << "processed=" << output.pipeline.records.size() << '\n';
    std::cout << "checksum=" << output.pipeline.checksum << '\n';
    std::cout << "batch_size=" << output.batch_size << '\n';
    return 0;
}
