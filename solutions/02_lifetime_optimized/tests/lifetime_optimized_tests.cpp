#include "financial/common/generator.hpp"
#include "financial/optimized/pipeline.hpp"
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

void test_optimized_matches_oracle()
{
    const auto reference = financial::common::ReferenceData::make_default();
    const auto workload = financial::common::generate_workload({.records = 700, .missing_percent = 9, .malformed_percent = 5});
    const auto oracle = financial::oracle::process_records(workload.records, reference);
    const auto optimized = financial::optimized::process_records(
        workload.records,
        reference,
        {.worker_count = 4, .queue_capacity = 5, .batch_size = 31});
    require(optimized.pipeline.records == oracle.records, "optimized output must match oracle records");
    require(optimized.pipeline.checksum == oracle.checksum, "optimized checksum must match oracle checksum");
}

void test_optimized_deterministic_across_batching()
{
    const auto reference = financial::common::ReferenceData::make_default();
    const auto workload = financial::common::generate_workload({.records = 513, .seed = 0x7777aaaaull, .large_text = true});
    const auto small_batches = financial::optimized::process_records(
        workload.records,
        reference,
        {.worker_count = 2, .queue_capacity = 2, .batch_size = 1});
    const auto larger_batches = financial::optimized::process_records(
        workload.records,
        reference,
        {.worker_count = 6, .queue_capacity = 11, .batch_size = 64});
    require(small_batches.pipeline.records == larger_batches.pipeline.records, "batch sizing must not change output records");
    require(small_batches.pipeline.checksum == larger_batches.pipeline.checksum, "batch sizing must not change checksum");
}

void test_heap_and_arena_modes_match()
{
    const auto reference = financial::common::ReferenceData::make_default();
    const auto workload = financial::common::generate_workload({.records = 257, .missing_percent = 12});
    const auto heap = financial::optimized::process_records(
        workload.records,
        reference,
        {.worker_count = 3,
         .queue_capacity = 4,
         .batch_size = 17,
         .allocation_mode = financial::optimized::AllocationMode::ordinary});
    const auto arena = financial::optimized::process_records(
        workload.records,
        reference,
        {.worker_count = 3,
         .queue_capacity = 4,
         .batch_size = 17,
         .allocation_mode = financial::optimized::AllocationMode::batch_arena});
    require(heap.pipeline.records == arena.pipeline.records, "allocation mode must not change output records");
    require(heap.pipeline.checksum == arena.pipeline.checksum, "allocation mode must not change checksum");
}

void test_batch_queue_reports_backpressure()
{
    const auto reference = financial::common::ReferenceData::make_default();
    const auto workload = financial::common::generate_workload({.records = 256, .large_text = true});
    const auto output = financial::optimized::process_records(
        workload.records,
        reference,
        {.worker_count = 1, .queue_capacity = 1, .batch_size = 8});
    require(output.queue.max_occupancy <= 1, "batch queue must respect configured capacity");
    require(output.pipeline.records.size() == workload.records.size(), "batch queue run should complete");
}

} // namespace

int main()
{
    try {
        test_optimized_matches_oracle();
        test_optimized_deterministic_across_batching();
        test_heap_and_arena_modes_match();
        test_batch_queue_reports_backpressure();
    } catch (const std::exception& ex) {
        std::cerr << ex.what() << '\n';
        return 1;
    }
    return 0;
}
