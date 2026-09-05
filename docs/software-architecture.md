# Software Architecture

The repository is organized around an article-friendly separation: stable semantics, stage implementations, measurement harnesses, and evidence. The code should make it difficult to change business behaviour accidentally while making it easy to compare memory and representation choices.

## Source Layout

```mermaid
flowchart TB
    include[include/throughput]
    src[src]
    tests[tests]
    benches[benchmarks/scenarios]
    docs[docs]
    article[article]

    include --> semantic[semantic_result.hpp<br/>stages.hpp<br/>domain/reference/generator headers]
    src --> core[domain.cpp<br/>reference_data.cpp<br/>generator.cpp<br/>checksum.cpp<br/>instrumentation.cpp]
    src --> stages[pipeline_baseline.cpp<br/>pipeline_reserved.cpp<br/>stages.cpp<br/>pipeline_shared.hpp]
    tests --> equivalence[determinism, baseline, allocation, stage equivalence tests]
    benches --> scenario[throughput_scenario.cpp]
    docs --> method[architecture, domain, methodology, testing, results]
    article --> draft[article draft and diagram sources]
```

## Layering

The stable layer defines what the pipeline means. Stage implementations define how that work is represented and allocated. The scenario runner and tests consume stage entry points through the semantic contract.

```mermaid
flowchart TB
    workload[Deterministic workload generation]
    reference[Immutable reference data]
    contract[SemanticResult contract]

    baseline[Stage: baseline]
    reserved[Stage: reserved]
    pmr[Stage: pmr]
    arena[Stage: arena]

    tests[CTest equivalence tests]
    scenario[Scenario measurement runner]
    evidence[Docs and article evidence]

    workload --> baseline
    workload --> reserved
    workload --> pmr
    workload --> arena
    reference --> baseline
    reference --> reserved
    reference --> pmr
    reference --> arena

    baseline --> contract
    reserved --> contract
    pmr --> contract
    arena --> contract

    contract --> tests
    contract --> scenario
    tests --> evidence
    scenario --> evidence
```

## Stage Contract

Each stage accepts the same encoded batch representation and immutable reference data. It may use different internal containers, allocation resources, or temporary representations, but it must emit behaviourally equivalent observable results.

```mermaid
flowchart LR
    input[EncodedBatch]
    ref[ReferenceData]
    input --> stage[Stage implementation]
    ref --> stage
    stage --> result[SemanticResult]
    result --> counts[normalized_records<br/>audit_records]
    result --> checksum[stable checksum]
```

The project currently keeps the original `PipelineResult` for the owning baseline API. The cross-stage API is `SemanticResult`, because later optimized stages should not be forced to expose the baseline object graph.

## Stage Roadmap

```mermaid
flowchart TB
    base[baseline<br/>ordinary owning C++]
    reserve[reserved<br/>same ownership, planned capacity]
    pmr[pmr<br/>standard polymorphic allocation]
    arena[arena<br/>project-owned monotonic resource]
    future[future representation stages<br/>views, compact layouts, reuse]

    base --> reserve
    reserve --> pmr
    pmr --> arena
    arena --> future
```

The sequence isolates variables:

- `baseline` establishes readable, idiomatic behaviour.
- `reserved` asks how much obvious capacity planning helps without changing ownership.
- `pmr` asks what batch-lifetime allocation changes when containers remain familiar.
- `arena` asks whether a project-owned resource gives different behaviour from standard PMR machinery.
- Future representation stages should change only one major variable at a time.

## Measured Region

The scenario runner constructs the reference data and generated input before measurement. The measured region contains only stage processing and checksum construction.

```mermaid
flowchart LR
    setup[Unmeasured setup<br/>reference data and generated batch]
    start[Start allocation/timer scope]
    work[Measured stage work<br/>decode, validate, normalize, enrich, output, checksum]
    stop[Stop timer and allocation scope]
    report[Print metrics]

    setup --> start --> work --> stop --> report
```

## Benchmark Integrity

The architecture exists to protect benchmark integrity. The same generated input, reference data, validation rules, normalization rules, and checksum rules must be used by every stage.

```mermaid
flowchart TB
    same[Must remain identical]
    same --> input[encoded input bytes]
    same --> reference[reference data]
    same --> rules[validation and normalization rules]
    same --> checksum[checksum semantics]
    same --> counts[observable result counts]

    variable[May vary by stage]
    variable --> allocation[allocation strategy]
    variable --> ownership[ownership model]
    variable --> layout[data layout]
    variable --> reuse[batch-local reuse/reset]
```

