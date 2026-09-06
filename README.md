# Engineering C++ for Extreme Throughput

This repository supports a C++ implementation and technical article about designing data, ownership, and memory lifetime around a high-throughput workload.

The project models a financial-record service. Deterministic encoded company financial records are decoded, normalized, enriched from immutable reference data, classified, encoded into canonical results, and measured under two article-facing architectures. A sequential implementation remains in `reference/sequential_oracle` only as a correctness oracle.

| Concern | Parallel batched baseline | Lifetime optimized |
| --- | --- | --- |
| Work unit | Batch | Batch |
| Allocation | General heap | Worker-local PMR scratch, canonical owning output |
| Coordination | Bounded batch queue | Bounded batch queue with smaller queue items |
| Representation | Conventional owning objects | Borrowed input ranges, batch-local normalized text |
| Reference data | Shared immutable | Shared immutable |
| I/O boundary | Batched in-memory processing | Batched in-memory processing |

## Build Instructions

Default preset:

```powershell
cmake --preset default
cmake --build --preset default
ctest --preset default
```

Visual Studio preset:

```powershell
cmake --preset msvc
cmake --build --preset msvc-release
ctest --preset msvc-release
```

## Run The Implementations

Default preset:

```powershell
.\build\default\bin\financial_oracle.exe
.\build\default\bin\financial_oracle_benchmark.exe 10000
.\build\default\bin\financial_parallel_batched.exe
.\build\default\bin\financial_parallel_batched_benchmark.exe 10000 4 32 64
.\build\default\bin\financial_lifetime_optimized.exe
.\build\default\bin\financial_lifetime_optimized_benchmark.exe 10000 4 32 64 arena
```

Visual Studio preset:

```powershell
.\build\msvc\bin\Release\financial_oracle.exe
.\build\msvc\bin\Release\financial_oracle_benchmark.exe 10000
.\build\msvc\bin\Release\financial_parallel_batched.exe
.\build\msvc\bin\Release\financial_parallel_batched_benchmark.exe 10000 4 32 64
.\build\msvc\bin\Release\financial_lifetime_optimized.exe
.\build\msvc\bin\Release\financial_lifetime_optimized_benchmark.exe 10000 4 32 64 arena
```

## Documentation

- [Domain model](docs/domain.md) explains the financial-record pipeline, reference data, validation rules, and lifetime model.
- [Software architecture](docs/software-architecture.md) explains the oracle, the two article-facing implementations, shared contracts, and benchmark boundaries.
- [Testing and verification](docs/testing.md) explains how to build, test, run benchmarks, and interpret failures.
- [Benchmark methodology](docs/benchmark-methodology.md) defines measured and unmeasured work.
- [Results](docs/results.md) is reserved for accepted evidence.
- [Implementation plan](docs/implementation-plan.md) describes the current two-stage direction.
- [Article draft](article/engineering-cpp-for-extreme-throughput.md) tracks verified observations while evidence is gathered.

## Current Status

Implemented:

- shared financial wire format, deterministic workload generation, reference fixtures, checksums, and allocation instrumentation;
- `reference/sequential_oracle`, a conventional single-threaded correctness oracle;
- `solutions/01_parallel_batched`, a competent parallel batched baseline with ordinary owning records and heap allocation;
- `solutions/02_lifetime_optimized`, a batch-oriented redesign with borrowed input ranges and worker-local PMR scratch state;
- equivalence tests showing both article-facing solutions produce the same canonical records and checksum as the oracle.

Final performance claims are intentionally deferred until repeated release-mode measurements are collected.
