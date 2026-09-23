# Cognitia Browser Adapter for Lean Thorium

**Version:** 1.0.0  
**Subsystem:** `chrome/browser/cognitia_adapter`  
**Authority Level:** **OBSERVATION ONLY** (Zero Browser Execution Authority)  
**IPC Channel:** Windows Local Named Pipe (`\\.\pipe\cognitia_browser_stream`)

---

## 1. Overview

The Cognitia Browser Adapter is an isolated, lightweight, asynchronous C++ bridge connecting Lean Thorium with the external Cognitia Cognitive Subsystem.

It provides real-time browser observation events (tab lifecycle, navigations, page metadata) to Cognitia without giving Cognitia any direct authority over browser operations, navigation, DOM mutation, or JavaScript execution.

---

## 2. Directory Architecture

```text
cognitia_adapter/
  ├── README.md                          <- Architecture and integration overview
  ├── cognitia_adapter_boundary.h        <- Top-level interface definition
  ├── observation/
  │     ├── observation_types.h          <- Event enum types (tab, nav, page)
  │     ├── browser_observation_collector.h / .cc <- TabStrip & WebContents observer hooks
  ├── protocol/
  │     ├── observation_envelope.h / .cc <- Versioned ABI-compliant envelope
  │     ├── url_sanitizer.h / .cc        <- Sensitive token and credential scrubber
  ├── transport/
  │     ├── cognitia_transport.h         <- Abstract transport interface
  │     ├── named_pipe_transport_win.h / .cc <- Local Windows Named Pipe transport
  │     ├── bounded_event_queue.h / .cc  <- Bounded ring-buffer with drop-oldest policy
  ├── serialization/
  │     ├── json_serializer.h / .cc      <- Canonical deterministic JSON serialization
  └── tests/
        └── cognitia_adapter_unittest.cc <- Unit & security isolation tests
```

---

## 3. Invariants & Security Guarantees

1. **Strict Epistemic Isolation:** Observations flow exclusively from Thorium to Cognitia. Cognitia cannot send commands or mutate browser state.
2. **Zero Credential Exposure:** Cookies, passwords, authentication headers, and URL user:pass fragments are scrubbed prior to serialization.
3. **Asynchronous Non-Blocking Execution:** Observation dispatch runs on a dedicated background `SequencedTaskRunner` (< 0.1ms overhead on browser UI thread).
4. **Resilient Failure Tolerance:** If Cognitia is absent, slow, or crashes, Thorium continues unaffected with zero user-visible error dialogs.
