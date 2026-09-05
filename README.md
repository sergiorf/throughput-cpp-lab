# Engineering C++ for Extreme Throughput

This repository supports a C++ implementation and technical article about designing data, ownership, and memory lifetime around a high-throughput workload.

The project models an in-memory event-normalization and audit service. It generates deterministic encoded business events, processes them through equivalent pipeline stages, and records correctness and measurement evidence.

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

## Documentation

- [Domain model](docs/domain.md) explains the business document pipeline, the Flink-like boundary, reference data, and lifetime model.
- [Software architecture](docs/software-architecture.md) explains the semantic contract, stage structure, measured region, and benchmark integrity boundaries.
- [Testing and verification](docs/testing.md) explains how to build, test, run the scenario, and interpret failures.
- [Benchmark methodology](docs/benchmark-methodology.md) defines measured and unmeasured work.
- [Results](docs/results.md) is reserved for accepted evidence.

## Current Status

Implemented:

- deterministic workload generation;
- immutable reference data;
- idiomatic C++ baseline;
- semantic result contract for cross-stage equivalence;
- baseline, reserved, PMR, and custom arena stage entry points;
- stable checksums;
- allocation instrumentation;
- stage equivalence tests.

Later stages can cover copy reduction, reuse, and data layout without changing the stable business semantics.
