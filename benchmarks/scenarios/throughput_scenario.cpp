#include "throughput/generator.hpp"
#include "throughput/instrumentation.hpp"
#include "throughput/pipeline_baseline.hpp"
#include "throughput/reference_data.hpp"

#include <chrono>
#include <iomanip>
#include <iostream>

int main()
{
    using clock = std::chrono::steady_clock;

    const throughput::WorkloadConfig config{
        .records = 10'000,
        .seed = 0x5eed1234ull,
        .oversized_percent = 1,
        .escaping_percent = 0,
        .max_attributes = 8,
        .max_relationships = 4,
        .skewed = true,
    };

    const auto reference = throughput::ReferenceData::make_default();
    const auto batch = throughput::generate_batch(config);

    throughput::reset_allocation_counters();
    const throughput::AllocationScope allocations;
    const auto start = clock::now();
    const auto result = throughput::baseline::process_batch(batch, reference);
    const auto stop = clock::now();
    const auto allocation_snapshot = allocations.snapshot();

    const auto elapsed = std::chrono::duration<double>(stop - start).count();
    const auto records_per_second = static_cast<double>(batch.events.size()) / elapsed;

    std::cout << "stage=baseline\n";
    std::cout << "records=" << batch.events.size() << '\n';
    std::cout << "input_bytes=" << batch.total_bytes << '\n';
    std::cout << "elapsed_seconds=" << std::fixed << std::setprecision(6) << elapsed << '\n';
    std::cout << "records_per_second=" << std::fixed << std::setprecision(2) << records_per_second << '\n';
    std::cout << "allocations=" << allocation_snapshot.allocations << '\n';
    std::cout << "allocated_bytes=" << allocation_snapshot.allocated_bytes << '\n';
    std::cout << "deallocations=" << allocation_snapshot.deallocations << '\n';
    std::cout << "deallocated_bytes=" << allocation_snapshot.deallocated_bytes << '\n';
    std::cout << "normalized_records=" << result.normalized.size() << '\n';
    std::cout << "audit_records=" << result.audit.size() << '\n';
    std::cout << "checksum=" << result.checksum << '\n';

    return result.normalized.size() == batch.events.size() && result.audit.size() == batch.events.size() ? 0 : 1;
}

