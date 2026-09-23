"""
Thorium-Cognitia Protocol v1 Types and Schemas
Strictly enforces protocol contracts, type invariants, and authority = NONE.
"""

from __future__ import annotations
from dataclasses import dataclass, field, asdict
from enum import Enum
from typing import Any, Dict, List, Optional
import json
import uuid
from datetime import datetime, timezone


class MessageType(str, Enum):
    SESSION_HELLO = "SESSION_HELLO"
    SESSION_ACCEPT = "SESSION_ACCEPT"
    SESSION_REJECT = "SESSION_REJECT"
    SESSION_CLOSE = "SESSION_CLOSE"
    OBSERVATION = "OBSERVATION"
    OBSERVATION_BATCH = "OBSERVATION_BATCH"
    DIRECTIONAL_SPEC = "DIRECTIONAL_SPEC"
    CANDIDATE_ACTION_PROPOSAL = "CANDIDATE_ACTION_PROPOSAL"
    EXECUTION_AUTHORIZATION_OBSERVED = "EXECUTION_AUTHORIZATION_OBSERVED"
    EXECUTION_RESULT_OBSERVED = "EXECUTION_RESULT_OBSERVED"
    ERROR = "ERROR"
    HEARTBEAT = "HEARTBEAT"


class ExecutionStatus(str, Enum):
    PROPOSED = "PROPOSED"
    AUTHORIZED_EXTERNALLY = "AUTHORIZED_EXTERNALLY"
    EXECUTED = "EXECUTED"
    FAILED = "FAILED"
    REJECTED = "REJECTED"
    CANCELLED = "CANCELLED"
    UNKNOWN = "UNKNOWN"


class RiskClassification(str, Enum):
    LOW = "LOW"
    MEDIUM = "MEDIUM"
    HIGH = "HIGH"
    CRITICAL = "CRITICAL"


@dataclass(frozen=True)
class SourceIdentity:
    application: str = "thorium"
    adapter: str = "cognitia_adapter"
    component: str = "observation_collector"
    instance_id: str = "default_instance"

    def to_dict(self) -> Dict[str, Any]:
        return asdict(self)


@dataclass(frozen=True)
class ObservationContext:
    profile_id: str
    tab_id: int
    is_incognito: bool
    url: str
    title: str = ""
    navigation_id: Optional[str] = None

    def to_dict(self) -> Dict[str, Any]:
        return asdict(self)


@dataclass(frozen=True)
class MessageEnvelope:
    protocol_version: str
    message_type: MessageType
    message_id: str
    session_id: str
    sequence_number: int
    timestamp: str
    source: SourceIdentity
    payload: Dict[str, Any]
    correlation_id: Optional[str] = None
    causation_id: Optional[str] = None

    def __post_init__(self):
        if not self.protocol_version.startswith("1."):
            raise ValueError(f"Unsupported protocol major version: {self.protocol_version}")
        if self.sequence_number < 1:
            raise ValueError(f"Sequence number must be >= 1, got {self.sequence_number}")
        try:
            uuid.UUID(self.message_id)
        except Exception as e:
            raise ValueError(f"Invalid message_id UUID: {self.message_id}") from e

    def to_dict(self) -> Dict[str, Any]:
        return {
            "protocol_version": self.protocol_version,
            "message_type": self.message_type.value if isinstance(self.message_type, MessageType) else str(self.message_type),
            "message_id": self.message_id,
            "session_id": self.session_id,
            "sequence_number": self.sequence_number,
            "timestamp": self.timestamp,
            "source": self.source.to_dict() if isinstance(self.source, SourceIdentity) else self.source,
            "payload": self.payload,
            "correlation_id": self.correlation_id,
            "causation_id": self.causation_id,
        }

    def to_json(self) -> str:
        return json.dumps(self.to_dict(), ensure_ascii=False, sort_keys=True)

    @classmethod
    def from_dict(cls, d: Dict[str, Any]) -> MessageEnvelope:
        src_raw = d.get("source", {})
        src = SourceIdentity(
            application=src_raw.get("application", "thorium"),
            adapter=src_raw.get("adapter", "cognitia_adapter"),
            component=src_raw.get("component", "observation_collector"),
            instance_id=src_raw.get("instance_id", "default_instance"),
        )
        return cls(
            protocol_version=d["protocol_version"],
            message_type=MessageType(d["message_type"]),
            message_id=d["message_id"],
            session_id=d["session_id"],
            sequence_number=int(d["sequence_number"]),
            timestamp=d["timestamp"],
            source=src,
            payload=d.get("payload", {}),
            correlation_id=d.get("correlation_id"),
            causation_id=d.get("causation_id"),
        )

    @classmethod
    def from_json(cls, json_str: str) -> MessageEnvelope:
        return cls.from_dict(json.loads(json_str))


