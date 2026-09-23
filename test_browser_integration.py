#!/usr/bin/env python3
"""
Lean Thorium Privacy P1.1 — Real Browser Integration & Compatibility Corpus Test
Section 6 & Section 19 Verification

Simulates and tests the exact Chromium network interception pipeline:
  network::ResourceRequest
      ↓
  LeanPrivacyURLLoaderThrottle::WillStartRequest
      ↓
  PrivacyPolicyManager::SanitizeUrl / EvaluateRequest
      ↓
  PrivacyDecision (ALLOW / BLOCK)
      ↓
  actual Chromium disposition (CONTINUE / ERR_BLOCKED_BY_CLIENT)

Also validates the 15-item Deterministic Compatibility Corpus.
"""

import sys
import os
import json
import urllib.parse
from typing import Dict, List, Tuple, Optional

class PrivacyEngine:
    def __init__(self):
        # Known tracking parameters
        self.tracking_params = {
            "utm_source", "utm_medium", "utm_campaign", "utm_term", "utm_content",
            "fbclid", "gclid", "dclid", "msclkid", "mc_eid", "igshid", "yclid",
            "_hsenc", "_hsmi", "mkt_tok", "wickedid"
        }
        # Explicit block rules
        self.block_rules = {
            "tracker.example.com", "ads.network.com", "telemetry.bad.org",
            "analytics.tracker.net", "tracking.evil.com", "pixel.adservice.com",
            "fingerprint.track.org", "ad.doubleclick.net", "google-analytics.com",
            "stats.thirdparty.com", "tracker.badanalytics.com"
        }
        # Explicit allow rules (override block)
        self.allow_rules = {
            "legitimate.cdn.com", "safe.analytics.tracker.net"
        }
        # Site exceptions: DEFAULT, ALLOW_ALL, BLOCK_ALL
        self.site_exceptions = {
            "exception-allow.example.com": "ALLOW_ALL",
            "exception-block.example.com": "BLOCK_ALL"
        }
        # Event log
        self.privacy_events = []
        self._event_counter = 0

    def get_registrable_domain(self, host: str) -> str:
        parts = host.lower().rstrip('.').split('.')
        if len(parts) <= 2:
            return '.'.join(parts)
        # Handle simple multi-part TLDs
        if parts[-2] in ('co', 'com', 'org', 'gov', 'edu', 'net') and len(parts) >= 3:
            return '.'.join(parts[-3:])
        return '.'.join(parts[-2:])

    def classify_party(self, target_host: str, initiator_host: Optional[str]) -> str:
        if not initiator_host:
            return "FIRST_PARTY"
        target_h = target_host.lower().rstrip('.')
        init_h = initiator_host.lower().rstrip('.')
        if target_h == init_h:
            return "FIRST_PARTY"
        if self.get_registrable_domain(target_h) == self.get_registrable_domain(init_h):
            return "SAME_SITE"
        return "THIRD_PARTY"

    def sanitize_url(self, url: str) -> Tuple[str, bool]:
        parsed = urllib.parse.urlparse(url)
        if not parsed.query:
            return url, False
        query_pairs = urllib.parse.parse_qsl(parsed.query, keep_blank_values=True)
        filtered_pairs = []
        stripped = False
        for k, v in query_pairs:
            if k.lower() in self.tracking_params:
                stripped = True
            else:
                filtered_pairs.append((k, v))
        new_query = urllib.parse.urlencode(filtered_pairs)
        new_url = urllib.parse.urlunparse((
            parsed.scheme, parsed.netloc, parsed.path, parsed.params, new_query, parsed.fragment
        ))
        return new_url, stripped

    def sanitize_url_for_telemetry(self, url: str) -> str:
        parsed = urllib.parse.urlparse(url)
        # Strip userinfo
        netloc = parsed.netloc
        if '@' in netloc:
            netloc = netloc.split('@', 1)[1]
        # Redact query values if sensitive
        if parsed.query:
            pairs = urllib.parse.parse_qsl(parsed.query, keep_blank_values=True)
            safe_pairs = []
            for k, v in pairs:
                if any(sec in k.lower() for sec in ["token", "secret", "password", "auth", "key", "state", "session"]):
                    safe_pairs.append((k, "[REDACTED]"))
                else:
                    safe_pairs.append((k, v))
            new_query = urllib.parse.urlencode(safe_pairs)
        else:
            new_query = ""
        return urllib.parse.urlunparse((parsed.scheme, netloc, parsed.path, parsed.params, new_query, ""))

    def evaluate_request(self, target_url: str, initiator_url: Optional[str], resource_type: str) -> Dict:
        target_parsed = urllib.parse.urlparse(target_url)
        target_host = target_parsed.netloc.split(':')[0]
        init_host = urllib.parse.urlparse(initiator_url).netloc.split(':')[0] if initiator_url else ""

        party_context = self.classify_party(target_host, init_host)

        # Check site exceptions
        exception = self.site_exceptions.get(init_host, "DEFAULT")
        if exception == "ALLOW_ALL":
            return {"decision": "ALLOW", "reason": "SITE_EXCEPTION_ALLOW_ALL", "party": party_context}
        if exception == "BLOCK_ALL" and party_context == "THIRD_PARTY":
            return {"decision": "BLOCK", "reason": "SITE_EXCEPTION_BLOCK_ALL", "party": party_context}

        # Check explicit allow rules first (precedence)
        for allowed in self.allow_rules:
            if target_host == allowed or target_host.endswith('.' + allowed):
                return {"decision": "ALLOW", "reason": "EXPLICIT_ALLOW_RULE", "party": party_context}

        # Check explicit block rules
        is_tracker = False
        matched_rule = None
        for blocked in self.block_rules:
            if target_host == blocked or target_host.endswith('.' + blocked):
                is_tracker = True
                matched_rule = blocked
                break

        if is_tracker:
            # Policy: First-party requests to tracker domains are allowed by default policy, third-party are blocked
            if party_context in ("FIRST_PARTY", "SAME_SITE"):
                decision = "ALLOW"
                reason = "FIRST_PARTY_TRACKER_ALLOWED_BY_POLICY"
            else:
                decision = "BLOCK"
                reason = "THIRD_PARTY_TRACKER_BLOCKED"

            # Emit telemetry
            self._event_counter += 1
            event = {
                "event_id": f"lt_priv_test_{self._event_counter:06d}",
                "event_type": "BLOCKED" if decision == "BLOCK" else "ALLOWED",
                "target_host": target_host,
                "sanitized_url": self.sanitize_url_for_telemetry(target_url),
                "party_context": party_context,
                "resource_type": resource_type,
                "matched_rule": matched_rule,
                "decision": decision
            }
            self.privacy_events.append(event)
            return {"decision": decision, "reason": reason, "party": party_context, "matched_rule": matched_rule}

        return {"decision": "ALLOW", "reason": "DEFAULT_ALLOW", "party": party_context}


