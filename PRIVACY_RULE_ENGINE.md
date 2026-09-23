# Privacy Rule Engine & Filter Specification

**Document Version:** 1.1.0
**Engine Type:** Deterministic Domain, Host & Path Indexed Matcher
**Phase:** Privacy P1.1

---

## 1. Supported Rule Syntax

Lean Thorium Privacy P1.1 implements a bounded, secure subset of standard filter list rules:

| Rule Syntax | Description | Example | Precedence |
|---|---|---|---|
| `@@\|\|domain.com^` | Explicit allow exception (overrides block rules) | `@@\|\|legitimate.cdn.com^` | High |
| `\|\|domain.com^` | Block `domain.com` and all subdomains (`*.domain.com`) | `\|\|google-analytics.com^` | Standard |
| `\|\|domain.com/path^` | Block `domain.com` for paths matching `/path` | `\|\|facebook.com/tr^` | Standard |
| `\|\|domain.com^$third-party` | Block only when requested from a 3rd-party origin | `\|\|doubleclick.net^$third-party` | Standard |
| `\|\|domain.com^$script` | Block only script resources | `\|\|tracker.com^$script` | Standard |

---

## 2. Algorithmic Lookup Complexity

The Rule Database implements direct indexed lookup using domain candidate generation rather than $O(N)$ full list scanning:

```text
Target Host: ads.sub.tracker.example.com

Candidate Hierarchy:
1. ads.sub.tracker.example.com
2. sub.tracker.example.com
3. tracker.example.com
4. example.com
```

- **Lookup Complexity:** $O(\text{labels} + \text{bucket\_rules})$ where $\text{labels} \le 5$ in standard hostnames.
- Hash map lookup is executed only for candidate domains.

---

## 3. Real C++ Performance Benchmark Results

Release build (`-O3`) benchmark measuring 100,000 requests across varying rule set sizes:

| Active Rules | Operation | p50 Latency | p95 Latency | p99 Latency | Max Latency |
|---|---|---|---|---|---|
| **10** | BLOCK | 3.7 µs | 4.2 µs | 17.1 µs | 51.9 µs |
| **10** | ALLOW | 2.5 µs | 2.5 µs | 2.9 µs | 69.0 µs |
| **10** | MISS | 2.1 µs | 2.2 µs | 3.6 µs | 38.0 µs |
| **100** | BLOCK | 3.8 µs | 7.8 µs | 23.2 µs | 4.7 ms |
| **100** | ALLOW | 2.5 µs | 3.5 µs | 5.2 µs | 67.4 µs |
| **100** | MISS | 2.3 µs | 2.4 µs | 2.6 µs | 45.2 µs |
| **1,000** | BLOCK | 3.9 µs | 6.7 µs | 7.7 µs | 139.6 µs |
| **1,000** | ALLOW | 2.6 µs | 3.3 µs | 5.2 µs | 72.5 µs |
| **1,000** | MISS | 2.3 µs | 4.6 µs | 4.9 µs | 36.7 µs |
| **10,000** | BLOCK | 3.9 µs | 4.2 µs | 7.0 µs | 43.6 µs |
| **10,000** | ALLOW | 2.6 µs | 2.7 µs | 3.2 µs | 51.9 µs |
| **10,000** | MISS | 2.3 µs | 2.3 µs | 2.9 µs | 27.5 µs |
| **50,000** | BLOCK | 3.8 µs | 4.5 µs | 18.7 µs | 189.9 µs |
| **50,000** | ALLOW | 2.6 µs | 2.7 µs | 4.0 µs | 26.5 µs |
| **50,000** | MISS | 2.2 µs | 2.3 µs | 2.5 µs | 38.5 µs |

---

## 4. Resource Protection & Safety Limits

- **Max Rule Length:** 1024 characters.
- **Max Rule Database Capacity:** 50,000 rules.
- **ReDoS Protection:** Strict tokenized prefix and suffix comparisons (no backtracking regex).
- **Fault Resilience:** Malformed rules are discarded; parser errors log diagnostics and default to safe allow disposition.
