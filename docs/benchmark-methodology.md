# Benchmark Methodology

This page defines the measurement boundary. For the broader system structure, see [Software architecture](software-architecture.md). For build, test, and benchmark commands, see [Testing and verification](testing.md).

The measured region begins after deterministic encoded input batches and immutable reference data have been constructed.

Measured work:

- decode input bytes;
- validate required fields;
- normalize selected values;
- perform reference-data enrichment;
- construct canonical output records;
- compute a stable checksum;
- release batch-local state when the pipeline invocation ends.

Unmeasured work:

- deterministic workload generation;
- reference-data construction;
- executable startup;
- result printing;
- chart generation.

## Metrics

The financial benchmark executables report:

- records processed;
- elapsed time;
- records per second;
- allocation count;
- total allocated bytes;
- checksum.

`financial_parallel_batched_benchmark` additionally reports worker count, queue capacity, batch size, maximum queue occupancy, producer waits, consumer waits, and accumulated wait time. `financial_lifetime_optimized_benchmark` reports those fields plus allocation mode so PMR scratch allocation can be varied without changing business semantics.

Future benchmark work will add repeated samples, median, p95, p99, peak resident memory where reliable, retained memory after reset, worker utilization, and workload matrix reporting.

## Validity Notes

Every stage must process identical encoded inputs and produce equivalent checksums. Allocation counters are process-local and intended for relative comparison inside one executable run. If instrumentation materially changes timing, instrumented and non-instrumented timings must be reported separately.
