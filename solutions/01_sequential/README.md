# Solution 1: Conventional Sequential Pipeline

This implementation processes one financial record at a time. The pipeline
decodes a binary payload into owning C++ objects, normalizes string fields,
enriches from immutable reference data, computes financial indicators, validates
the result, and appends a canonical output record.

The ownership model is deliberately ordinary: `std::string`, `std::vector`, and
value types own their data at each stage. There is no concurrency and no custom
memory management. Intermediate records are explicit where they make the
business flow easier to inspect.

Expected strengths are clarity, deterministic correctness, and a trustworthy
baseline. Expected limits are per-record allocation, repeated representation
construction, and no ability to use multiple cores.

Build and run from the repository root:

```powershell
cmake --preset default
cmake --build --preset default
ctest --preset default
.\build\default\bin\financial_seq.exe
.\build\default\bin\financial_seq_benchmark.exe 10000
```
