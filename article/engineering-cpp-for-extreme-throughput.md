# Engineering C++ for Extreme Throughput

Draft status: evidence gathering.

This article will be written after the implementation stages have produced measurements. The conclusion is intentionally deferred until the evidence exists.

The working argument is that high-throughput C++ systems are not improved only by making allocation faster. The larger gains may come from representing data, ownership, and lifetime in a way that matches the workload.

The current repository establishes the financial-record domain model, deterministic workload, correctness checks, allocation instrumentation, and three separated implementations.

The sequential implementation is a conventional owning record pipeline. It receives one encoded record at a time, decodes into ordinary C++ objects, normalizes strings, enriches from immutable reference data, computes fixed-point financial indicators, validates the result, and appends a canonical record. Its value is not that it is slow; its value is that it is an understandable baseline with ordinary ownership boundaries.

The parallel implementation keeps that same record-at-a-time model and adds a bounded worker queue. The producer copies encoded records into queue work items, workers process one owning record each, and the output vector preserves input order by sequence position. This makes the second architecture deliberately recognizable as the migration many teams would attempt first: add cores around the existing data model, expose backpressure, and measure where the coordination and allocation costs appear.

A local smoke run on 2026-09-06 processed 2,000 deterministic records through the implementations and produced the same checksum, `8738754028557089284`. The parallel smoke run with four workers and queue capacity 64 filled the queue, reported 693 producer waits, and produced the same canonical output. Those numbers are evidence that the benchmark plumbing and equivalence checks are active, not final performance evidence.

The throughput-oriented implementation changes the queue contract. Instead of moving one owning encoded record per work item, the producer moves bounded index ranges, and each worker processes a batch using local scratch state. Normalized temporary text can be allocated from a per-worker `std::pmr::monotonic_buffer_resource` and reset at the batch boundary. The canonical output remains ordinary owning data because it must survive the batch for checksum and equivalence comparison. This is the first point in the project where the code treats lifetime as an architectural boundary rather than only as a property of individual objects.

The first throughput smoke run used four workers, queue capacity 16, and batch size 64. It produced the same checksum, reached queue occupancy 16, and reported 13 producer waits. The run is not article-grade evidence, but it does show the expected mechanical change: fewer queue transfers for the same business work and a visible batch reset boundary. A paired heap-mode run produced the same checksum and the same measured global allocation count, so the project should not yet claim that the observed timing difference is caused by PMR arenas.

The working domain is a Flink-like business document pipeline implemented as a custom in-process C++ component. The point is not to argue that C++ replaces a distributed stream processor. The useful boundary is narrower: when a transform is stable, local, deterministic, and dominated by per-record memory behaviour, the implementation can trade distributed runtime flexibility for direct control over representation, ownership, allocation, and batch lifetime.

Supporting architecture notes live in:

- `docs/domain.md`
- `docs/software-architecture.md`
- `docs/testing.md`
- `docs/implementation-plan.md`
