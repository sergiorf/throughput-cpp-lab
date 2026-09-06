# Results

No final benchmark results have been accepted yet.

The repository currently contains a scenario runner plus smoke benchmarks for the separated sequential and parallel financial solutions. Published results should not be added until repeated runs and environment metadata are collected.

## Local Smoke Evidence

These runs were collected on the development environment on 2026-09-06 with the MSVC release preset. They are useful for checking correctness and benchmark plumbing, but they are not final article evidence.

```text
financial_seq_benchmark 2000
records=2000
input_bytes=428760
output_bytes=379997
elapsed_seconds=0.029183
records_per_second=68532.35
allocations=16428
allocated_bytes=1422887
checksum=8738754028557089284

financial_par_benchmark 2000 4 64
records=2000
input_bytes=428760
output_bytes=379997
elapsed_seconds=0.014149
records_per_second=141355.74
allocations=18571
allocated_bytes=1858095
checksum=8738754028557089284
queue_max_occupancy=64
producer_waits=631
consumer_waits=7
```

The matching checksum is the important observation at this stage. The timing difference is provisional and should not be interpreted until repeated runs, environment metadata, and a workload matrix are available.
