# Cognitia Browser Integration — Architectural Specification

**Document Version:** 1.0.0 (Phase 3 Final)  
**Host Browser:** Lean Thorium `138.0.7204.306`  
**Cognitia Repository:** `E:\Cognitia`  
**Thorium Repository:** `E:\Thorium`

---

## 1. Architectural Topology & Invariants

```text
┌─────────────────────────────────────────────────────────────────────────────────────────┐
│                                     LEAN THORIUM                                        │
│                                                                                         │
│   ┌─────────────────────────────────────────────────────────────────────────────────┐   │
│   │  Sandboxed Browser Core & UI Layer                                              │   │
│   │   • TabStripModelObserver  ──► [TabCreated, TabActivated, TabClosed]            │   │
│   │   • WebContentsObserver    ──► [NavStarted, NavCommitted, TitleChanged]         │   │
│   │   • ContentExtractor       ──► [User-Authorized Selected Text / Page Content]   │   │
│   └────────────────────────────────────────┬────────────────────────────────────────┘   │
│                                            │ Fast Async Call (<0.05ms)                  │
│   ┌────────────────────────────────────────▼────────────────────────────────────────┐   │
│   │  Thorium Cognitia Adapter (chrome/browser/cognitia_adapter/)                    │   │
│   │   • URL Sanitizer (Credentials & Sensitive Token Scrubber)                      │   │
│   │   • Canonical Cognitia ABI v1.0.0 Serializer                                    │   │
│   │   • Bounded Event Queue & Disconnected Content Drop Safety Policy               │   │
│   └────────────────────────────────────────┬────────────────────────────────────────┘   │
└────────────────────────────────────────────┼────────────────────────────────────────────┘
                                             │ Asynchronous Background Windows Named Pipe
                                             │ (\\.\pipe\cognitia_browser_stream)
                                             ▼
┌─────────────────────────────────────────────────────────────────────────────────────────┐
│                              COGNITIA COGNITIVE PLANE (E:\Cognitia)                     │
│                                                                                         │
│   ┌─────────────────────────────────────────────────────────────────────────────────┐   │
│   │  Cognitive ABI & Ingestion Pipeline (src/cognitia/abi/ & epistemic/)            │   │
│   │   • Deterministic Canonical Serializer & Checksummer                            │   │
│   │   • Epistemic Observation Ingestion (Status: OBSERVED)                          │   │
│   │   • Immutable Provenance Graph (SourceType: SENSOR)                             │   │
│   │   • Advisory Directional Reasoning & Proposal Synthesis (NO browser authority)  │   │
│   └─────────────────────────────────────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────────────────────────────────────┘
```

---

## 2. Core Architectural Principles

1. **Strict Dependency Direction:** Thorium adapter depends upon canonical Cognitia ABI and contracts. Cognitia never imports Chromium headers or depends on Chromium runtime internals.
2. **Zero Reverse Execution Authority:** Communication is strictly unidirectional. Cognitia emits advisory proposals for human review; it cannot issue browser execution directives.
3. **No Embedded Python in Chromium:** Chromium remains pure C++. Inter-process communication across the language barrier is mediated via local Windows Named Pipes.
