# Cognitia Provenance Mapping & Lineage Specification

**Subsystem Reference:** `E:\Cognitia\src\cognitia\provenance\` & `E:\Cognitia\contracts\provenance-contract.md`

---

## 1. Provenance Schema & Lineage Flow

Cognitia maintains an immutable provenance graph ensuring every derived cognitive artifact traces its origin back to the primary sensor observation:

```text
┌─────────────────────────────────────────────────────────────────────────────────────────┐
│                                PROVENANCE LINEAGE GRAPH                                 │
│                                                                                         │
│   [Thorium Observation] ──► ProvenanceRecord(                                           │
│                              source_type = SourceType.SENSOR,                           │
│                              producer_id = "thorium_browser_adapter",                   │
│                              capability_id = "browser_observation",                     │
│                              parent_ids = []                                            │
│                            )                                                            │
│                                      │                                                  │
│                                      ▼ Parent Link                                      │
│   [Derived Claim / Model] ─► ProvenanceRecord(                                          │
│                              source_type = SourceType.REASONING_ENGINE,                 │
│                              producer_id = "cognitia_epistemic_core",                   │
│                              capability_id = "claim_formulation",                       │
│                              parent_ids = [obs_provenance.id],                          │
│                              checksum = sha256(...)                                     │
│                            )                                                            │
└─────────────────────────────────────────────────────────────────────────────────────────┘
```

---

## 2. Invariants

1. **Source Distinction:** Sensor observations produced by Thorium are typed strictly as `SourceType.SENSOR`.
2. **Deterministic Checksumming:** SHA-256 digests are computed over canonical deterministic JSON strings.
3. **Immutable History:** Provenance records are append-only and cannot be mutated or rewritten.
