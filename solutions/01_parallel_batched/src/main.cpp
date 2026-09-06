#include "financial/common/generator.hpp"
#include "financial/batched/pipeline.hpp"

#include <iostream>

int main()
{
    const auto reference = financial::common::ReferenceData::make_default();
    const auto workload = financial::common::generate_workload({.records = 1000});
    const auto output = financial::batched::process_records(
        workload.records, reference, {.worker_count = 4, .queue_capacity = 32, .batch_size = 64});
    std::cout << "solution=01_parallel_batched\n";
    std::cout << "records=" << output.pipeline.records.size() << '\n';
    std::cout << "input_bytes=" << output.pipeline.input_bytes << '\n';
    std::cout << "output_bytes=" << output.pipeline.output_bytes << '\n';
    std::cout << "checksum=" << output.pipeline.checksum << '\n';
    std::cout << "worker_count=" << output.worker_count << '\n';
    std::cout << "queue_capacity=" << output.queue_capacity << '\n';
    std::cout << "batch_size=" << output.batch_size << '\n';
    std::cout << "queue_max_occupancy=" << output.queue.max_occupancy << '\n';
    return output.pipeline.records.size() == workload.records.size() ? 0 : 1;
}
