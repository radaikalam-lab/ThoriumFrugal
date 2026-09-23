"""
Mock Cognitia Daemon.
Simulates Cognitia daemon endpoint, epistemic ingestion, advisory proposal generation,
and external execution observation reconciliation. Zero browser execution authority.
"""

from __future__ import annotations
from typing import Dict, Any, List, Optional, Tuple
import uuid
import datetime
from .protocol_types import (
    MessageType,
    MessageEnvelope,
    SourceIdentity,
    ExecutionStatus,
    RiskClassification,
    CandidateActionProposalData,
    SessionAcceptData,
    SessionRejectData,
)
from .protocol_validator import ProtocolValidator, ProtocolValidationError
from .security_validator import SecurityValidator, SecurityViolationError


class MockCognitiaDaemon:
    """
    Simulates the Cognitia IPC Daemon endpoint.
    Ingests passive observations, validates protocol & security, reasons over directional specs,
    produces advisory proposals (authority=NONE), and reconciles external execution results.
    """

    def __init__(self, expected_token: str = "mock_secret_token"):
        self.expected_token = expected_token
        self.active_session_id: Optional[str] = None
        self.protocol_validator = ProtocolValidator()
        self.security_validator = SecurityValidator()

        # Epistemic / Adaptive stores
        self.observations_store: List[Dict[str, Any]] = []
        self.proposals_store: Dict[str, Dict[str, Any]] = {}
        self.authorizations_store: Dict[str, Dict[str, Any]] = {}
        self.execution_results_store: Dict[str, Dict[str, Any]] = {}
        self.audit_log: List[Dict[str, Any]] = []

        self.source = SourceIdentity(
            application="cognitia",
            adapter="daemon_ipc",
            component="governance_and_inference",
            instance_id="cognitia_daemon_proc",
        )
        self.sequence_number = 0

    def process_incoming_envelope(self, envelope: MessageEnvelope) -> Tuple[bool, Optional[MessageEnvelope]]:
        """
        Processes an inbound message from Thorium or an external client.
        Returns (success, response_envelope_if_any).
        """
        # 1. Protocol Validation
        self.protocol_validator.validate_envelope(envelope)

        # 2. Security Validation
        self.security_validator.validate_message_security(envelope)

        # 3. Append to immutable audit log
        self.audit_log.append({
            "timestamp": datetime.datetime.now(datetime.timezone.utc).isoformat(),
            "direction": "INBOUND",
            "envelope": envelope.to_dict(),
        })

        # 4. Handle message type
        if envelope.message_type == MessageType.SESSION_HELLO:
            return self._handle_session_hello(envelope)

        elif envelope.message_type == MessageType.OBSERVATION:
            self._handle_observation(envelope)
            return True, None

        elif envelope.message_type == MessageType.EXECUTION_AUTHORIZATION_OBSERVED:
            self._handle_execution_auth_observed(envelope)
            return True, None

        elif envelope.message_type == MessageType.EXECUTION_RESULT_OBSERVED:
            self._handle_execution_result_observed(envelope)
            return True, None

        elif envelope.message_type == MessageType.SESSION_CLOSE:
            self.active_session_id = None
            return True, None

        return True, None

    def _handle_session_hello(self, envelope: MessageEnvelope) -> Tuple[bool, MessageEnvelope]:
        auth_token = envelope.payload.get("auth_token", "")
        self.sequence_number = 1

        if auth_token != self.expected_token:
            reject_payload = SessionRejectData(
                reason_code="AUTH_FAILED",
                message="Invalid or missing peer authentication token."
            ).to_dict()
            resp = MessageEnvelope(
                protocol_version="1.0",
                message_type=MessageType.SESSION_REJECT,
                message_id=str(uuid.uuid4()),
                session_id=envelope.session_id,
                sequence_number=self.sequence_number,
                timestamp=datetime.datetime.now(datetime.timezone.utc).isoformat(),
                source=self.source,
                payload=reject_payload,
                correlation_id=envelope.message_id,
                causation_id=envelope.message_id,
            )
            return False, resp

        self.active_session_id = envelope.session_id
        accept_payload = SessionAcceptData(
            session_id=envelope.session_id,
            negotiated_version="1.0.0",
            negotiated_capabilities=["OBSERVATION", "SANITIZATION_V1", "ADVISORY_PROPOSALS"],
            heartbeat_interval_ms=5000,
            server_nonce=str(uuid.uuid4()),
        ).to_dict()

        resp = MessageEnvelope(
            protocol_version="1.0",
            message_type=MessageType.SESSION_ACCEPT,
            message_id=str(uuid.uuid4()),
            session_id=envelope.session_id,
            sequence_number=self.sequence_number,
            timestamp=datetime.datetime.now(datetime.timezone.utc).isoformat(),
            source=self.source,
            payload=accept_payload,
            correlation_id=envelope.message_id,
            causation_id=envelope.message_id,
        )
        return True, resp

    def _handle_observation(self, envelope: MessageEnvelope) -> None:
        """Stores passive observation with epistemic UNRESOLVED status."""
        self.observations_store.append(envelope.to_dict())

    def _handle_execution_auth_observed(self, envelope: MessageEnvelope) -> None:
        prop_id = envelope.payload["proposal_id"]
        self.authorizations_store[prop_id] = envelope.payload

    def _handle_execution_result_observed(self, envelope: MessageEnvelope) -> None:
        prop_id = envelope.payload["proposal_id"]
        self.execution_results_store[prop_id] = envelope.payload

    def generate_candidate_proposal(
        self,
        domain_id: str,
        intent: str,
        target: str,
        candidate_action: Dict[str, Any],
        rationale: str,
        causation_message_id: Optional[str] = None
    ) -> MessageEnvelope:
        """
        Generates an advisory CandidateActionProposal.
        STRICTLY ENFORCES authority = 'NONE'.
        """
        if not self.active_session_id:
            raise RuntimeError("Cannot generate proposal without an active session.")

        prop_data = CandidateActionProposalData(
            proposal_id=str(uuid.uuid4()),
            source_model="cognitia_laya_surrogate_v1",
            domain_id=domain_id,
            intent=intent,
            target=target,
            candidate_action=candidate_action,
            rationale=rationale,
            constraints=["USER_CONFIRMATION_REQUIRED", "DOMAIN_MATCH"],
            risk_classification=RiskClassification.LOW,
            created_at=datetime.datetime.now(datetime.timezone.utc).isoformat(),
            authority="NONE",  # MANDATORY INVARIANT
        )

        self.sequence_number += 1
        envelope = MessageEnvelope(
            protocol_version="1.0",
            message_type=MessageType.CANDIDATE_ACTION_PROPOSAL,
            message_id=str(uuid.uuid4()),
            session_id=self.active_session_id,
            sequence_number=self.sequence_number,
            timestamp=datetime.datetime.now(datetime.timezone.utc).isoformat(),
            source=self.source,
            payload=prop_data.to_dict(),
            causation_id=causation_message_id,
            correlation_id=causation_message_id,
        )

        # Store and log
        self.proposals_store[prop_data.proposal_id] = prop_data.to_dict()
        self.audit_log.append({
            "timestamp": datetime.datetime.now(datetime.timezone.utc).isoformat(),
            "direction": "OUTBOUND",
            "envelope": envelope.to_dict(),
        })

        return envelope
