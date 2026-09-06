# Solution 2: Conventional Parallel Pipeline

This implementation keeps the record-at-a-time object model from the sequential
baseline and adds worker threads around it. The producer copies each encoded
record into a bounded work queue. Workers decode, normalize, enrich, calculate,
validate, and encode-size one owning record at a time.

Output order is preserved by writing each result into the slot associated with
its input sequence position. That makes equivalence testing straightforward and
keeps the external contract simple, but it also means the implementation pays
for per-record coordination and a retained output array.

The memory strategy is conventional heap allocation. The queue stores owning
work items, each worker constructs normal intermediate objects, and no arena or
view-backed representation crosses a lifetime boundary. Immutable reference
data is shared read-only among workers.

Expected strengths are simple multicore scaling and a familiar migration path
from the sequential service. Expected limits are queue contention, producer
backpressure, repeated allocation, per-record synchronization, and fragmented
memory access.

Build and run from the repository root:

```powershell
cmake --preset default
cmake --build --preset default
ctest --preset default
.\build\default\bin\financial_par.exe
.\build\default\bin\financial_par_benchmark.exe 10000 4 256
```
