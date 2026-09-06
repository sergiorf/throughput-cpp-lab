# Software Architecture

The repository is organized around a correctness oracle and two independently browsable financial-record implementations. Shared code exists only for contracts that make comparison fair: encoded input, deterministic workload generation, immutable reference data, stable checksums, benchmark metric fields, and allocation instrumentation.

## Source Layout

```mermaid
flowchart TB
    common[common/include/financial/common]
    oracle[reference/sequential_oracle]
    batched[solutions/01_parallel_batched]
    optimized[solutions/02_lifetime_optimized]
    docs[docs]
    article[article]

    common --> contracts[wire format<br/>reference data<br/>generator<br/>checksum<br/>instrumentation]
    oracle --> oracle_arch[sequential correctness oracle]
    batched --> batched_arch[parallel batched baseline<br/>owning records]
    optimized --> optimized_arch[lifetime optimized pipeline<br/>borrowed ranges and scratch]
    docs --> method[architecture, domain, testing, methodology, results]
    article --> draft[article draft and diagram sources]
```

The processing pipeline, queue abstraction, worker architecture, internal record representation, and memory strategy are not shared by the two visible solutions. Those are the design choices the project compares.

## Shared Contract

Each implementation accepts the same `std::vector<financial::common::EncodedRecord>`, reads the same immutable `financial::common::ReferenceData`, and returns canonical records plus a stable checksum. Equivalence is defined by `financial::common::CanonicalRecord`, not by internal objects.

```mermaid
flowchart LR
    input[Encoded financial records]
    ref[Immutable reference data]
    impl[Implementation]
    output[Canonical records]
    checksum[Stable checksum]

    input --> impl
    ref --> impl
    impl --> output
    output --> checksum
```

## Implementations

`reference/sequential_oracle` is the test oracle. It processes one record at a time with ordinary owning C++ objects and no worker coordination. It exists to make semantic regressions easier to debug, not as an article-facing throughput stage.

`solutions/01_parallel_batched` is the competent baseline. It uses worker threads, a bounded queue, explicit backpressure, batch work items, preserved output ordering, conventional owning record objects, and general heap allocation. Queue entries own a vector of encoded records, so this stage batches coordination without introducing borrowed input lifetime rules.

`solutions/02_lifetime_optimized` keeps the same external contract and batch boundary, then changes data movement and lifetime. Queue entries carry input index ranges, workers own reusable scratch state, temporary normalized strings can be allocated from a batch-reset `std::pmr::monotonic_buffer_resource`, and canonical output remains owning because it must survive the batch.

```mermaid
flowchart LR
    oracle[sequential oracle]
    batched[01_parallel_batched<br/>owning batches]
    optimized[02_lifetime_optimized<br/>borrowed ranges and scratch]

    oracle --> tests[Equivalence tests]
    batched --> tests
    optimized --> tests
    batched --> optimized
```

## Measured Region

Benchmark executables construct deterministic input and immutable reference data before the timed region. The timed region includes decode, normalization, enrichment, financial calculations, validation, canonical output construction, checksum protection, allocation effects, and any queue or worker coordination used by that implementation.

```mermaid
flowchart LR
    setup[Unmeasured setup<br/>generate records and reference data]
    start[Start timer and allocation scope]
    work[Measured implementation work]
    stop[Stop timer and allocation scope]
    report[Print metrics]

    setup --> start --> work --> stop --> report
```

## Benchmark Integrity

The comparison is valid only while input bytes, reference data, normalization rules, validation rules, financial formulas, canonical output, and checksum semantics remain equivalent. A faster implementation that changes the checksum is a failed implementation, not a performance result.

The article should not argue that batching is surprising. The baseline is already batched. The central question is what remains after parallelism and batching are present: copying, temporary ownership, allocator behaviour, cache locality, and lifetime boundaries.
