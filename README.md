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

## Current Status

Implemented:

- deterministic workload generation;
- immutable reference data;
- idiomatic C++ baseline;
- stable checksums;
- allocation instrumentation;
- baseline tests.

Later stages will cover copy reduction, reuse, data layout, `std::pmr`, and a custom arena only if the evidence justifies it.
