# Sequential Oracle

This implementation processes one financial record at a time. It decodes a binary payload into owning C++ objects, normalizes string fields, enriches from immutable reference data, computes financial indicators, validates the result, and appends a canonical output record.

The oracle is deliberately ordinary: `std::string`, `std::vector`, and value types own their data at each stage. There is no concurrency and no custom memory management. Its job is to define and debug business semantics for the visible implementations, not to be an article-facing throughput architecture.

Build and run from the repository root:

```powershell
cmake --preset default
cmake --build --preset default
ctest --preset default
.\build\default\bin\financial_oracle.exe
.\build\default\bin\financial_oracle_benchmark.exe 10000
```
