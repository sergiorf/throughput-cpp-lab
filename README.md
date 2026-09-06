# Engineering C++ for Extreme Throughput

This repository supports a C++ implementation and technical article about designing data, ownership, and memory lifetime around a high-throughput workload.

The project models a financial-record service. Deterministic encoded company financial records are decoded, normalized, enriched from immutable reference data, classified, encoded into canonical results, and measured under multiple architectures.

The current refactor keeps the three comparison solutions separated under `solutions/`:

| Concern | Sequential | Parallel | Throughput-oriented |
| --- | --- | --- | --- |
| Work unit | One record | One record | Batch |
| Allocation | General heap | General heap | Worker-local PMR scratch, canonical owning output |
| Coordination | None | Bounded per-record queue | Bounded batch queue |
| Representation | Owning objects | Owning objects | Borrowed input ranges, batch-local normalized text |
| Reference data | Immutable | Shared immutable | Shared immutable |
| I/O | Record-oriented payloads | Record-oriented payloads | Batched in-memory processing boundary |

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

## Run The Scenario

Default preset:

```powershell
.\build\default\bin\throughput_scenario.exe
```

Visual Studio preset:

```powershell
.\build\msvc\bin\Release\throughput_scenario.exe
```

## Run The Financial Solutions

Default preset:

```powershell
.\build\default\bin\financial_seq.exe
.\build\default\bin\financial_seq_benchmark.exe 10000
.\build\default\bin\financial_par.exe
.\build\default\bin\financial_par_benchmark.exe 10000 4 256
.\build\default\bin\financial_tp.exe
.\build\default\bin\financial_tp_benchmark.exe 10000 4 32 64 arena
```

Visual Studio preset:

```powershell
.\build\msvc\bin\Release\financial_seq.exe
.\build\msvc\bin\Release\financial_seq_benchmark.exe 10000
.\build\msvc\bin\Release\financial_par.exe
.\build\msvc\bin\Release\financial_par_benchmark.exe 10000 4 256
.\build\msvc\bin\Release\financial_tp.exe
.\build\msvc\bin\Release\financial_tp_benchmark.exe 10000 4 32 64 arena
```

## Documentation

- [Domain model](docs/domain.md) explains the business document pipeline, the Flink-like boundary, reference data, and lifetime model.
- [Software architecture](docs/software-architecture.md) explains the semantic contract, stage structure, measured region, and benchmark integrity boundaries.
- [Testing and verification](docs/testing.md) explains how to build, test, run the scenario, and interpret failures.
- [Benchmark methodology](docs/benchmark-methodology.md) defines measured and unmeasured work.
- [Results](docs/results.md) is reserved for accepted evidence.
- [Implementation plan](docs/implementation-plan.md) describes the target separated-solution structure.
- [Article draft](article/engineering-cpp-for-extreme-throughput.md) tracks verified observations while evidence is gathered.

## Current Status

Implemented:

- shared financial wire format, deterministic workload generation, reference fixtures, checksums, and allocation instrumentation;
- `solutions/01_sequential`, a conventional owning record-at-a-time pipeline;
- `solutions/02_parallel`, a conventional worker-thread pipeline with bounded queue backpressure and order-preserving output;
- `solutions/03_throughput`, a batch-oriented worker pipeline with borrowed input ranges and worker-local PMR scratch state;
- equivalence tests showing the parallel and throughput-oriented solutions produce the same canonical records and checksum as the sequential solution.

The socket boundary remains to be implemented. Final performance claims are intentionally deferred until repeated release-mode measurements are collected.
