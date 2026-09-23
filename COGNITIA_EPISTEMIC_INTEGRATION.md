# Cognitia Epistemic Subsystem Integration Specification

**Subsystem Reference:** `E:\Cognitia\src\cognitia\epistemic\` & `E:\Cognitia\contracts\epistemic-contract.md`

---

## 1. Epistemic Ingestion Pipeline

When Thorium emits a browser observation, it flows through the epistemic lifecycle without blurring sensor telemetry with belief or authority:

```text
┌─────────────────────────────────────────────────────────────────────────────────────────┐
│                              EPISTEMIC INGESTION PIPELINE                               │
│                                                                                         │
│   [THORIUM BROWSER EVENT]                                                               │
│          │                                                                              │
│          ▼ (Named Pipe ABI JSON)                                                        │
│   [Observation(id, payload, source_id="thorium_browser_adapter")]                       │
│          │                                                                              │
│          ▼                                                                              │
│   [EpistemicService.record_observation(obs)] ──► EpistemicNode(status: OBSERVED)        │
│          │                                                                              │
│          ▼                                                                              │
│   [Evidence(target_id=claim.id, observation=obs, direction=SUPPORT)]                    │
│          │                                                                              │
│          ▼                                                                              │
│   [EpistemicService.transition_state(claim_node.id, SUPPORTED)]                         │
└─────────────────────────────────────────────────────────────────────────────────────────┘
```

---

## 2. Epistemic Status State Machine

1. **`UNKNOWN`:** Unmonitored or unparsed browser context.
2. **`OBSERVED`:** Raw observation recorded in Epistemic graph with `ProvenanceRecord(SourceType.SENSOR)`.
3. **`HYPOTHESIS`:** Inferred user intent or document property.
4. **`SUPPORTED`:** Corroborated by deterministic URL, title, or navigation evidence.
5. **`REFUTED`:** Contradicted by navigation failure (e.g. HTTP 404/500 status code).
6. **`UNRESOLVED`:** Inconclusive or conflicting signals.
