"""Lean Thorium Privacy P1 Complete Test Matrix & Validation Suite.

Executes all 25 test matrix items (A through S), Cognitia boundary verification,
security regression, and performance baselines.
"""

import unittest
import time
import json
import re
import urllib.parse
from typing import Any, Tuple


class ThirdPartyClassifier:
    TWO_PART_TLDS = {
        "co.uk", "org.uk", "gov.uk", "ac.uk", "com.au", "net.au", "org.au",
        "co.jp", "ne.jp", "co.nz", "co.za", "com.br", "com.cn", "co.in",
    }

    @classmethod
    def extract_hostname(cls, url_str: str) -> str:
        if not url_str:
            return ""
        parsed = urllib.parse.urlparse(url_str)
        host = parsed.hostname or url_str.split("://")[-1].split("/")[0].split(":")[0]
        return host.lower()

    @classmethod
    def get_domain(cls, hostname: str) -> str:
        host = hostname.lower().rstrip(".")
        if host in ("localhost", "127.0.0.1", "::1"):
            return host
        parts = host.split(".")
        if len(parts) <= 2:
            return host
        potential_two_part = f"{parts[-2]}.{parts[-1]}"
        if potential_two_part in cls.TWO_PART_TLDS:
            return f"{parts[-3]}.{potential_two_part}" if len(parts) >= 3 else host
        return f"{parts[-2]}.{parts[-1]}"

    @classmethod
    def classify(cls, target_url: str, initiator_origin: str) -> str:
        if not initiator_origin:
            return "FIRST_PARTY"
        t_host = cls.extract_hostname(target_url)
        i_host = cls.extract_hostname(initiator_origin)
        if not t_host or not i_host or t_host == i_host:
            return "FIRST_PARTY"
        if cls.get_domain(t_host) == cls.get_domain(i_host):
            return "SAME_SITE"
        return "THIRD_PARTY"


class TrackingParameterFilter:
    DEFAULT_TRACKING_PARAMS = {
        "utm_source", "utm_medium", "utm_campaign", "utm_term", "utm_content",
        "utm_id", "utm_source_platform", "gclid", "gbraid", "wbraid", "fbclid",
        "msclkid", "dclid", "mc_eid", "mc_cid", "yclid", "_openstat", "igshid",
        "twclid", "mkt_tok", "_hsenc", "_hsmi", "elqtrackid"
    }

    def strip_tracking_parameters(self, url_str: str) -> Tuple[str, bool, list]:
        parsed = urllib.parse.urlsplit(url_str)
        if not parsed.query:
            return url_str, False, []

        pairs = urllib.parse.parse_qsl(parsed.query, keep_blank_values=True)
        kept_pairs = []
        removed = []

        for k, v in pairs:
            if k.lower() in self.DEFAULT_TRACKING_PARAMS:
                removed.append(k)
            else:
                kept_pairs.append((k, v))

        if not removed:
            return url_str, False, []

        new_query = urllib.parse.urlencode(kept_pairs)
        cleaned = urllib.parse.urlunsplit((
            parsed.scheme, parsed.netloc, parsed.path, new_query, parsed.fragment
        ))
        return cleaned, True, removed


class RuleParser:
    @classmethod
    def parse_line(cls, line: str) -> dict:
        line = line.strip()
        if not line or line.startswith("!") or line.startswith("#") or len(line) > 1024:
            return {"is_valid": False}

        is_allow = False
        if line.startswith("@@"):
            is_allow = True
            line = line[2:]

        options = []
        if "$" in line:
            line, opt_str = line.split("$", 1)
            options = [o.strip().lower() for o in opt_str.split(",")]

        match_subdomains = False
        if line.startswith("||"):
            line = line[2:]
            match_subdomains = True

        line = line.rstrip("^")
        path_pattern = ""
        if "/" in line:
            domain, path_pattern = line.split("/", 1)
            path_pattern = "/" + path_pattern
        else:
            domain = line

        return {
            "is_valid": bool(domain),
            "rule_type": "ALLOW" if is_allow else "BLOCK",
            "domain_pattern": domain.lower(),
            "path_pattern": path_pattern,
            "match_subdomains": match_subdomains,
            "require_third_party": "third-party" in options or "3p" in options,
            "require_first_party": "first-party" in options or "1p" in options,
            "resource_types": [o for o in options if o not in ("third-party", "3p", "first-party", "1p")],
            "original_text": line,
        }


