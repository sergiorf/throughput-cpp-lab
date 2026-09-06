#include "financial/common/generator.hpp"
#include "financial/par/pipeline.hpp"
#include "financial/seq/pipeline.hpp"

#include <iostream>
#include <stdexcept>

namespace {

void require(bool condition, const char* message)
{
    if (!condition) {
        throw std::runtime_error(message);
    }
}

void test_parallel_matches_sequential()
{
    const auto reference = financial::common::ReferenceData::make_default();
    const auto workload = financial::common::generate_workload({.records = 512, .missing_percent = 8, .malformed_percent = 5});
    const auto sequential = financial::seq::process_records(workload.records, reference);
    const auto parallel = financial::par::process_records(workload.records, reference, {.worker_count = 4, .queue_capacity = 17});
    require(parallel.pipeline.records == sequential.records, "parallel output must match sequential records");
    require(parallel.pipeline.checksum == sequential.checksum, "parallel checksum must match sequential checksum");
}

void test_parallel_deterministic_across_worker_counts()
{
    const auto reference = financial::common::ReferenceData::make_default();
    const auto workload = financial::common::generate_workload({.records = 257, .seed = 0xfeed1234ull});
    const auto one = financial::par::process_records(workload.records, reference, {.worker_count = 1, .queue_capacity = 3});
    const auto many = financial::par::process_records(workload.records, reference, {.worker_count = 6, .queue_capacity = 19});
    require(one.pipeline.records == many.pipeline.records, "parallel output must be independent of worker count");
    require(one.pipeline.checksum == many.pipeline.checksum, "parallel checksum must be independent of worker count");
}

void test_bounded_queue_reports_backpressure()
{
    const auto reference = financial::common::ReferenceData::make_default();
    const auto workload = financial::common::generate_workload({.records = 128, .large_text = true});
    const auto output = financial::par::process_records(workload.records, reference, {.worker_count = 1, .queue_capacity = 1});
    require(output.queue.max_occupancy <= 1, "bounded queue must respect configured capacity");
    require(output.pipeline.records.size() == workload.records.size(), "bounded queue run should complete");
}

} // namespace

int main()
{
    try {
        test_parallel_matches_sequential();
        test_parallel_deterministic_across_worker_counts();
        test_bounded_queue_reports_backpressure();
    } catch (const std::exception& ex) {
        std::cerr << ex.what() << '\n';
        return 1;
    }
    return 0;
}
