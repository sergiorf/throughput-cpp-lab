# Engineering C++ for Extreme Throughput

Draft status: evidence gathering.

This article will be written after the implementation stages have produced measurements. The conclusion is intentionally deferred until the evidence exists.

The working argument is that high-throughput C++ systems are not improved only by making allocation faster. The larger gains may come from representing data, ownership, and lifetime in a way that matches the workload.

The current repository establishes the domain model, deterministic workload, correctness checks, allocation instrumentation, and a conventional baseline.

The working domain is a Flink-like business document pipeline implemented as a custom in-process C++ component. The point is not to argue that C++ replaces a distributed stream processor. The useful boundary is narrower: when a transform is stable, local, deterministic, and dominated by per-record memory behaviour, the implementation can trade distributed runtime flexibility for direct control over representation, ownership, allocation, and batch lifetime.

Supporting architecture notes live in:

- `docs/domain.md`
- `docs/software-architecture.md`
- `docs/testing.md`
