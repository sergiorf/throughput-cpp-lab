#include "financial/common/generator.hpp"
#include "financial/batched/pipeline.hpp"
#include "financial/oracle/pipeline.hpp"

#include <iostream>
#include <stdexcept>

namespace {

void require(bool condition, const char* message)
{
    if (!condition) {
        throw std::runtime_error(message);
    }
}

void test_parallel_batched_matches_oracle()
{
    const auto reference = financial::common::ReferenceData::make_default();
    const auto workload = financial::common::generate_workload({.records = 512, .missing_percent = 8, .malformed_percent = 5});
    const auto oracle = financial::oracle::process_records(workload.records, reference);
    const auto batched = financial::batched::process_records(
        workload.records, reference, {.worker_count = 4, .queue_capacity = 3, .batch_size = 17});
    require(batched.pipeline.records == oracle.records, "batched output must match oracle records");
    require(batched.pipeline.checksum == oracle.checksum, "batched checksum must match oracle checksum");
}

void test_parallel_batched_deterministic_across_worker_counts_and_batches()
{
    const auto reference = financial::common::ReferenceData::make_default();
    const auto workload = financial::common::generate_workload({.records = 257, .seed = 0xfeed1234ull});
    const auto one = financial::batched::process_records(
        workload.records, reference, {.worker_count = 1, .queue_capacity = 3, .batch_size = 1});
    const auto many = financial::batched::process_records(
        workload.records, reference, {.worker_count = 6, .queue_capacity = 5, .batch_size = 41});
    require(one.pipeline.records == many.pipeline.records, "batched output must be independent of worker count and batch size");
    require(one.pipeline.checksum == many.pipeline.checksum, "batched checksum must be independent of worker count and batch size");
}

void test_bounded_queue_reports_backpressure()
{
    const auto reference = financial::common::ReferenceData::make_default();
    const auto workload = financial::common::generate_workload({.records = 128, .large_text = true});
    const auto output = financial::batched::process_records(
        workload.records, reference, {.worker_count = 1, .queue_capacity = 1, .batch_size = 8});
    require(output.queue.max_occupancy <= 1, "bounded queue must respect configured capacity");
    require(output.pipeline.records.size() == workload.records.size(), "bounded queue run should complete");
}

} // namespace

int main()
{
    try {
        test_parallel_batched_matches_oracle();
        test_parallel_batched_deterministic_across_worker_counts_and_batches();
        test_bounded_queue_reports_backpressure();
    } catch (const std::exception& ex) {
        std::cerr << ex.what() << '\n';
        return 1;
    }
    return 0;
}
