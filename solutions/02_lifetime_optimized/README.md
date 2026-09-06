# Solution 2: Lifetime Optimized Pipeline

This implementation keeps the batch-oriented parallel boundary and changes the movement and lifetime of data inside it.

The producer walks the common encoded input vector and pushes input index ranges into a bounded queue. Each worker owns mutable scratch state and processes complete batches into pre-sized canonical output slots. The immutable reference tables are shared read-only across workers.

Input payloads are owned by the caller for the duration of `process_records`. Batch work items borrow them by index range only; no view escapes the call. Worker scratch memory is local to each worker. In `batch_arena` mode, normalized temporary strings are allocated from a per-worker `std::pmr::monotonic_buffer_resource` and the arena is reset after each batch.

Canonical output records use ordinary owning `std::string` values because they survive the batch and are used for checksum and equivalence tests.

Main files:

- `include/financial/optimized/pipeline.hpp`: public configuration, metrics, and processing API.
- `src/pipeline.cpp`: range queue, worker-local state, normalization, enrichment, financial calculations.
- `benchmarks/lifetime_optimized_benchmark.cpp`: smoke benchmark with worker, queue, batch, and allocation-mode switches.
- `tests/lifetime_optimized_tests.cpp`: equivalence, determinism, bounded queue, and arena-mode checks.

Build and run from the repository root:

```powershell
cmake --preset default
cmake --build --preset default
ctest --preset default
.\build\default\bin\financial_lifetime_optimized.exe
.\build\default\bin\financial_lifetime_optimized_benchmark.exe 10000 4 32 64 arena
```

Use `heap` instead of `arena` as the fifth benchmark argument to compare ordinary temporary allocation against batch-scoped PMR scratch allocation.