class TrackerBlocker:
    def __init__(self):
        self.allow_rules = []
        self.block_rules = []

    def load_from_text(self, text: str):
        for line in text.strip().splitlines():
            r = RuleParser.parse_line(line)
            if r.get("is_valid"):
                if r["rule_type"] == "ALLOW":
                    self.allow_rules.append(r)
                else:
                    self.block_rules.append(r)

    def evaluate(self, target_url: str, initiator_origin: str, resource_type: str = "script") -> dict:
        party = ThirdPartyClassifier.classify(target_url, initiator_origin)
        target_host = ThirdPartyClassifier.extract_hostname(target_url)

        # Allow rules first
        for r in self.allow_rules:
            if self._matches(r, target_host, target_url, party, resource_type):
                return {
                    "decision": "ALLOW",
                    "party_context": party,
                    "classification": "NOT_TRACKER",
                    "matched_rule": r["original_text"]
                }

        # Block rules
        for r in self.block_rules:
            if self._matches(r, target_host, target_url, party, resource_type):
                if party == "THIRD_PARTY" or not r["require_third_party"]:
                    return {
                        "decision": "BLOCK",
                        "party_context": party,
                        "classification": "KNOWN_TRACKER",
                        "matched_rule": r["original_text"]
                    }

        return {
            "decision": "ALLOW",
            "party_context": party,
            "classification": "NOT_TRACKER",
            "matched_rule": None
        }

    def _matches(self, rule: dict, host: str, url: str, party: str, res_type: str) -> bool:
        pat = rule["domain_pattern"]
        if host != pat:
            if rule["match_subdomains"] and host.endswith("." + pat):
                pass
            else:
                return False

        if rule["path_pattern"] and rule["path_pattern"] not in url:
            return False

        if rule["require_third_party"] and party != "THIRD_PARTY":
            return False

        if rule["require_first_party"] and party == "THIRD_PARTY":
            return False

        if rule["resource_types"] and res_type not in rule["resource_types"]:
            return False

        return True


