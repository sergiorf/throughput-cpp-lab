# Results

No final benchmark results have been accepted yet.

The repository currently contains a scenario runner plus smoke benchmarks for the three separated financial solutions. Published results should not be added until repeated runs and environment metadata are collected.

## Local Smoke Evidence

These runs were collected on the development environment on 2026-09-06 with the MSVC release preset. They are useful for checking correctness and benchmark plumbing, but they are not final article evidence.

```text
financial_seq_benchmark 2000
records=2000
input_bytes=428760
output_bytes=379997
elapsed_seconds=0.030680
records_per_second=65187.99
allocations=16428
allocated_bytes=1422887
checksum=8738754028557089284

financial_par_benchmark 2000 4 64
records=2000
input_bytes=428760
output_bytes=379997
elapsed_seconds=0.014641
records_per_second=136600.83
allocations=18571
allocated_bytes=1858095
checksum=8738754028557089284
queue_max_occupancy=64
producer_waits=693
consumer_waits=0

financial_tp_benchmark 2000 4 16 64 arena
records=2000
input_bytes=428760
output_bytes=379997
elapsed_seconds=0.011831
records_per_second=169053.13
allocations=12370
allocated_bytes=1283863
checksum=8738754028557089284
queue_max_occupancy=16
producer_waits=13
consumer_waits=0

financial_tp_benchmark 2000 4 16 64 heap
records=2000
input_bytes=428760
output_bytes=379997
elapsed_seconds=0.016194
records_per_second=123501.01
allocations=12370
allocated_bytes=1283863
checksum=8738754028557089284
queue_max_occupancy=16
producer_waits=9
consumer_waits=0
```

The matching checksum is the important observation at this stage. The throughput-oriented smoke run transfers index ranges in batches rather than copying encoded records into per-record queue entries. The heap and arena modes reported the same global allocation counts for this small workload, which means the current instrumentation is not enough by itself to attribute the timing difference to PMR allocation. The timing differences are provisional and should not be interpreted until repeated runs, environment metadata, and a workload matrix are available.
