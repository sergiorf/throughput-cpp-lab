# Solution 1: Parallel Batched Baseline

This implementation is the article-facing baseline. It uses worker threads, a bounded queue, explicit backpressure, preserved output ordering, batch work items, conventional owning C++ records, and general heap allocation.

The producer groups encoded records into owning batches and pushes those batches into a bounded queue. Workers decode, normalize, enrich, calculate, validate, and encode-size ordinary records from each batch. Results are written into pre-sized output slots so the external order is deterministic.

This stage intentionally does not use PMR, arenas, borrowed input ranges, compact layouts, or reduced-copy representations. It asks what remains after the obvious throughput moves, parallelism and batching, are already present.

Main files:

- `include/financial/batched/pipeline.hpp`: public configuration, metrics, and processing API.
- `src/pipeline.cpp`: bounded batch queue, worker loop, conventional business pipeline.
- `benchmarks/parallel_batched_benchmark.cpp`: smoke benchmark with record, worker, queue, and batch-size arguments.
- `tests/parallel_batched_tests.cpp`: oracle equivalence, deterministic output, ordering, and backpressure checks.

Build and run from the repository root:

```powershell
cmake --preset default
cmake --build --preset default
ctest --preset default
.\build\default\bin\financial_parallel_batched.exe
.\build\default\bin\financial_parallel_batched_benchmark.exe 10000 4 32 64
```
