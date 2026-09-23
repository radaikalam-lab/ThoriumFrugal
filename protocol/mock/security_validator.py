"""
Security Validator and Sanitizer for Thorium-Cognitia Integration.
Enforces Invariants S1-S16, URL redaction, credential scrubbing, incognito isolation, and prompt injection defense.
"""

from __future__ import annotations
from typing import Dict, Any, List, Optional, Tuple
from urllib.parse import urlparse, urlunparse, parse_qsl, urlencode
import re
from .protocol_types import MessageEnvelope, MessageType


class SecurityViolationError(Exception):
    def __init__(self, invariant_id: str, message: str):
        super().__init__(f"[{invariant_id}] {message}")
        self.invariant_id = invariant_id
        self.message = message


class SecurityValidator:
    """
    Validates and sanitizes Thorium-Cognitia messages against security invariants S1-S16.
    """

    SENSITIVE_QUERY_PARAMS = {
        "token", "auth", "password", "secret", "session", "sig", "key",
        "access_token", "api_key", "passwd", "pwd", "client_secret",
        "refresh_token", "credential", "private_key", "bearer", "nonce", "csrf"
    }

    FORBIDDEN_HEADERS = {
        "authorization", "cookie", "set-cookie", "proxy-authorization"
    }

    PROMPT_INJECTION_PATTERNS = [
        r"ignore\s+(all\s+)?previous\s+instructions",
        r"system\s*:\s*you\s+are",
        r"override\s+system\s+prompt",
        r"you\s+must\s+now\s+execute",
        r"download\s+this\s+file\s+immediately",
        r"exfiltrate",
    ]

    def __init__(self, max_payload_bytes: int = 1048576):  # 1MB limit
        self.max_payload_bytes = max_payload_bytes
        self.compiled_injection_regexes = [
            re.compile(p, re.IGNORECASE) for p in self.PROMPT_INJECTION_PATTERNS
        ]

    def sanitize_url(self, raw_url: str) -> str:
        """
        Sanitizes a URL by:
        1. Redacting user:password authentication components.
        2. Redacting sensitive query parameters.
        """
        if not raw_url:
            return ""

        try:
            parsed = urlparse(raw_url)
        except Exception:
            return "[MALFORMED_URL]"

        # Redact userinfo
        netloc = parsed.netloc
        if "@" in netloc:
            userinfo, host = netloc.split("@", 1)
            if ":" in userinfo:
                user, _ = userinfo.split(":", 1)
                netloc = f"{user}:***@{host}"
            else:
                netloc = f"***@{host}"

        # Redact sensitive query parameters
        if parsed.query:
            query_pairs = parse_qsl(parsed.query, keep_blank_values=True)
            sanitized_pairs = []
            for k, v in query_pairs:
                if k.lower() in self.SENSITIVE_QUERY_PARAMS:
                    sanitized_pairs.append((k, "[REDACTED]"))
                else:
                    sanitized_pairs.append((k, v))
            new_query = urlencode(sanitized_pairs)
        else:
            new_query = ""

        sanitized = urlunparse((
            parsed.scheme,
            netloc,
            parsed.path,
            parsed.params,
            new_query,
            parsed.fragment
        ))
        return sanitized

    def sanitize_headers(self, headers: Dict[str, str]) -> Dict[str, str]:
        """
        Strips forbidden credentials/cookies from header dictionaries.
        """
        clean_headers = {}
        for k, v in headers.items():
            if k.lower() in self.FORBIDDEN_HEADERS:
                clean_headers[k] = "[REDACTED_HEADER]"
            else:
                clean_headers[k] = v
        return clean_headers

    def validate_message_security(self, envelope: MessageEnvelope) -> Tuple[bool, Optional[str]]:
        """
        Validates an envelope against security invariants S1-S16.
        Raises SecurityViolationError if a violation occurs.
        """
        # S10: Bounded payload size check
        serialized_len = len(envelope.to_json().encode('utf-8'))
        if serialized_len > self.max_payload_bytes:
            raise SecurityViolationError(
                "S10_PAYLOAD_LIMIT",
                f"Message payload size {serialized_len} bytes exceeds max budget {self.max_payload_bytes} bytes."
            )

        # S3, S4, S5, S6: Check for raw credential/cookie leakage in payload
        self._check_credential_leakage(envelope.payload)

        # S7, S8, S16: Proposal authority validation
        if envelope.message_type == MessageType.CANDIDATE_ACTION_PROPOSAL:
            auth = envelope.payload.get("authority")
            if auth != "NONE":
                raise SecurityViolationError(
                    "S7_AUTHORITY_VIOLATION",
                    f"CandidateActionProposal MUST have authority='NONE'. Found authority='{auth}'."
                )

        # S14, S16: Execution authorization observation validation
        if envelope.message_type == MessageType.EXECUTION_AUTHORIZATION_OBSERVED:
            cognitia_auth = envelope.payload.get("cognitia_authority")
            if cognitia_auth != "NONE":
                raise SecurityViolationError(
                    "S16_EXECUTION_AUTHORITY_VIOLATION",
                    f"Execution authorization must record Cognitia authority as 'NONE', got '{cognitia_auth}'."
                )
            decision_source = envelope.payload.get("decision_source")
            if decision_source == "COGNITIA":
                raise SecurityViolationError(
                    "S7_DIRECT_EXECUTION_FORBIDDEN",
                    "Cognitia cannot be the decision_source for execution authorization."
                )

        # S1, S2: Observation epistemic status and prompt injection boundary
        if envelope.message_type == MessageType.OBSERVATION:
            epistemic_status = envelope.payload.get("epistemic_status", "UNRESOLVED")
            source_type = envelope.payload.get("source_type", "SENSOR")

            if source_type != "SENSOR" or epistemic_status != "UNRESOLVED":
                raise SecurityViolationError(
                    "S1_UNTRUSTED_CONTENT_STATUS",
                    f"Observation web content must have source_type='SENSOR' and epistemic_status='UNRESOLVED'. Got ({source_type}, {epistemic_status})."
                )

            # S12: Incognito privacy constraints
            ctx = envelope.payload.get("context", {})
            if ctx.get("is_incognito") is True:
                if "persistent_user_id" in ctx or "sync_account" in ctx:
                    raise SecurityViolationError(
                        "S12_INCOGNITO_PRIVACY_LEAK",
                        "Incognito observation context must not include persistent profile identifiers."
                    )

        return True, None

    def _check_credential_leakage(self, data: Any) -> None:
        """Recursively checks dictionaries/lists for unredacted raw password/auth tokens."""
        if isinstance(data, dict):
            for k, v in data.items():
                k_lower = k.lower()
                if k_lower in ("password", "passwd", "client_secret", "private_key", "cookie", "set-cookie", "authorization"):
                    if isinstance(v, str) and v and v != "[REDACTED]" and v != "[REDACTED_HEADER]":
                        raise SecurityViolationError(
                            "S3_CREDENTIAL_LEAKAGE",
                            f"Unredacted sensitive field '{k}' detected in message payload."
                        )
                elif k_lower == "url" and isinstance(v, str):
                    if "@" in v:
                        parsed = urlparse(v)
                        if parsed.password:
                            raise SecurityViolationError(
                                "S3_URL_CREDENTIAL_LEAKAGE",
                                f"Unredacted userinfo password in URL: '{v}'."
                            )
                self._check_credential_leakage(v)
        elif isinstance(data, list):
            for item in data:
                self._check_credential_leakage(item)

    def detect_prompt_injection_in_observation(self, text_content: str) -> bool:
        """
        Detects prompt injection signatures in untrusted web observations.
        Does NOT crash the collector, but marks the extracted text as quarantined/tainted.
        """
        if not text_content:
            return False
        for pattern in self.compiled_injection_regexes:
            if pattern.search(text_content):
                return True
        return False
