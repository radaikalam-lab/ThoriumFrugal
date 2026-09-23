# Lean Thorium — Privacy P1 Architecture & Implementation

**Subsystem:** Local Tracker Blocking, Anti-Tracking & Third-Party Classification  
**Phase:** Privacy P1  
**Status:** Validated & Frozen  
**Baseline:** Lean Thorium Phase 3 (`lean-thorium-v1.0.0-phase3-final`)  

---

## 1. Architectural Principle

Privacy authority resides strictly inside **Lean Thorium**.

```
                         LEAN THORIUM BROWSER
                                  │
                 ┌────────────────┴────────────────┐
                 │                                 │
                 ▼                                 ▼
      ┌───────────────────────┐       ┌───────────────────────┐
      │   Privacy Subsystem   │       │   Cognitia Adapter    │
      │  (Decision Authority: │       │   (Observation Only)  │
      │   Lean Thorium)       │       └───────────┬───────────┘
      └──────────┬────────────┘                   │ local IPC
                 │                                ▼
                 │ Privacy Event              COGNITIA
                 └──────────────────────────► Epistemic Core
                                              (Authority: NONE)
```

- **Zero Cognitia Decision Authority:** Cognitia never makes blocking or privacy decisions. Privacy events are received solely as passive observations.
- **Local-First & Offline:** No remote reputation lookups, no cloud telemetry, no analytics beacons, and no dynamic unverified downloads.

---

## 2. Core Components

1. **Third-Party Classifier (`third_party_classifier/`):**
   - Deterministic classification: `FIRST_PARTY`, `SAME_SITE`, `THIRD_PARTY`, `KNOWN_TRACKER`.
   - Multi-part public suffix support (`.co.uk`, `.com.au`, `.co.jp`, etc.).

2. **Tracking-Parameter Stripper (`tracking_parameter_filter/`):**
   - Strips high-confidence tracking query parameters (`utm_*`, `gclid`, `fbclid`, `msclkid`, `dclid`, `mc_eid`, `yclid`, `igshid`, `twclid`, `mkt_tok`).
   - Strictly preserves functional parameters (`id`, `q`, `search`, `page`, `token`, `session`, `auth`, `code`, `state`, `redirect_uri`).

3. **Tracker Blocker & Rule Engine (`tracker_blocker/`):**
   - High-speed domain/subdomain and path matching.
   - Resource-type filtering (`script`, `image`, `xhr_fetch`, `sub_frame`, `ping_beacon`).
   - Explicit allow exception rule evaluation (allow rules override block rules).
   - Bounded memory footprint and bounded execution time (< 15 µs per lookup).

4. **Privacy Policy Manager (`privacy_policy/`):**
   - Per-site policy overrides (`site_exceptions.json`).
   - Global on/off toggle.
   - Conservative P1 policy: `KNOWN_TRACKER + THIRD_PARTY` → `BLOCK`.

5. **Local Privacy Event Dispatcher (`privacy_events/`):**
   - Emits internal events: `privacy.request_blocked`, `privacy.request_allowed`, `privacy.tracker_detected`, `privacy.tracking_parameter_removed`, `privacy.third_party_detected`, `privacy.policy_exception_applied`.
   - Invariant: Zero credentials, cookies, post bodies, or session tokens in event telemetry.