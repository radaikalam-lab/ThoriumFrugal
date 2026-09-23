"""
Protocol Validator for Thorium-Cognitia Protocol v1.
Validates envelope conformance, monotonic sequence numbers, session boundaries, and causal integrity.
"""

from __future__ import annotations
from typing import Dict, Any, Optional, Set, Tuple
import uuid
import datetime
from .protocol_types import (
    MessageType,
    MessageEnvelope,
    ExecutionStatus,
    RiskClassification,
)


class ProtocolValidationError(Exception):
    def __init__(self, code: str, message: str):
        super().__init__(f"[{code}] {message}")
        self.code = code
        self.message = message


class ProtocolValidator:
    """
    Validates Thorium <-> Cognitia IPC messages against the Protocol v1 specification.
    Maintains session state, sequence tracking, and message deduplication.
    """

    def __init__(self):
        self.active_session_id: Optional[str] = None
        self.last_sequence_number: int = 0
        self.seen_message_ids: Set[str] = set()
        self.session_established: bool = False
        self.seen_sequence_numbers: Set[int] = set()

    def reset_session(self, session_id: Optional[str] = None) -> None:
        """Reset state when a new session begins or after disconnect."""
        self.active_session_id = session_id
        self.last_sequence_number = 0
        self.seen_message_ids.clear()
        self.seen_sequence_numbers.clear()
        self.session_established = (session_id is not None)

    def validate_envelope(self, envelope: MessageEnvelope) -> Tuple[bool, Optional[str]]:
        """
        Validate envelope structure, version, monotonic sequencing, and session integrity.
        Raises ProtocolValidationError on critical violation or returns (True, None).
        """
        # 1. Major version check (fail-closed on major version mismatch)
        if not envelope.protocol_version.startswith("1."):
            raise ProtocolValidationError(
                "ERR_VERSION_MISMATCH",
                f"Unsupported protocol major version: '{envelope.protocol_version}'. Expected '1.x'."
            )

        # 2. Message ID format and uniqueness
        try:
            val_uuid = uuid.UUID(envelope.message_id)
        except Exception:
            raise ProtocolValidationError(
                "ERR_MALFORMED_MESSAGE_ID",
                f"Message ID '{envelope.message_id}' is not a valid UUID."
            )

        if envelope.message_id in self.seen_message_ids:
            raise ProtocolValidationError(
                "ERR_DUPLICATE_MESSAGE_ID",
                f"Duplicate message ID observed: '{envelope.message_id}'."
            )

        # 3. Session lifecycle & handshake validation
        if envelope.message_type == MessageType.SESSION_HELLO:
            # Session Hello establishes a new handshake
            if self.session_established and self.active_session_id != envelope.session_id:
                # Re-handshake or session restart
                self.reset_session(envelope.session_id)
            elif not self.session_established:
                self.active_session_id = envelope.session_id

        elif envelope.message_type == MessageType.SESSION_ACCEPT:
            self.active_session_id = envelope.session_id
            self.session_established = True

        elif envelope.message_type == MessageType.SESSION_CLOSE:
            # Session termination
            if self.active_session_id and envelope.session_id != self.active_session_id:
                raise ProtocolValidationError(
                    "ERR_SESSION_MISMATCH",
                    f"Session close for unknown session '{envelope.session_id}', active is '{self.active_session_id}'."
                )

        else:
            # Regular messages require established session
            if not self.session_established and envelope.session_id != self.active_session_id:
                raise ProtocolValidationError(
                    "ERR_NO_ACTIVE_SESSION",
                    f"Message type '{envelope.message_type}' received before session established."
                )
            if self.active_session_id and envelope.session_id != self.active_session_id:
                raise ProtocolValidationError(
                    "ERR_STALE_SESSION",
                    f"Session mismatch: message session '{envelope.session_id}' != active session '{self.active_session_id}'."
                )

        # 4. Sequence number monotonicity and gap detection
        seq = envelope.sequence_number
        if envelope.message_type != MessageType.SESSION_ACCEPT:
            if seq in self.seen_sequence_numbers:
                raise ProtocolValidationError(
                    "ERR_DUPLICATE_SEQUENCE",
                    f"Duplicate sequence number {seq} in session '{envelope.session_id}'."
                )

        if envelope.message_type not in (MessageType.SESSION_HELLO, MessageType.SESSION_ACCEPT):
            if seq != self.last_sequence_number + 1:
                if seq <= self.last_sequence_number:
                    raise ProtocolValidationError(
                        "ERR_OUT_OF_ORDER_SEQUENCE",
                        f"Out-of-order sequence number {seq} (last was {self.last_sequence_number})."
                    )
                else:
                    raise ProtocolValidationError(
                        "ERR_SEQUENCE_GAP",
                        f"Sequence gap detected: received {seq}, expected {self.last_sequence_number + 1}."
                    )

        # 5. Timestamp format (ISO 8601 UTC)
        try:
            # Replace Z with +00:00 for fromisoformat compatibility
            ts_clean = envelope.timestamp.replace("Z", "+00:00")
            datetime.datetime.fromisoformat(ts_clean)
        except Exception:
            raise ProtocolValidationError(
                "ERR_INVALID_TIMESTAMP",
                f"Timestamp '{envelope.timestamp}' is not valid ISO 8601 UTC."
            )

        # 6. Payload validation based on message type
        self._validate_payload(envelope.message_type, envelope.payload)

        # Record message ID and sequence
        self.seen_message_ids.add(envelope.message_id)
        if envelope.message_type != MessageType.SESSION_ACCEPT:
            self.seen_sequence_numbers.add(seq)
            self.last_sequence_number = seq

        if envelope.message_type == MessageType.SESSION_CLOSE:
            self.session_established = False

        return True, None

    def _validate_payload(self, msg_type: MessageType, payload: Dict[str, Any]) -> None:
        if not isinstance(payload, dict):
            raise ProtocolValidationError("ERR_MALFORMED_PAYLOAD", "Payload must be a JSON object (dict).")

        if msg_type == MessageType.OBSERVATION:
            if "observation_type" not in payload:
                raise ProtocolValidationError("ERR_PAYLOAD_MISSING_FIELD", "Observation payload missing 'observation_type'.")
            if "context" not in payload:
                raise ProtocolValidationError("ERR_PAYLOAD_MISSING_FIELD", "Observation payload missing 'context'.")
            ctx = payload["context"]
            if not isinstance(ctx, dict) or "tab_id" not in ctx or "url" not in ctx:
                raise ProtocolValidationError("ERR_PAYLOAD_MISSING_FIELD", "Observation context missing tab_id or url.")

        elif msg_type == MessageType.CANDIDATE_ACTION_PROPOSAL:
            if "proposal_id" not in payload:
                raise ProtocolValidationError("ERR_PAYLOAD_MISSING_FIELD", "Proposal missing 'proposal_id'.")
            if payload.get("authority") != "NONE":
                raise ProtocolValidationError("ERR_AUTHORITY_VIOLATION", f"Proposal authority must be 'NONE', got '{payload.get('authority')}'.")

        elif msg_type == MessageType.EXECUTION_AUTHORIZATION_OBSERVED:
            if "proposal_id" not in payload or "decision_source" not in payload:
                raise ProtocolValidationError("ERR_PAYLOAD_MISSING_FIELD", "Execution authorization missing required fields.")
            if payload.get("cognitia_authority") != "NONE":
                raise ProtocolValidationError("ERR_AUTHORITY_VIOLATION", "Cognitia authority must be 'NONE'.")
            if payload.get("decision_source") == "COGNITIA":
                raise ProtocolValidationError("ERR_AUTHORITY_VIOLATION", "Decision source cannot be COGNITIA.")

        elif msg_type == MessageType.EXECUTION_RESULT_OBSERVED:
            if "proposal_id" not in payload or "status" not in payload:
                raise ProtocolValidationError("ERR_PAYLOAD_MISSING_FIELD", "Execution result missing proposal_id or status.")
            valid_statuses = [e.value for e in ExecutionStatus]
            if payload["status"] not in valid_statuses:
                raise ProtocolValidationError("ERR_INVALID_STATUS", f"Unknown execution status: {payload['status']}")

        elif msg_type == MessageType.DIRECTIONAL_SPEC:
            req_fields = ["spec_id", "domain_id", "desired_direction"]
            for rf in req_fields:
                if rf not in payload:
                    raise ProtocolValidationError("ERR_PAYLOAD_MISSING_FIELD", f"DirectionalSpec missing '{rf}'.")