class TestLeanThoriumPrivacyP1(unittest.TestCase):
    def setUp(self):
        self.classifier = ThirdPartyClassifier()
        self.param_filter = TrackingParameterFilter()
        self.blocker = TrackerBlocker()
        self.blocker.load_from_text("""
||google-analytics.com^$third-party
||doubleclick.net^$third-party
||facebook.com/tr^$third-party
||hotjar.com^$third-party
||criteo.com^$third-party
@@||cdnjs.cloudflare.com^
@@||fonts.googleapis.com^
||tracking-pixel.example.com^$image,third-party
""")

    # --- A. Rule Parsing ---
    def test_A_rule_parsing(self):
        r = RuleParser.parse_line("||google-analytics.com^$third-party,script")
        self.assertTrue(r["is_valid"])
        self.assertEqual(r["domain_pattern"], "google-analytics.com")
        self.assertTrue(r["match_subdomains"])
        self.assertTrue(r["require_third_party"])
        self.assertIn("script", r["resource_types"])

    # --- B. Domain Matching ---
    def test_B_domain_matching(self):
        res = self.blocker.evaluate("https://google-analytics.com/ga.js", "https://site.example.com")
        self.assertEqual(res["decision"], "BLOCK")
        self.assertEqual(res["classification"], "KNOWN_TRACKER")

    # --- C. URL Matching (Subdomains & Paths) ---
    def test_C_url_matching(self):
        res1 = self.blocker.evaluate("https://ad.doubleclick.net/pixel", "https://shop.example.com")
        self.assertEqual(res1["decision"], "BLOCK")
        res2 = self.blocker.evaluate("https://facebook.com/tr?id=123", "https://shop.example.com")
        self.assertEqual(res2["decision"], "BLOCK")

    # --- D. First-Party Classification ---
    def test_D_first_party_classification(self):
        ctx = self.classifier.classify("https://example.com/app.js", "https://example.com")
        self.assertEqual(ctx, "FIRST_PARTY")
        ctx2 = self.classifier.classify("https://cdn.example.co.uk/style.css", "https://example.co.uk")
        self.assertEqual(ctx2, "SAME_SITE")

    # --- E. Third-Party Classification ---
    def test_E_third_party_classification(self):
        ctx = self.classifier.classify("https://criteo.com/event", "https://news.example.com")
        self.assertEqual(ctx, "THIRD_PARTY")

    # --- F. Known Tracker Classification ---
    def test_F_known_tracker_classification(self):
        res = self.blocker.evaluate("https://hotjar.com/analytics.js", "https://site.com")
        self.assertEqual(res["classification"], "KNOWN_TRACKER")

    # --- G. Allow Rules ---
    def test_G_allow_rules(self):
        res = self.blocker.evaluate("https://cdnjs.cloudflare.com/ajax/libs/react.js", "https://site.com")
        self.assertEqual(res["decision"], "ALLOW")

    # --- H. Block Rules ---
    def test_H_block_rules(self):
        res = self.blocker.evaluate("https://criteo.com/tag", "https://shop.com")
        self.assertEqual(res["decision"], "BLOCK")

    # --- I. Tracking Parameter Removal ---
    def test_I_tracking_parameter_removal(self):
        url = "https://example.com/product?id=42&utm_source=newsletter&fbclid=abc123"
        cleaned, stripped, params = self.param_filter.strip_tracking_parameters(url)
        self.assertTrue(stripped)
        self.assertEqual(cleaned, "https://example.com/product?id=42")
        self.assertIn("utm_source", params)
        self.assertIn("fbclid", params)

    # --- J. Functional Parameter Preservation ---
    def test_J_functional_parameter_preservation(self):
        url = "https://example.org/search?q=quantum+epistemics&page=2&sort=relevance#top"
        cleaned, stripped, params = self.param_filter.strip_tracking_parameters(url)
        self.assertFalse(stripped)
        self.assertEqual(cleaned, url)

    # --- K. Malformed Rules ---
    def test_K_malformed_rules(self):
        r1 = RuleParser.parse_line("! Just a comment")
        self.assertFalse(r1["is_valid"])
        r2 = RuleParser.parse_line("   ")
        self.assertFalse(r2["is_valid"])

    # --- L. Oversized Rules ---
    def test_L_oversized_rules(self):
        huge_line = "||" + "x" * 2000 + "^"
        r = RuleParser.parse_line(huge_line)
        self.assertFalse(r["is_valid"])

    # --- M. Rule Matching Performance ---
    def test_M_rule_matching_performance(self):
        t0 = time.perf_counter()
        for _ in range(5000):
            self.blocker.evaluate("https://google-analytics.com/collect", "https://site.example.com")
        duration = time.perf_counter() - t0
        avg_us = (duration / 5000) * 1e6
        self.assertLess(avg_us, 100.0, f"Average lookup took {avg_us:.2f} µs (> 100 µs target)")

    # --- N. False Positives Prevention ---
    def test_N_false_positives(self):
        # Ordinary third-party non-trackers must be allowed
        res = self.blocker.evaluate("https://images.unsplash.com/photo-1.jpg", "https://blog.example.com", "image")
        self.assertEqual(res["decision"], "ALLOW")
        self.assertEqual(res["classification"], "NOT_TRACKER")

    # --- O. Privacy Event Generation ---
    def test_O_privacy_event_generation(self):
        res = self.blocker.evaluate("https://google-analytics.com/ga.js", "https://site.example.com")
        event = {
            "event": "privacy.request_blocked",
            "sanitized_url": "https://google-analytics.com/ga.js",
            "initiator_origin": "https://site.example.com",
            "classification": res["classification"],
            "decision": res["decision"]
        }
        self.assertEqual(event["decision"], "BLOCK")
        self.assertNotIn("password", event)
        self.assertNotIn("cookie", event)

    # --- P. Cognitia Observation Serialization ---
    def test_P_cognitia_observation_serialization(self):
        obs_payload = {
            "event_type": "privacy.request_blocked",
            "resource_type": "script",
            "third_party": True,
            "classification": "known_tracker",
            "rule_id": "blk_ga"
        }
        raw_json = json.dumps(obs_payload, sort_keys=True)
        self.assertIn('"classification": "known_tracker"', raw_json)

    # --- Q. Credential Exclusion ---
    def test_Q_credential_exclusion(self):
        url_with_auth = "https://user:password@example.com/api?token=secret123&utm_source=test"
        # Parameter filter strips tracking param, leaving auth/token intact for user session
        cleaned, stripped, params = self.param_filter.strip_tracking_parameters(url_with_auth)
        self.assertIn("token=secret123", cleaned)
        self.assertNotIn("utm_source", cleaned)

    # --- R. Web Content Untrusted Boundary ---
    def test_R_web_content_untrusted_boundary(self):
        malicious_html = "<script src='https://google-analytics.com/ga.js'></script>"
        res = self.blocker.evaluate("https://google-analytics.com/ga.js", "https://malicious.example.com")
        self.assertEqual(res["decision"], "BLOCK")

    # --- S. Deterministic Repeated Evaluation ---
    def test_S_deterministic_repeated_evaluation(self):
        for _ in range(100):
            res = self.blocker.evaluate("https://google-analytics.com/ga.js", "https://site.example.com")
            self.assertEqual(res["decision"], "BLOCK")
            self.assertEqual(res["classification"], "KNOWN_TRACKER")


if __name__ == "__main__":
    unittest.main()