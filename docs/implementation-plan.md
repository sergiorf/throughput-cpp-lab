# Implementation Plan

This refactor turns the existing throughput lab into a financial-record service
comparison for the article "Engineering C++ for Extreme Throughput".

## Final Repository Structure

The repository keeps a small `common` area for comparison contracts and places
architectural decisions inside three separated solutions:

- `common/include/financial/common`: wire record types, deterministic fixtures,
  immutable reference data, checksums, benchmark metrics, and allocation
  counters.
- `solutions/01_sequential`: conventional record-at-a-time owning pipeline.
- `solutions/02_parallel`: conventional owning pipeline behind a bounded
  per-record worker queue.
- `solutions/03_throughput`: batch-oriented pipeline with worker-local mutable
  state and optional reusable memory.
- `docs`: methodology, wire format, reproducibility, limitations, and this plan.
- `article`: companion article and editable diagram sources.

## Shared Contracts

The solutions share only contracts required for fair comparison:

- a length-prefixed binary message containing a deterministic financial record;
- deterministic workload generation with fixed seeds and named profiles;
- immutable country, currency, and industry reference fixtures;
- canonical output records and stable checksums for equivalence tests;
- benchmark metric schema and allocation counters.

No shared library owns the processing pipeline, worker architecture, record
representation, queue abstraction, or memory strategy.

## Solution Architectures

`01_sequential` decodes one framed record into owning C++ objects, normalizes
strings, enriches from immutable reference data, computes ratios, validates the
record, encodes a canonical result, and moves to the next record. It is the
baseline for correctness, clarity, and ordinary heap allocation.

`02_parallel` keeps conventional owning records but adds producer/consumer
coordination around a bounded queue. One record is one work item. The
implementation makes output ordering explicit by preserving input sequence in
the final result, which exposes the coordination cost of retaining a simple
record model.

`03_throughput` changes the work unit to a batch. Queue boundaries transfer
batches, workers own reusable mutable state, temporary memory can be reset at
batch boundaries, and numerical fields are processed from compact vectors where
that keeps the design explainable. The normal path remains readable; benchmark
switches vary batching and memory reuse independently.

## Benchmark Methodology

Two benchmark modes are planned:

- core processing: pre-generated messages and an in-memory sink;
- socket boundary: local framed streams that exercise fragmentation, partial
  reads, partial writes, batching, and shutdown.

Every run reports all samples after warm-up, input/output byte counts, stable
checksum, latency distribution where available, allocation counters, worker
count, queue capacity, batch size, and environment metadata. Results gathered
on this development machine are provisional until repeated under a documented
release configuration.

## Equivalence Strategy

The sequential solution defines the first accepted canonical output. Parallel
and throughput-oriented solutions must produce the same canonical records and
stable checksum for representative fixtures, edge-case records, and generated
workloads. Floating-point values are rounded into fixed-point basis points in
the canonical record before checksumming.

## Article Outline

1. Introduce the financial-record service and bounded business problem.
2. Explain why ordinary business computation is used deliberately.
3. Present the sequential architecture and its ownership model.
4. Establish baseline timing, allocation, and checksum evidence.
5. Present conventional parallelization around record work items.
6. Measure what cores improve and what queueing exposes.
7. Explain why data movement and lifetime require a redesign.
8. Present batch-oriented processing, worker ownership, and reusable memory.
9. Measure the combined throughput-oriented result.
10. Attribute changes through controlled switches.
11. Discuss throughput, latency, memory, and complexity together.
12. Stop where measured benefit no longer justifies architectural cost.

## Planned Diagrams

- business-processing pipeline;
- sequential record lifecycle;
- parallel ownership transfer and ordered output;
- bounded queue and backpressure;
- record-at-a-time versus batch-oriented flow;
- per-worker ownership;
- arena reset boundaries;
- object layout versus compact numerical batch layout;
- immutable reference-data access;
- final throughput-oriented architecture;
- performance evolution;
- throughput versus tail latency;
- memory usage across configurations.
