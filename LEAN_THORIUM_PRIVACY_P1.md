# Lean Thorium — Privacy P1.1 Architecture & Implementation

**Subsystem:** Local Tracker Blocking, Anti-Tracking & Third-Party Classification
**Phase:** Privacy P1.1 (Hardening, Chromium Integration & Final Reconciliation)
**Status:** Validated, Hardened & Frozen
**Baseline:** Lean Thorium Phase 3 (`lean-thorium-v1.0.0-phase3-final`)

---

## 1. Architectural Principle & Authority Invariant

Privacy authority resides strictly inside **Lean Thorium**.

```text
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

- **Zero Cognitia Decision Authority:** Cognitia has `AUTHORITY = NONE`. Cognitia receives privacy observations but never blocks, allows, or commands network or browser state.
- **Local-First & Offline:** No remote reputation lookups, no cloud telemetry, no analytics beacons, and no dynamic unverified downloads.
- **Safe Subsystem Fallback:** On internal privacy subsystem error, the default fallback is `ALLOW` (unless standard Chromium security policies independently dictate blocking).

---

## 2. Real Chromium Network Interception Call Path

Privacy evaluation is integrated directly into the Chromium request interception pipeline via `URLLoaderThrottle` and `NavigationThrottle`:

```text
Actual Chromium Network Request (network::ResourceRequest)
              │
              ▼
ChromeContentBrowserClient::CreateURLLoaderThrottles()
              │
              ▼
LeanPrivacyURLLoaderThrottle::WillStartRequest()
              │
              ├──► PrivacyPolicyManager::SanitizeUrl() [Query Parameter Stripping]
              │
              ▼
PrivacyPolicyManager::EvaluateRequest()
              │
              ├──► ThirdPartyClassifier::Classify() [PartyContext]
              │
              ├──► TrackerBlocker::Evaluate() [RuleDatabase Candidate Lookup]
              │
              ▼
        PrivacyDecision
       ┌──────┴──────┐
       ▼             ▼
     ALLOW         BLOCK
       │             │
       ▼             ▼
 Chromium          delegate_->CancelWithError(net::ERR_BLOCKED_BY_CLIENT)
 continues
```

---

## 3. Formal Semantic Dimensions

Privacy P1.1 strictly separates orthogonal semantic dimensions:
1. **Origin Relationship:** `SAME_ORIGIN` vs `CROSS_ORIGIN`.
2. **Site Relationship (`PartyContext`):**
   - `FIRST_PARTY`: Initiator host equals target host.
   - `SAME_SITE`: Initiator and target share the same registrable domain (e.g. `sub.example.com` and `app.example.com`).
   - `THIRD_PARTY`: Initiator and target have distinct registrable domains.
3. **Tracker Classification (`TrackerClassification`):**
   - `NOT_TRACKER`: Known benign or unlisted host.
   - `KNOWN_TRACKER`: Host matches a designated tracking entity rule.
   - `UNKNOWN`: Default unclassified resource.
4. **Rule Matching (`RuleMatch`):**
   - `NO_MATCH`, `BLOCK_MATCH`, `ALLOW_MATCH`.
5. **Privacy Decision (`PrivacyDecision`):**
   - `ALLOW`: Permitted to proceed.
   - `BLOCK`: Cancelled with client error (`ERR_BLOCKED_BY_CLIENT`).

---

## 4. Normative Rule Precedence

Precedence is strictly deterministic and evaluated in the following order:
1. **Browser Security Policy:** Hard security blocks (e.g. mixed content, certificate errors) always take absolute precedence.
2. **Per-Site Exceptions:** Explicit user configuration (`ALLOW_ALL` or `BLOCK_ALL` for the initiator site).
3. **Explicit Allow Rules (`@@`):** Allow exception rules override all block rules.
4. **Explicit Block Rules:** Evaluated against candidate domains with resource-type and third-party restrictions.
5. **Default Policy:** First-party requests to tracker domains are allowed by default policy; third-party requests to tracker domains are blocked; unlisted requests are allowed.

---

## 5. Core Components

1. **Third-Party Classifier (`third_party_classifier/`):**
   - Deterministic classification: `FIRST_PARTY`, `SAME_SITE`, `THIRD_PARTY`.
   - Multi-part public suffix support (`.co.uk`, `.com.au`, `.co.jp`, etc.).
2. **Tracking-Parameter Stripper (`tracking_parameter_filter/`):**
   - Strips high-confidence tracking query parameters (`utm_*`, `gclid`, `fbclid`, `msclkid`, `dclid`, `mc_eid`, `yclid`, `igshid`, `twclid`, `mkt_tok`).
   - Strictly preserves functional query parameters (`id`, `q`, `search`, `page`, `token`, `session`, `auth`, `code`, `state`, `redirect_uri`).
   - Preserves query component ordering, duplicates, and fragments.
3. **Tracker Blocker & Indexed Rule Engine (`tracker_blocker/`):**
   - Fast $O(\text{hostname labels} + \text{bucket rules})$ domain hash lookup with candidate suffix generation.
   - Explicit allow exceptions (`@@`) and resource-type constraints (`script`, `image`, `xhr_fetch`, `sub_frame`, `ping_beacon`).
   - Average lookup latency: 2.2 – 3.9 µs across 50,000 rules.
4. **Privacy Policy Manager (`privacy_policy/`):**
   - Per-site policy overrides (`site_exceptions.json`).
   - Global on/off toggle.
   - URL Sanitizer for telemetry: redacts passwords, credentials, and sensitive access tokens.
5. **Local Privacy Event Dispatcher (`privacy_events/`):**
   - Emits internal local telemetry with unique monotonic event IDs (`lt_priv_<hex>_<counter>`).
   - Zero credentials, cookies, POST bodies, or session tokens transmitted.
