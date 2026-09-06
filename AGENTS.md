# Engineering C++ for Extreme Throughput

## Project Purpose

This repository supports a technical article provisionally titled "Engineering C++ for Extreme Throughput". The code and article investigate whether extreme throughput comes primarily from faster allocation, or from designing data representation, ownership, and memory lifetime around the workload.

The project must preserve negative and surprising results. Do not force arenas, PMR, or any other technique to win.

## Core Constraints

- Main language target: portable C++23.
- Build system: CMake.
- Test runner: CTest.
- External infrastructure is out of scope. Do not require Kafka, networking, databases, storage engines, or kernel-level I/O.
- Keep external dependencies minimal. Add a dependency only when it materially improves measurement or clarity.
- Keep benchmark code separate from core domain logic.
- Every implementation stage must perform equivalent business work and produce deterministic, verifiable results.
- Do not change multiple major variables in one benchmark stage.
- Avoid lock-free queues, SIMD, networking, and external I/O in the initial article project.

## Current Build Commands

```powershell
cmake --preset default
cmake --build --preset default
ctest --preset default
```

For MSVC multi-config generators:

```powershell
cmake --preset msvc
cmake --build --preset msvc-release
ctest --preset msvc-release
```

## Benchmark Commands

```powershell
.\build\default\bin\financial_oracle_benchmark.exe 10000
.\build\default\bin\financial_parallel_batched_benchmark.exe 10000 4 32 64
.\build\default\bin\financial_lifetime_optimized_benchmark.exe 10000 4 32 64 arena
```

These benchmark executables are not a substitute for full article-grade measurement. They exist first to prove deterministic execution, output checksums, allocation instrumentation, and measured-region boundaries.

## Benchmark Integrity Rules

- Generate deterministic input data outside the measured region unless testing generation itself.
- Use the same generated input bytes, reference data, and correctness checks for every stage.
- Report all repeated runs. Do not select only favourable samples.
- Record throughput, batch latency, allocation count, allocated bytes, checksum, and environment metadata.
- Keep warm-up runs separate from reported samples.
- Protect against dead-code elimination with stable checksums and observable result counts.
- Treat allocation instrumentation overhead as part of the measurement design. If it distorts timing, report instrumented and non-instrumented runs separately.
- Track retained memory after reuse or reset where applicable.
- Do not claim "zero allocation" unless instrumentation proves it for the precisely defined measured region.

## Article Principles

- The article is an architectural investigation, not a code walkthrough.
- The prose should be concise, thoughtful, and technically precise.
- Prefer substantial paragraphs with connected reasoning over mechanical bullet lists.
- Avoid promotional phrasing and generic "modern C++" claims.
- Do not write the final conclusion until all benchmark evidence exists.
- Preserve the distinction between faster allocation, fewer allocations, fewer copies, better locality, cheaper reclamation, and improved batching.
- Diagrams must answer specific questions and be backed by implementation evidence.

## Implementation Stage Contract

Each stage must:

- accept the same encoded batch representation;
- use the same immutable reference data;
- apply the same validation and normalization rules;
- emit behaviourally equivalent canonical results;
- produce the same stable checksum for equivalent workload parameters;
- expose allocation and timing observations without changing business semantics.

## Lifetime Model

The code and docs distinguish these lifetimes:

- immutable application-lifetime reference data;
- input-buffer lifetime;
- temporary parsing and validation data;
- batch-lifetime normalized data;
- output that must survive the batch;
- exceptional objects with irregular or independent lifetimes.

APIs that borrow memory must document the owner and the lifetime boundary. No stage may rely on dangling `std::string_view`, `std::span`, pointer, or index assumptions.
