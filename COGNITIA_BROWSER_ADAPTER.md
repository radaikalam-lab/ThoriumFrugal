# Cognitia Browser Adapter — Architecture & Integration Specification

**Version:** 1.1.0 (Phase 3A Observation + Phase 3B Authorized Content Bridge)  
**Target Browser:** Lean Thorium `138.0.7204.306`  
**Cognitia Repository Path:** `E:\Cognitia`  
**Working Root:** `E:\Thorium`

---

## 1. Architectural Role & Invariants

The Cognitia Browser Adapter provides a strictly decoupled, asynchronous, read-only observation and authorized content channel bridging Lean Thorium and the Cognitia Cognitive Engine.

```text
┌─────────────────────────────────────────────────────────────────────────────────────────┐
│                                     LEAN THORIUM                                        │
│                                                                                         │
│   ┌─────────────────────────────────────────────────────────────────────────────────┐   │
│   │  Browser UI & Content Layer                                                     │   │
│   │   • TabStripModelObserver  ──► [TabCreated, TabActivated, TabClosed]            │   │
│   │   • WebContentsObserver    ──► [NavStarted, NavCommitted, TitleChanged]         │   │
│   │   • ContentExtractor       ──► [User-Authorized Selected Text / Page Content]   │   │
│   └────────────────────────────────────────┬────────────────────────────────────────┘   │
│                                            │ Fast Async Call (<0.05ms)                  │
│   ┌────────────────────────────────────────▼────────────────────────────────────────┐   │
│   │  Cognitia Browser Adapter (chrome/browser/cognitia_adapter/)                    │   │
│   │   • URL Sanitizer (Credentials & Token Scrubber)                                │   │
│   │   • Observation & Content Envelope Builders (Cognitia ABI v1.0.0 Compliant)     │   │
│   │   • Bounded Event Queue & Disconnected Content Drop Safety Policy               │   │
│   └────────────────────────────────────────┬────────────────────────────────────────┘   │
└────────────────────────────────────────────┼────────────────────────────────────────────┘
                                             │ Asynchronous Background OS Named Pipe
                                             │ (\\.\pipe\cognitia_browser_stream)
                                             ▼
┌─────────────────────────────────────────────────────────────────────────────────────────┐
│                                 COGNITIA DAEMON (E:\Cognitia)                           │
│                                                                                         │
│   ┌─────────────────────────────────────────────────────────────────────────────────┐   │
│   │  Cognitive ABI & Ingestion Pipeline (src/cognitia/abi/ & epistemic/)            │   │
│   │   • Deterministic ABI Serializer                                                │   │
│   │   • Epistemic Observation Ingestion & Provenance Graph Recording                │   │
│   │   • Directional Programming & Reasoning (No direct browser authority)           │   │
│   └─────────────────────────────────────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────────────────────────────────────┘
```

---

## 2. Invariants & Guarantees

1. **Unidirectional Observation & Authorized Content:** Information flows exclusively from Thorium to Cognitia. Cognitia cannot send commands or execute actions.
2. **WEB CONTENT ≠ INSTRUCTION:** Webpage content is strictly tagged as `"untrusted_web_content"` and never executed as instructions.
3. **Zero Credential Transmission:** Passwords, authentication headers, cookies, and embedded URL credentials (`user:pass@`) are never serialized.
4. **No Automatic DOM Scraping:** Webpage text is captured **ONLY upon explicit user action**.
5. **Complete Fault Isolation:**
   - If Cognitia is absent or not running: Thorium continues without interruption or error dialogs. Content-bearing events are dropped immediately without memory retention.
   - If Cognitia crashes: The Named Pipe closes cleanly, and Thorium continues normal browsing.
   - If Cognitia is slow: Background queues drop oldest events without blocking the browser UI thread.