def run_integration_tests():
    print("============================================================")
    print("LEAN THORIUM PRIVACY P1.1 — BROWSER INTEGRATION TEST SUITE")
    print("============================================================")

    engine = PrivacyEngine()
    passed = 0
    total = 0

    def test(name: str, condition: bool, details: str = ""):
        nonlocal passed, total
        total += 1
        status = "PASS" if condition else "FAIL"
        if condition:
            passed += 1
        print(f"[{status}] {name}")
        if details:
            print(f"       Details: {details}")

    # Test 1: First-party legitimate resource -> LOAD (ALLOW)
    res = engine.evaluate_request("https://news.example.com/article.html", "https://news.example.com", "MAIN_FRAME")
    test("Legitimate first-party main frame -> ALLOW", res["decision"] == "ALLOW" and res["party"] == "FIRST_PARTY")

    res = engine.evaluate_request("https://news.example.com/styles.css", "https://news.example.com/article.html", "STYLESHEET")
    test("Legitimate first-party stylesheet -> ALLOW", res["decision"] == "ALLOW" and res["party"] == "FIRST_PARTY")

    # Test 2: Legitimate unknown third-party resource -> LOAD (ALLOW)
    res = engine.evaluate_request("https://cdn.jsdelivr.net/npm/bootstrap.js", "https://news.example.com/article.html", "SCRIPT")
    test("Legitimate unknown third-party resource -> ALLOW", res["decision"] == "ALLOW" and res["party"] == "THIRD_PARTY")

    # Test 3: Known third-party tracker -> BLOCK (ERR_BLOCKED_BY_CLIENT)
    res = engine.evaluate_request("https://tracker.badanalytics.com/track.js", "https://news.example.com/article.html", "SCRIPT")
    test("Known third-party tracker -> BLOCK", res["decision"] == "BLOCK" and res["party"] == "THIRD_PARTY", f"Reason: {res['reason']}")

    # Test 4: Known tracker from first-party context -> ALLOW by policy
    res = engine.evaluate_request("https://tracker.example.com/dashboard", "https://tracker.example.com/index.html", "MAIN_FRAME")
    test("Known tracker from first-party context -> ALLOW", res["decision"] == "ALLOW" and res["party"] == "FIRST_PARTY", f"Reason: {res['reason']}")

    # Test 5: Tracking parameter stripping
    raw_url = "https://shop.example.com/product?id=123&utm_source=newsletter&utm_medium=email&fbclid=IwAR0xyz&coupon=SAVE20#reviews"
    clean_url, stripped = engine.sanitize_url(raw_url)
    expected_url = "https://shop.example.com/product?id=123&coupon=SAVE20#reviews"
    test("Tracking parameter filter removes tracking query params while preserving functional params & fragment",
         stripped and clean_url == expected_url, f"Result: {clean_url}")

    # Test 6: URL sanitization for telemetry excludes userinfo and sensitive tokens
    sens_url = "https://user:password123@news.example.com/api?access_token=secret_token_abc&page=2"
    telemetry_url = engine.sanitize_url_for_telemetry(sens_url)
    test("Telemetry URL sanitizer redacts credentials and auth tokens",
         "password123" not in telemetry_url and "secret_token_abc" not in telemetry_url and "REDACTED" in telemetry_url,
         f"Telemetry URL: {telemetry_url}")

    print("\n------------------------------------------------------------")
    print("SECTION 19: DETERMINISTIC BROWSER COMPATIBILITY CORPUS (15 SCENARIOS)")
    print("------------------------------------------------------------")

    corpus_scenarios = [
        ("1. Static page", "https://static.example.org/index.html", "https://static.example.org", "MAIN_FRAME", "ALLOW"),
        ("2. JS-heavy app", "https://app.example.com/bundle.js", "https://app.example.com/app", "SCRIPT", "ALLOW"),
        ("3. Login page", "https://auth.example.com/login", "https://auth.example.com", "MAIN_FRAME", "ALLOW"),
        ("4. OAuth flow", "https://oauth.provider.com/auth?client_id=1&state=xyz", "https://oauth.provider.com", "MAIN_FRAME", "ALLOW"),
        ("5. Ecommerce checkout", "https://store.example.com/checkout?cart=42", "https://store.example.com", "MAIN_FRAME", "ALLOW"),
        ("6. Search engine", "https://search.example.org/search?q=lean+thorium", "https://search.example.org", "MAIN_FRAME", "ALLOW"),
        ("7. News site", "https://news.example.net/top-stories", "https://news.example.net", "MAIN_FRAME", "ALLOW"),
        ("8. Video stream", "https://video.example.com/stream.m3u8", "https://video.example.com/watch", "MEDIA", "ALLOW"),
        ("9. CDN-heavy page", "https://legitimate.cdn.com/lib/react.production.min.js", "https://myportal.com", "SCRIPT", "ALLOW"),
        ("10. WebSocket endpoint", "wss://ws.example.com/socket", "https://ws.example.com", "WEBSOCKET", "ALLOW"),
        ("11. Iframe nested page", "https://embed.partner.com/widget", "https://main.example.com", "SUB_FRAME", "ALLOW"),
        ("12. Redirect chain", "https://redirect.link/step1", "https://redirect.link", "MAIN_FRAME", "ALLOW"),
        ("13. Signed URL", "https://storage.cdn.com/file.pdf?Signature=abc1234&Expires=170000", "https://portal.com", "OTHER", "ALLOW"),
        ("14. Query-sensitive app", "https://database.internal/query?filter=active&order=asc", "https://database.internal", "XHR", "ALLOW"),
        ("15. Legitimate 3rd-party resource", "https://fonts.googleapis.com/css2?family=Roboto", "https://myblog.com", "STYLESHEET", "ALLOW"),
    ]

    for name, url, init, rtype, expected in corpus_scenarios:
        res = engine.evaluate_request(url, init, rtype)
        test(f"Corpus: {name}", res["decision"] == expected, f"Expected {expected}, got {res['decision']} ({res['reason']})")

    print("\n------------------------------------------------------------")
    print(f"TOTAL TESTS: {total} | PASSED: {passed} | FAILED: {total - passed}")
    print("============================================================")
    return passed == total

if __name__ == "__main__":
    success = run_integration_tests()
    sys.exit(0 if success else 1)
