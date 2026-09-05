# Architecture Notes

The project models a batch-oriented event-normalization service. External I/O is intentionally absent so measurements focus on application-level processing and memory behaviour.

See also:

- [Domain model](domain.md) for the business document framing and Flink-like boundary.
- [Software architecture](software-architecture.md) for source layout, stage contracts, and diagrams.
- [Testing and verification](testing.md) for build, test, scenario, and equivalence checks.

## Semantic Contract

The stable layer defines the encoded input batch, immutable reference data, validation and normalization semantics, and observable result contract. Optimized stages may use different internal storage, but they must produce the same normalized record count, audit record count, and stable checksum for equivalent workload parameters.

The current cross-stage contract is `throughput::SemanticResult`. It intentionally records observable behaviour rather than exposing a required concrete record layout. This keeps later stages free to use owning strings, borrowed views, PMR containers, arena-owned objects, or denser layouts without changing the business result.

## Pipeline

Each stage performs the same business work:

1. decode deterministic encoded input events;
2. validate required fields;
3. normalize identifiers and selected text values;
4. enrich from immutable reference data;
5. construct normalized output records;
6. construct audit records;
7. compute a stable batch checksum;
8. release or reset batch-scoped state.

## Lifetime Categories

Application lifetime contains immutable reference data: jurisdictions, source systems, event types, aliases, and category mappings.

Input-buffer lifetime contains encoded batch bytes. Later stages may borrow from this buffer when safe.

Temporary parsing and validation lifetime contains decoded fragments, scratch strings, and validation state.

Batch lifetime contains normalized records and detailed audit records that do not escape the batch.

Output lifetime contains selected records that survive commit.

Irregular lifetime contains exceptional objects whose ownership cannot be predicted cleanly.

## Stage Structure

The baseline is intentionally ordinary C++ rather than a straw man. It uses owning `std::string` and `std::vector` members in decoded, normalized, and audit records. This creates many independent allocations, but the design is clear, safe, and maintainable.

The baseline establishes behavioural semantics for all later stages.

The initial source structure contains four stages:

1. `baseline`: ordinary owning C++ records with straightforward parsing and normalization.
2. `reserved`: the same owning representation, but with predictable nested container capacity reserved from encoded delimiter counts.
3. `pmr`: PMR strings and vectors backed by a batch-scoped `std::pmr::monotonic_buffer_resource`.
4. `arena`: the same PMR-aware pipeline backed by a project-owned monotonic memory resource with a fixed local buffer and upstream fallback.

The reserved stage isolates capacity planning from allocator strategy. The PMR stage isolates standard allocator/lifetime control from custom arena design. The custom arena stage exists as an implementation experiment, not as an assumed winner.
