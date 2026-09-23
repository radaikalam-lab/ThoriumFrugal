# Cognitia Browser Adapter — Security & Authority Boundary Specification

**Document Version:** 1.0.0 (Phase 3A)  
**Security Level:** Epistemic Isolation / Zero Execution Authority

---

## 1. Epistemic Separation Invariant

The fundamental security model of the Cognitia-Thorium integration is strict separation between observation, reasoning, and execution authority:

```text
┌─────────────────────────────────────────────────────────────────────────────────────────┐
│                                 AUTHORITY ARCHITECTURE                                  │
│                                                                                         │
│   [UNTRUSTED WEB CONTENT] ──► [SANDBOXED BROWSER RENDERER]                              │
│                                           │                                             │
│                                           ▼ (Observation Only)                          │
│                               [COGNITIA BROWSER ADAPTER]                                │
│                                           │                                             │
│                                           ▼ (Local Named Pipe)                          │
│                               [COGNITIA REASONING ENGINE]                               │
│                                           │                                             │
│                                           ▼ (Advisory Proposal)                         │
│                               [HUMAN / USER CONFIRMATION]                               │
│                                           │                                             │
│                                           ▼ (Explicit Approval)                         │
│                               [BROWSER ACTION EXECUTION]                                │
└─────────────────────────────────────────────────────────────────────────────────────────┘
```

1. **Cognitia holds ZERO execution privileges:** It cannot issue remote commands, navigate tabs, click buttons, or execute scripts.
2. **Untrusted Input Rule:** Webpage content arriving from the browser is marked as `UNTRUSTED_SENSOR_DATA`. It can never be treated as system prompts or developer instructions.
3. **Local-Only Access:** The Windows Named Pipe (`\\.\pipe\cognitia_browser_stream`) is created with Windows Security Attributes restricting access exclusively to the current user's security token (Logon SID). Network listeners (`0.0.0.0`) are disabled.
4. **Credential Isolation:** Passwords, authentication headers, and cookies are never collected or transmitted.
