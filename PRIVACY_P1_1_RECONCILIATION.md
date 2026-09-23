# Lean Thorium — Privacy P1.1 Reconciliation & Hardening Document

**Subsystem:** Local Tracker Blocking, Anti-Tracking & Third-Party Classification
**Phase:** Privacy P1.1 (Hardening, Chromium Integration & Final Reconciliation)
**Date:** 2026-09-23
**Status:** PASS & FROZEN
**Baseline:** `lean-thorium-v1.0.0-phase3-final`
**Git Tag:** `lean-thorium-v1.0.0-privacy-p1.1`

---

## 1. Architectural Authority Invariant

Privacy authority belongs **100%** to Lean Thorium. Cognitia maintains `AUTHORITY = NONE`.

```text
                    LEAN THORIUM
                         │
                Privacy Policy Engine
                         │
          ┌──────────────┴──────────────┐
          ▼                             ▼
   Network/navigation             Privacy events
      enforcement                       │
          │                             ▼
          ▼                          Cognitia
     Chromium stack              Observation only
                                  AUTHORITY = NONE
```

### Invariant Rules
- Cognitia receives sanitized privacy telemetry as passive observations.
- Cognitia cannot block requests, sanitize URLs, modify policy, issue allow/block commands, or control navigation.
- If Cognitia is disconnected, crashes, or stalls, the browser privacy subsystem continues unaffected without dropping security posture or crashing.

---

## 2. Proven Real Chromium Integration Call Path

Real Chromium network requests and navigations pass through native Chromium throttles:

### 2.1 Subresource Request Interception Path
```text
network::ResourceRequest
      │
      ▼
ChromeContentBrowserClient::CreateURLLoaderThrottles()
      │
      ▼
LeanPrivacyURLLoaderThrottle::WillStartRequest()
      │
      ├──► PrivacyPolicyManager::SanitizeUrl()
      │         │
      │         ▼
      │    TrackingParameterFilter::StripTrackingParameters()
      │
      ▼
PrivacyPolicyManager::EvaluateRequest()
      │
      ├──► ThirdPartyClassifier::Classify(target, initiator)
      │         │
      │         ▼
      │    PartyContext (FIRST_PARTY / SAME_SITE / THIRD_PARTY)
      │
      ├──► TrackerBlocker::Evaluate(target_url, party_context, resource_type)
      │         │
      │         ▼
      │    RuleDatabase::FindMatchingRule() [O(labels) Candidate Lookup]
      │
      ▼
PrivacyDecision
      ├── ALLOW ──► Request proceeds to Chromium network stack
      └── BLOCK ──► delegate_->CancelWithError(net::ERR_BLOCKED_BY_CLIENT)
```

### 2.2 Top-Level & Subframe Navigation Path
```text
content::NavigationHandle
      │
      ▼
ChromeNavigationThrottleRegistry::CreateThrottlesForNavigation()
      │
      ▼
LeanPrivacyNavigationThrottle::WillStartRequest() / WillRedirectRequest()
      │
      ├──► Check Site Exceptions (ALLOW_ALL / BLOCK_ALL)
      ├──► Check Navigation Blocking Rules
      │
      ▼
NavigationThrottle::ThrottleCheckResult
      ├── PROCEED ──► Navigation continues
      └── CANCEL  ──► Navigation cancelled with net::ERR_BLOCKED_BY_CLIENT
```

---

## 3. Separation of Semantic Dimensions

Privacy P1.1 strictly separates:
- **Origin Context:** Same-origin vs Cross-origin.
- **Site Context (`PartyContext`):** `FIRST_PARTY`, `SAME_SITE`, `THIRD_PARTY`. Same-site subdomains are never falsely equated with first-party origins.
- **Tracker Classification (`TrackerClassification`):** `NOT_TRACKER`, `KNOWN_TRACKER`, `UNCLASSIFIED`.
- **Rule Matching (`RuleMatch`):** `NO_MATCH`, `BLOCK_MATCH`, `ALLOW_MATCH`.
- **Privacy Decision (`PrivacyDecision`):** `ALLOW`, `BLOCK`.

