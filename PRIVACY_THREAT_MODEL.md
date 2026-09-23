# Privacy Threat Model & Boundary Invariants

**Document Version:** 1.0.0  
**Threat Model Level:** Local-First Privacy Boundary  

---

## 1. Threat Vectors Addressed in Privacy P1

1. **Third-Party Behavioral Tracking:**
   - Mitigated by blocking requests to known tracking domains from third-party initiator origins.
2. **Cross-Site Navigation Link Tracking:**
   - Mitigated by stripping URL tracking query parameters (`utm_*`, `fbclid`, `gclid`, etc.) before navigation dispatch.
3. **Information Leakage via Telemetry:**
   - Mitigated by keeping privacy event telemetry strictly local. Zero remote endpoints are queried.
4. **Denial of Service via Pathological Filter Lists:**
   - Mitigated by enforcing strict length limits, hash-based indexing, bounded execution time, and safe fallbacks.
5. **False Positive Breakage:**
   - Mitigated by preserving functional query parameters, respecting explicit allow rules, and providing per-site exceptions.

---

## 2. Out of Scope for P1 (Documented Limitations)

- Fingerprint randomization (canvas, WebGL, audio, fonts).
- Custom cookie partitioning / sandbox redesign (deferred to future P2 evaluation).
- Cloud reputation querying (strictly prohibited in local-first architecture).