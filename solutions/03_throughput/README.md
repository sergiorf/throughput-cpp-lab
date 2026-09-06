# Solution 3: Throughput-Oriented Parallel Pipeline

This implementation changes the work unit from a single record to a bounded batch. Queue entries contain an input index range instead of owning record payloads, so the queue transfer is smaller and synchronization is amortized across many records.

## Architecture

The producer walks the common encoded input vector and pushes `BatchItem` ranges into a bounded queue. Each worker owns mutable scratch state and processes complete batches into pre-sized canonical output slots. The immutable reference tables are shared read-only across workers.

## Ownership And Lifetime

Input payloads are owned by the caller for the duration of `process_records`. Batch work items borrow them by index range only; no view escapes the call. Worker scratch memory is local to each worker. In `batch_arena` mode, normalized temporary strings are allocated from a per-worker `std::pmr::monotonic_buffer_resource` and the arena is reset after each batch.

Canonical output records use ordinary owning `std::string` values because they survive the batch and are used for checksum and equivalence tests.

## Concurrency And Backpressure

The queue is bounded and protected by a mutex and condition variables. A full queue blocks the producer. An empty queue blocks workers. Output order is preserved by writing each processed record to its original index in the output vector.

## Main Files

- `include/financial/tp/pipeline.hpp`: public configuration, metrics, and processing API.
- `src/pipeline.cpp`: batch queue, worker-local state, normalization, enrichment, financial calculations.
- `benchmarks/throughput_benchmark.cpp`: core benchmark smoke runner with worker, queue, batch, and allocation-mode switches.
- `tests/throughput_tests.cpp`: equivalence, determinism, bounded queue, and arena-mode checks.

## Build And Run

```powershell
cmake --preset msvc
cmake --build --preset msvc-release
ctest --preset msvc-release
.\build\msvc\bin\Release\financial_tp.exe
.\build\msvc\bin\Release\financial_tp_benchmark.exe 10000 4 32 64 arena
```

Use `heap` instead of `arena` as the fifth benchmark argument to compare ordinary temporary allocation against batch-scoped PMR scratch allocation.