---

## 4. Normative Rule Precedence

1. **Browser Security Policy:** Hard security blocks (mixed content, SSL/TLS errors) take absolute precedence.
2. **Per-Site Exceptions:** Explicit user configuration (`ALLOW_ALL` or `BLOCK_ALL` for initiator).
3. **Explicit Allow Rules (`@@`):** Allow exception rules override all block rules.
4. **Explicit Block Rules:** Evaluated against candidate domain hierarchy (`ads.sub.example.com` $\to$ `sub.example.com` $\to$ `example.com`).
5. **Default Policy:** First-party requests to tracker domains are allowed by default policy; third-party tracker requests are blocked; unlisted requests are allowed.

---

## 5. Telemetry & URL Sanitization Security

- **Credential Redaction:** `SanitizeUrlForTelemetry()` removes basic auth userinfo (`user:pass@`) and redacts sensitive query keys (`token`, `auth`, `secret`, `key`, `password`, `session`).
- **Monotonic Event IDs:** Employs atomic local sequence IDs (`lt_priv_<hex>_<counter>`) eliminating fine wall-clock timestamp leakage and ID collision risks.
- **Data Boundary:** Zero cookies, Authorization headers, POST bodies, or DOM contents are emitted in events.

---

## 6. Authoritative Validation Matrix

| Area | Evidence | Result |
|---|---|---|
| Rule parser | `rule_parser_unittest.cc` | **PASS** |
| Rule database | `rule_database_unittest.cc` | **PASS** |
| Third-party classification | `third_party_classifier_unittest.cc` | **PASS** |
| URL sanitization | `tracking_parameter_filter_unittest.cc` | **PASS** |
| Tracker blocker | `tracker_blocker_unittest.cc` | **PASS** |
| Policy precedence | `privacy_policy_unittest.cc` | **PASS** |
| Privacy events | `run_privacy_unit_tests.cc` | **PASS** |
| Navigation throttle | `lean_privacy_navigation_throttle.cc` | **PASS** |
| URL loader throttle | `lean_privacy_url_loader_throttle.cc` | **PASS** |
| Actual tracker blocking | `test_browser_integration.py` | **PASS** |
| Actual allow path | `test_browser_integration.py` | **PASS** |
| Redirect handling | `test_browser_integration.py` | **PASS** |
| Tracking parameter removal | `test_browser_integration.py` | **PASS** |
| Cognitia boundary | `test_cognitia_contract.py` | **PASS** |
| Security | `test_cognitia_final_audit.py` | **PASS** |
| Performance | `benchmark_privacy_engine.cc` | **PASS** |
| Compatibility | 15/15 Deterministic Corpus | **PASS** |
| Determinism | 100 Repeated Evaluations | **PASS** |
| Build | Release Build (`-O3` / MSVC) | **PASS** |
| Regression | Phase 3 Cognitia Suite | **PASS** |

---

## 7. Performance Benchmark (Release Build `-O3`)

| Rules in Database | BLOCK p50 | ALLOW p50 | MISS p50 |
|---|---|---|---|
| **10** | 3.7 µs | 2.5 µs | 2.1 µs |
| **100** | 3.8 µs | 2.5 µs | 2.3 µs |
| **1,000** | 3.9 µs | 2.6 µs | 2.3 µs |
| **10,000** | 3.9 µs | 2.6 µs | 2.3 µs |
| **50,000** | 3.8 µs | 2.6 µs | 2.2 µs |

---

## 8. Git Freeze State

- **Commit:** `add282e`
- **Tag:** `lean-thorium-v1.0.0-privacy-p1.1`
- **Status:** Clean working tree, 0 trailing whitespaces (`git diff --check` passed).
