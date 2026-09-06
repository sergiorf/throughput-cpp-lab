# Architecture Notes

The current architecture is a two-stage financial-record comparison with a sequential oracle. The old event-normalization experiment and the earlier three-visible-stage progression have been removed from the active build and documentation.

## Boundary

The project studies an in-process C++ transform. It does not require Kafka, databases, network services, storage engines, or kernel-level I/O. That boundary keeps the initial article focused on application-level costs: decoding, normalization, immutable lookups, numerical work, allocation, copying, queueing, worker ownership, and batch lifetime.

## Visible Solution Progression

The article-facing progression is:

1. `01_parallel_batched`: parallel workers, bounded batch queue, ordinary owning records, ordinary heap allocation, and preserved output ordering.
2. `02_lifetime_optimized`: the same business contract and batch boundary, with borrowed input ranges, worker-local mutable state, optional batch-reset PMR scratch, and reduced queue payload movement.

`reference/sequential_oracle` remains outside this progression. It provides a simple correctness baseline for tests.

## Ownership

The shared workload owns the encoded input bytes. The oracle decodes each record into owning strings and emits owning canonical records.

The parallel batched baseline queues owning batches. Each queue item contains copied encoded records and an output start index. Workers decode those owned records into conventional intermediate objects and write canonical records into stable output slots.

The lifetime-optimized solution queues only input index ranges. The encoded records remain owned by the caller for the duration of `process_records`. Worker-local scratch owns temporary batch data, and canonical output is copied into ordinary owning records at the boundary where it must survive for checksumming and comparison.

## Current Limitations

The benchmark executables are smoke-grade. They report throughput, bytes, checksum, allocation counts, and solution-specific queue observations, but article-grade results still need repeated samples, environment metadata, latency distributions, and a workload matrix.
