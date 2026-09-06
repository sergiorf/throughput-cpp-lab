# Results

No final benchmark results have been accepted yet.

The repository currently contains smoke benchmarks for the sequential oracle and the two article-facing financial solutions. Published results should not be added until repeated runs and environment metadata are collected.

## Local Smoke Evidence

These runs were collected on the development environment on 2026-09-06 with the MSVC release preset. They are useful for checking correctness and benchmark plumbing, but they are not final article evidence.

```text
financial_oracle_benchmark 2000
records=2000
input_bytes=428760
output_bytes=379997
elapsed_seconds=0.020545
records_per_second=97346.34
allocations=16428
allocated_bytes=1422887
checksum=8738754028557089284

financial_parallel_batched_benchmark 2000 4 16 64
records=2000
input_bytes=428760
output_bytes=379997
elapsed_seconds=0.010467
records_per_second=191069.42
allocations=18505
allocated_bytes=1901487
checksum=8738754028557089284
worker_count=4
queue_capacity=16
batch_size=64
queue_max_occupancy=16
producer_waits=12
consumer_waits=0

financial_lifetime_optimized_benchmark 2000 4 16 64 arena
records=2000
input_bytes=428760
output_bytes=379997
elapsed_seconds=0.008263
records_per_second=242034.05
allocations=12370
allocated_bytes=1283863
checksum=8738754028557089284
worker_count=4
queue_capacity=16
batch_size=64
allocation_mode=arena
queue_max_occupancy=16
producer_waits=14
consumer_waits=0

financial_lifetime_optimized_benchmark 2000 4 16 64 heap
records=2000
input_bytes=428760
output_bytes=379997
elapsed_seconds=0.007860
records_per_second=254462.64
allocations=12370
allocated_bytes=1283863
checksum=8738754028557089284
worker_count=4
queue_capacity=16
batch_size=64
allocation_mode=heap
queue_max_occupancy=16
producer_waits=16
consumer_waits=0
```

The matching checksum is the important observation at this stage. The parallel batched baseline copies encoded records into owning batch queue entries. The lifetime-optimized implementation transfers index ranges instead. In this single smoke run, the lifetime-optimized heap mode was faster than arena mode while reporting the same global allocation counts. That result is provisional, but it is exactly the kind of evidence the project should preserve: the current instrumentation is not enough by itself to claim that PMR arena allocation caused an improvement.
