# Privacy Threat Model & Boundary Invariants

**Document Version:** 1.1.0
**Threat Model Level:** Local-First Privacy Boundary
**Phase:** Privacy P1.1

---

## 1. Threat Vectors Addressed in Privacy P1.1

1. **Third-Party Behavioral Tracking:**
   - Mitigated by blocking requests to known tracking domains from third-party initiator origins via Chromium `URLLoaderThrottle` integration.
2. **Cross-Site Navigation Link Tracking:**
   - Mitigated by stripping tracking query parameters (`utm_*`, `fbclid`, `gclid`, `msclkid`, etc.) before network dispatch.
3. **Information Leakage via Telemetry / Logs:**
   - Mitigated by sanitizing URLs in telemetry: user credentials (username/password) and authentication tokens are stripped or redacted.
   - Privacy event IDs use atomic local monotonic sequences (`lt_priv_<hex>_<counter>`) instead of fine wall-clock timestamps.
4. **Denial of Service via Pathological Filter Lists:**
   - Mitigated by enforcing strict length limits (1024 chars), indexed candidate lookups, bounded memory, and absence of backtracking regular expressions.
5. **False Positive Functional Breakage:**
   - Mitigated by preserving functional query parameters, respecting explicit allow rules (`@@`), and providing per-site exceptions (`ALLOW_ALL`).
6. **Adversarial Web Content Injection:**
   - Webpage content cannot inject privacy rules or modify browser policy. Webpage strings remain passive data.

---

## 2. Architectural Invariants

1. **Authority Boundary:**
   - Decision authority is 100% within Lean Thorium.
   - Cognitia has `AUTHORITY = NONE`. Cognitia receives observations only and cannot alter decisions, block requests, or execute browser commands.
2. **Failure Isolation:**
   - Subsystem failure (rule parse failure, memory exhaustion, Cognitia disconnect) defaults to `ALLOW` and never crashes the browser.
3. **Future Trust Boundary for Rule Data Integrity:**
   - Untrusted rule list sources in future phases must undergo syntax validation, capacity limits, cryptographic integrity verification, and atomic installation.

---

## 3. Out of Scope for P1.1 (Documented Limitations)

- Fingerprint randomization (canvas, WebGL, audio, fonts) — deferred to future phases.
- Cookie/storage partitioning — deferred to future phases.
- Cloud reputation querying — strictly prohibited in local-first design.
- Automatic remote rule updates — out of scope for P1/P1.1.
