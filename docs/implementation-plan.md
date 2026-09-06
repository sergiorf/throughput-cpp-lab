# Implementation Plan

This refactor narrows the article-facing comparison to two serious implementations. A sequential implementation remains as a quiet oracle for correctness and debugging, but it is not a principal architecture in the article.

## Repository Structure

- `common/include/financial/common`: wire record types, deterministic fixtures, immutable reference data, checksums, benchmark metrics, and allocation counters.
- `reference/sequential_oracle`: simple single-threaded correctness oracle.
- `solutions/01_parallel_batched`: competent production-style baseline using worker threads, a bounded batch queue, ordinary owning records, and general heap allocation.
- `solutions/02_lifetime_optimized`: deeper throughput redesign using borrowed input ranges, worker-local mutable state, batch-lifetime scratch memory, and controlled allocation modes.
- `docs`: methodology, architecture, domain, testing, results, and this plan.
- `article`: companion article and editable diagram sources.

## Shared Contracts

All implementations share only the contracts required for fair comparison:

- deterministic encoded financial records;
- immutable country, currency, and industry reference data;
- canonical output records;
- stable checksums;
- benchmark metric fields;
- allocation counters.

The visible solutions do not share pipeline logic, queue abstractions, worker architecture, record representation, or memory strategy.

## Architecture Direction

`reference/sequential_oracle` decodes records one at a time into ordinary owning objects and emits canonical records. Its purpose is to make semantic regressions easy to diagnose.

`01_parallel_batched` starts from the architecture a competent team should already have before pursuing extreme throughput: workers, bounded backpressure, batch work items, ordered output, and conventional heap-owned records. It asks what performance looks like when parallelism and batching are already table stakes.

`02_lifetime_optimized` keeps the external contract and batch processing boundary, then changes deeper costs: queue items carry borrowed input ranges, each worker owns reusable scratch state, temporary normalized text can live in batch-reset PMR storage, and final output is copied only at the lifetime boundary where it must survive.

## Benchmark Methodology

The measured region starts after deterministic workload generation and reference-data construction. Every benchmark reports records, input bytes, output bytes, elapsed time, records per second, checksum, allocation count, and allocated bytes. Worker implementations also report worker count, queue capacity, batch size, queue occupancy, producer waits, consumer waits, and accumulated wait time.

Article-grade results still require warm-up, repeated samples, environment metadata, release builds, and a workload matrix. Smoke runs are allowed only to prove plumbing and equivalence.

## Equivalence Strategy

The oracle defines the canonical business result. Both visible solutions must match its canonical records and checksum across representative workloads, edge cases, worker counts, batch sizes, and allocation modes. Floating-point calculations are rounded into fixed-point basis points before checksumming.

## Article Outline

1. Introduce the financial-record service.
2. Explain why the business computation is ordinary and deterministic.
3. Present the parallel batched baseline as the expected starting point.
4. Establish its timing, allocation, queue, and memory profile.
5. Explain why batching and more workers do not remove all data-movement costs.
6. Present the lifetime-optimized design.
7. Use controlled switches to attribute changes to batching, allocation, copying, and representation.
8. Discuss throughput, latency, memory, and complexity together.
9. Stop where measured benefit no longer justifies architectural complexity.

## Planned Diagrams

- business-processing pipeline;
- oracle role in equivalence testing;
- parallel batched ownership transfer;
- bounded batch queue and backpressure;
- owning batch flow versus borrowed-range flow;
- per-worker ownership;
- arena reset boundaries;
- conventional object layout versus optimized batch-local representation;
- immutable reference-data access;
- final lifetime-optimized architecture;
- performance evolution;
- throughput versus tail latency;
- memory usage across configurations.