# Payload Schemas

@dataclass
class SessionHelloData:
    client_version: str = "1.0.0"
    client_capabilities: List[str] = field(default_factory=lambda: ["OBSERVATION", "SANITIZATION_V1"])
    auth_token: str = ""
    nonce: str = ""

    def to_dict(self) -> Dict[str, Any]:
        return asdict(self)


@dataclass
class SessionAcceptData:
    session_id: str
    negotiated_version: str = "1.0.0"
    negotiated_capabilities: List[str] = field(default_factory=lambda: ["OBSERVATION", "SANITIZATION_V1"])
    heartbeat_interval_ms: int = 5000
    server_nonce: str = ""

    def to_dict(self) -> Dict[str, Any]:
        return asdict(self)


@dataclass
class SessionRejectData:
    reason_code: str
    message: str

    def to_dict(self) -> Dict[str, Any]:
        return asdict(self)


@dataclass
class SessionCloseData:
    reason: str = "USER_REQUEST"

    def to_dict(self) -> Dict[str, Any]:
        return asdict(self)


@dataclass
class ErrorData:
    error_code: str
    message: str
    details: Optional[Dict[str, Any]] = None

    def to_dict(self) -> Dict[str, Any]:
        return asdict(self)


@dataclass
class ObservationData:
    observation_type: str
    context: ObservationContext
    data: Dict[str, Any]
    epistemic_status: str = "UNRESOLVED"
    source_type: str = "SENSOR"

    def to_dict(self) -> Dict[str, Any]:
        return {
            "observation_type": self.observation_type,
            "context": self.context.to_dict() if isinstance(self.context, ObservationContext) else self.context,
            "data": self.data,
            "epistemic_status": self.epistemic_status,
            "source_type": self.source_type,
        }


@dataclass
class DirectionalSpecData:
    spec_id: str
    domain_id: str
    current_state: Dict[str, Any]
    context: Dict[str, Any]
    desired_direction: str
    target_state: Dict[str, Any]
    objectives: List[str]
    constraints: List[str]
    allowed_capabilities: List[str]

    def to_dict(self) -> Dict[str, Any]:
        return asdict(self)


@dataclass
class CandidateActionProposalData:
    proposal_id: str
    source_model: str
    domain_id: str
    intent: str
    target: str
    candidate_action: Dict[str, Any]
    rationale: str
    constraints: List[str]
    risk_classification: RiskClassification
    created_at: str
    authority: str = "NONE"  # MANDATORY INVARIANT: MUST BE "NONE"

    def __post_init__(self):
        if self.authority != "NONE":
            raise ValueError(f"Authority violation: CandidateActionProposal authority must be 'NONE', got {self.authority}")

    def to_dict(self) -> Dict[str, Any]:
        return {
            "proposal_id": self.proposal_id,
            "source_model": self.source_model,
            "domain_id": self.domain_id,
            "intent": self.intent,
            "target": self.target,
            "candidate_action": self.candidate_action,
            "rationale": self.rationale,
            "constraints": self.constraints,
            "risk_classification": self.risk_classification.value if isinstance(self.risk_classification, RiskClassification) else str(self.risk_classification),
            "created_at": self.created_at,
            "authority": self.authority,
        }


@dataclass
class ExecutionAuthObservedData:
    authorization_id: str
    proposal_id: str
    decision_source: str  # "EXTERNAL_USER" or "POLICY_ENGINE"
    approved: bool
    authorized_action: Dict[str, Any]
    timestamp: str
    cognitia_authority: str = "NONE"  # Always NONE

    def __post_init__(self):
        if self.cognitia_authority != "NONE":
            raise ValueError("Cognitia authority violation: must be 'NONE'")
        if self.decision_source == "COGNITIA":
            raise ValueError("Decision source cannot be COGNITIA")

    def to_dict(self) -> Dict[str, Any]:
        return asdict(self)


@dataclass
class ExecutionResultObservedData:
    result_id: str
    proposal_id: str
    authorization_id: Optional[str]
    status: ExecutionStatus
    actual_outcome: Dict[str, Any]
    observed_at: str
    execution_duration_ms: Optional[float] = None
    error_message: Optional[str] = None

    def to_dict(self) -> Dict[str, Any]:
        return {
            "result_id": self.result_id,
            "proposal_id": self.proposal_id,
            "authorization_id": self.authorization_id,
            "status": self.status.value if isinstance(self.status, ExecutionStatus) else str(self.status),
            "actual_outcome": self.actual_outcome,
            "observed_at": self.observed_at,
            "execution_duration_ms": self.execution_duration_ms,
            "error_message": self.error_message,
        }
