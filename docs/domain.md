# Domain Model

The project models a high-throughput business document pipeline. The workload is intentionally similar to a narrow Flink job: encoded business documents arrive in batches, each document is decoded, validated, normalized, enriched from immutable reference data, and emitted as normalized output plus an audit trail.

The project does not model distributed execution, external connectors, checkpointing, networking, or storage. That boundary is deliberate. The article studies what happens when the transform is narrow enough to own inside one process and the main engineering question becomes data representation, ownership, allocation, and lifetime.

## Why Not Flink?

Flink, Spark, Beam, and similar systems are the right tools when the system needs distributed execution, operational connectors, event-time windows, checkpointing, recovery, cluster elasticity, and mature runtime controls. This project is not a replacement for that class of system.

The custom C++ pipeline represents a different corner of the design space. It is plausible when input already arrives in bounded batches, reference data fits in memory, the transform is stable, deterministic replay is sufficient, deployment needs a small native component, or infrastructure cost and per-core efficiency matter more than distributed flexibility.

The article should therefore avoid the claim that C++ is categorically better than a stream processor. The more precise claim is that general stream processors optimize for distributed correctness and operational flexibility, while this project studies a fixed business transform with aggressive control over memory lifetime inside one process.

```mermaid
flowchart TB
    question{Which problem is this?}
    question --> distributed[Distributed stream processing]
    question --> local[Fixed local transform]

    distributed --> flink[Flink or similar runtime]
    flink --> connectors[Connectors and external systems]
    flink --> checkpointing[Checkpointing and recovery]
    flink --> windows[Event-time windows]
    flink --> elasticity[Cluster elasticity]

    local --> cpp[Custom C++ pipeline]
    cpp --> bounded[Bounded encoded batches]
    cpp --> memory[Reference data fits in memory]
    cpp --> deterministic[Deterministic replay and checksum]
    cpp --> lifetime[Explicit ownership and lifetime design]
    cpp --> cost[Low overhead per core or edge deployment]
```

## Business Documents

Each generated record is an encoded business document. It contains stable scalar fields, business identity, nested collections, and payload fragments. The encoding is intentionally simple because external I/O and wire-format parsing are not the subject of the first article.

```mermaid
classDiagram
    class EncodedBatch {
        WorkloadConfig config
        vector~EncodedEvent~ events
        size_t total_bytes
    }

    class EncodedEvent {
        string bytes
    }

    class DecodedEvent {
        uint64 event_id
        uint32 source_id
        uint64 timestamp_ns
        EventType event_type
        string company_name
        string jurisdiction
        identifiers[]
        addresses[]
        attributes[]
        relationships[]
        payload_fragments[]
    }

    class NormalizedRecord {
        canonical_company_name
        jurisdiction_id
        primary_external_id
        category_code
        normalized_addresses[]
        normalized_attributes[]
        relationship_refs[]
        payload_digest
        validation_flags
    }

    class AuditRecord {
        event_id
        source_id
        accepted
        validation_errors[]
        normalization_actions[]
        reference_lookup_hits
        payload_checksum
    }

    EncodedBatch "1" --> "*" EncodedEvent
    EncodedEvent --> DecodedEvent : decode
    DecodedEvent --> NormalizedRecord : normalize
    DecodedEvent --> AuditRecord : validate/audit
```

## Reference Data

Reference data is immutable for the duration of a pipeline run. It models application-lifetime data that would normally come from configuration, a database snapshot, or a broadcast state source in a distributed runtime.

Current reference data includes:

- jurisdiction code to jurisdiction id;
- source system id to category code;
- company-name aliases used during normalization.

```mermaid
flowchart LR
    decoded[Decoded document] --> jurisdiction[Jurisdiction lookup]
    decoded --> category[Source category lookup]
    decoded --> alias[Company alias lookup]

    ref[(Immutable reference data)]
    ref --> jurisdiction
    ref --> category
    ref --> alias

    jurisdiction --> normalized[Normalized record]
    category --> normalized
    alias --> normalized
    jurisdiction --> audit[Audit record]
    category --> audit
    alias --> audit
```

## Processing Semantics

Every implementation stage must perform the same business work:

1. Decode the deterministic encoded document.
2. Validate required fields.
3. Normalize selected text values.
4. Enrich from immutable reference data.
5. Construct normalized records.
6. Construct audit records.
7. Compute a stable checksum.
8. Release or reset batch-local state.

```mermaid
sequenceDiagram
    participant Batch as Encoded batch
    participant Stage as Pipeline stage
    participant Ref as Reference data
    participant Norm as Normalized output
    participant Audit as Audit output
    participant Sum as Checksum

    Batch->>Stage: encoded document bytes
    Stage->>Stage: decode fields and nested lists
    Stage->>Stage: validate required fields
    Stage->>Stage: normalize text and identifiers
    Stage->>Ref: lookup jurisdiction/category/alias
    Ref-->>Stage: immutable enrichment values
    Stage->>Norm: append normalized record
    Stage->>Audit: append audit record
    Norm->>Sum: observable fields
    Audit->>Sum: observable audit facts
```

## Lifetime Categories

The lifetime model is central to the article. Later stages are allowed to change internal representation only when they preserve these boundaries.

```mermaid
flowchart TB
    app[Application lifetime<br/>immutable reference data]
    input[Input-buffer lifetime<br/>encoded batch bytes]
    temp[Temporary parse/validation lifetime<br/>decoded fragments and scratch values]
    batch[Batch lifetime<br/>normalized and audit records]
    output[Output lifetime<br/>records that survive commit]
    irregular[Irregular lifetime<br/>exceptions and rare independent objects]

    app --> temp
    input --> temp
    temp --> batch
    batch --> output
    temp --> irregular
    batch --> irregular
```

Borrowed memory must identify its owner and lifetime boundary. No stage may rely on dangling `std::string_view`, `std::span`, pointer, or index assumptions.

