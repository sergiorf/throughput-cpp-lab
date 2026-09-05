# Benchmark Methodology

The measured region begins after deterministic encoded input batches and immutable reference data have been constructed.

Measured work:

- decode input bytes;
- validate required fields;
- normalize selected values;
- perform reference-data enrichment;
- construct normalized and audit records;
- compute a stable checksum;
- release batch-local state when the pipeline invocation ends.

Unmeasured work:

- deterministic workload generation;
- reference-data construction;
- executable startup;
- result printing;
- chart generation.

## Metrics

The initial scenario runner reports:

- records processed;
- elapsed time;
- records per second;
- allocation count;
- total allocated bytes;
- normalized output count;
- audit output count;
- checksum.

Future benchmark work will add repeated samples, median, p95, p99, peak resident memory where reliable, retained memory after reset, and workload matrix reporting.

## Validity Notes

Every stage must process identical encoded inputs and produce equivalent checksums. Allocation counters are process-local and intended for relative comparison inside one executable run. If instrumentation materially changes timing, instrumented and non-instrumented timings must be reported separately.

