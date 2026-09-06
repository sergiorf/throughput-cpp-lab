# Engineering C++ for Extreme Throughput

Draft status: evidence gathering.

This article will be completed after the implementation stages have produced repeated measurements. The conclusion is intentionally deferred until the evidence exists.

The working argument is that high-throughput C++ systems are not improved only by making allocation faster. The larger gains may come from representing data, ownership, and lifetime in a way that matches the workload.

The current repository establishes the financial-record domain model, deterministic workload, correctness checks, allocation instrumentation, a sequential oracle, and two article-facing implementations.

The sequential implementation is now a reference oracle rather than a headline stage. It receives one encoded record at a time, decodes into ordinary C++ objects, normalizes strings, enriches from immutable reference data, computes fixed-point financial indicators, validates the result, and appends a canonical record. Its value is diagnostic: when a worker implementation changes the checksum, the oracle gives a small and readable place to inspect the intended business behaviour.

The first visible implementation is already parallel and batched. It represents the architecture a competent team should have before claiming to work on extreme throughput: worker threads, bounded backpressure, batch work items, preserved output ordering, ordinary owning records, and general heap allocation. This baseline does not try to make batching look novel. It asks what remains expensive after queue synchronization has already been amortized across batches.

The second visible implementation keeps the batch boundary but changes data movement and lifetime. Queue entries contain input index ranges rather than owning vectors of encoded records. Workers own mutable scratch state. Normalized temporary text can be allocated from a per-worker `std::pmr::monotonic_buffer_resource` and reset at the batch boundary. Canonical output remains ordinary owning data because it must survive the batch for checksum and equivalence comparison.

The central distinction is therefore not sequential versus parallel, or record-at-a-time versus batch. The project starts after those obvious choices. The article investigates the harder question: once the service is parallel, batched, bounded, and deterministic, which additional costs still matter enough to justify more complex ownership and lifetime design?

A local smoke run on 2026-09-06 processed 2,000 deterministic records through the oracle, the parallel batched baseline, and the lifetime-optimized implementation. All runs produced the same checksum, `8738754028557089284`. The parallel batched baseline reported 12 producer waits with queue capacity 16 and batch size 64. The lifetime-optimized heap and arena modes also matched the checksum and reported the same global allocation counts; in this single run, heap mode was faster than arena mode. Those numbers are useful as plumbing evidence, but they are not article-grade measurements.

The project should not claim that PMR, arenas, borrowed ranges, or reduced copying are universally superior. They are mechanisms with costs. The article should preserve cases where the simpler batched baseline is close enough, and it should separate fewer allocations from faster allocation, reduced queue payload movement from better cache locality, and lower average latency from tail-latency trade-offs.

Supporting architecture notes live in:

- `docs/domain.md`
- `docs/software-architecture.md`
- `docs/testing.md`
- `docs/implementation-plan.md`
