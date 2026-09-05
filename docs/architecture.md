# Architecture Notes

The project models a batch-oriented event-normalization service. External I/O is intentionally absent so measurements focus on application-level processing and memory behaviour.

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

## Stage 1 Baseline

The baseline is intentionally ordinary C++ rather than a straw man. It uses owning `std::string` and `std::vector` members in decoded, normalized, and audit records. This creates many independent allocations, but the design is clear, safe, and maintainable.

The baseline establishes behavioural semantics for all later stages.

