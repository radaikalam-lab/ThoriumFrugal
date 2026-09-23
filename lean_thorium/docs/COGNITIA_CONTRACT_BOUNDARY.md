# Cognitia Contract & Semantic Boundary Specification

**Document Version:** 1.0.0 (Phase 3 Final)

---

## 1. Contract Ownership Matrix

| Subsystem / Semantic Entity | Canonical Owner | Contract Reference Location | Role of Thorium Browser Adapter |
| :--- | :--- | :--- | :--- |
| **Cognitive ABI & Base Object** | **Cognitia** | `E:\Cognitia\src\cognitia\abi\types.py` | Produces canonical `Observation` objects with UUIDv4, ISO-8601 UTC timestamp, schema `1.0.0` |
| **Epistemic Ingestion & Nodes** | **Cognitia** | `E:\Cognitia\src\cognitia\epistemic\service.py` | Ingests observations as `EpistemicNode` (`OBSERVED`); links evidence |
| **Provenance Graph & Lineage** | **Cognitia** | `E:\Cognitia\src\cognitia\provenance\record.py` | Maps to `SourceType.SENSOR`, `producer_id: "thorium_browser_adapter"`, parent lineage |
| **Directional Specifications** | **Cognitia** | `E:\Cognitia\contracts\directional-programming-contract.md` | Receives advisory proposals (`PROPOSED` status) without execution authority |
| **Browser Event Extraction** | **Thorium** | `E:\Thorium\lean_thorium\src\chrome\browser\cognitia_adapter\` | Captures tab lifecycle, navigations, and user-authorized content |
| **Security & Credential Scrub** | **Thorium** | `protocol/url_sanitizer.h` & `content_extractor.h` | Filters passwords, cookies, tokens, and sanitizes URLs prior to IPC |
| **Local OS Transport & Queue** | **Thorium** | `transport/named_pipe_transport_win.h` & `bounded_event_queue.h` | Dispatches events over local Named Pipe with Logon SID ACL; drops content when offline |

---

## 2. Absolute Semantic Separation Invariant

$$\text{Observation} \neq \text{Interpretation} \neq \text{Claim} \neq \text{Hypothesis} \neq \text{Decision} \neq \text{Action}$$

- **Observation:** External sensor telemetry emitted by browser.
- **Evidence:** Observation linked with direction (`SUPPORT`/`REFUTE`) toward an epistemic target.
- **Claim:** Proposition with evaluated confidence.
- **Decision:** Advisory suggestion presented to the user.
- **Action:** Executed strictly by the authorized human user.
