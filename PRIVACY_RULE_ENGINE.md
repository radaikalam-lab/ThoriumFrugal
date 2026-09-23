# Privacy Rule Engine & Filter Specification

**Document Version:** 1.0.0  
**Engine Type:** Deterministic Domain, Host & Path Matcher  

---

## 1. Supported Rule Syntax

Lean Thorium Privacy P1 implements a bounded, secure subset of standard filter list rules:

| Rule Syntax | Description | Example |
|---|---|---|
| `||domain.com^` | Block `domain.com` and all subdomains (`*.domain.com`) | `||google-analytics.com^` |
| `||domain.com/path^` | Block `domain.com` for paths matching `/path` | `||facebook.com/tr^` |
| `||domain.com^$third-party` | Block only when requested from a 3rd-party origin | `||doubleclick.net^$third-party` |
| `||domain.com^$script` | Block only script resources | `||tracker.com^$script` |
| `@@||domain.com^` | Explicit allow exception (overrides all block rules) | `@@||cdnjs.cloudflare.com^` |

---

## 2. Resource Protection & Safety Limits

- **Max Rule Length:** 1024 characters.
- **Max Rule Database Capacity:** 50,000 rules.
- **Lookup Complexity:** $O(1)$ domain hash lookup + bounded suffix verification.
- **ReDoS Protection:** No arbitrary backtracking regular expressions are evaluated.
- **Fault Resilience:** Malformed rules are skipped with diagnostics; the browser never crashes.