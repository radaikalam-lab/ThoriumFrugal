"""
Tests for Authority and Governance Law.
Proves Cognitia execution authority = NONE, external authorization requirement, and execution observation.
"""

import pytest
import uuid
import datetime
from mock.protocol_types import (
    MessageType,
    MessageEnvelope,
    SourceIdentity,
    ExecutionStatus,
    RiskClassification,
    CandidateActionProposalData,
    ExecutionAuthObservedData,
    ExecutionResultObservedData,
)
from mock.protocol_validator import ProtocolValidator, ProtocolValidationError
from mock.security_validator import SecurityValidator, SecurityViolationError
from mock.mock_cognitia_daemon import MockCognitiaDaemon
from mock.mock_thorium_adapter import MockThoriumAdapter


def test_candidate_action_proposal_authority_none_enforced():
    daemon = MockCognitiaDaemon()
    daemon.active_session_id = str(uuid.uuid4())

    prop_env = daemon.generate_candidate_proposal(
        domain_id="browser_navigation",
        intent="NAVIGATE_TO_HELP_DOCS",
        target="tab_1",
        candidate_action={"action_type": "NAVIGATE", "url": "https://help.example.com"},
        rationale="User requested help page.",
    )

    assert prop_env.message_type == MessageType.CANDIDATE_ACTION_PROPOSAL
    assert prop_env.payload["authority"] == "NONE"


def test_proposal_authority_tampering_rejected_by_schema():
    with pytest.raises(ValueError) as exc_info:
        CandidateActionProposalData(
            proposal_id=str(uuid.uuid4()),
            source_model="model_x",
            domain_id="browser",
            intent="AUTO_CLICK",
            target="button_1",
            candidate_action={"action": "CLICK"},
            rationale="Test",
            constraints=[],
            risk_classification=RiskClassification.HIGH,
            created_at=datetime.datetime.now(datetime.timezone.utc).isoformat(),
            authority="AUTONOMOUS_EXECUTE",  # VIOLATION
        )
    assert "Authority violation" in str(exc_info.value)


def test_proposal_authority_tampering_rejected_by_validators():
    sec_validator = SecurityValidator()
    proto_validator = ProtocolValidator()
    session_id = str(uuid.uuid4())
    proto_validator.active_session_id = session_id
    proto_validator.session_established = True

    tampered_env = MessageEnvelope(
        protocol_version="1.0",
        message_type=MessageType.CANDIDATE_ACTION_PROPOSAL,
        message_id=str(uuid.uuid4()),
        session_id=session_id,
        sequence_number=1,
        timestamp=datetime.datetime.now(datetime.timezone.utc).isoformat(),
        source=SourceIdentity(),
        payload={
            "proposal_id": str(uuid.uuid4()),
            "authority": "BROWSER_ADMIN",  # VIOLATION
        },
    )

    with pytest.raises(SecurityViolationError) as exc_info:
        sec_validator.validate_message_security(tampered_env)
    assert exc_info.value.invariant_id == "S7_AUTHORITY_VIOLATION"

    with pytest.raises(ProtocolValidationError) as exc_proto:
        proto_validator.validate_envelope(tampered_env)
    assert exc_proto.value.code == "ERR_AUTHORITY_VIOLATION"


def test_external_authorization_observation_lifecycle():
    adapter = MockThoriumAdapter(instance_id="thorium_gov_test")
    daemon = MockCognitiaDaemon(expected_token="mock_secret_token")

    # Handshake
    hello_env = adapter.start_session(auth_token="mock_secret_token")
    ok, _ = daemon.process_incoming_envelope(hello_env)
    assert ok is True
    session_id = hello_env.session_id

    # 1. Generate advisory proposal
    prop_env = daemon.generate_candidate_proposal(
        domain_id="browser_nav",
        intent="NAVIGATE",
        target="tab_1",
        candidate_action={"action": "NAVIGATE", "url": "https://trusted.org"},
        rationale="User research task",
    )
    prop_id = prop_env.payload["proposal_id"]

    # 2. Simulate External Host Approval (e.g. user clicked Approve dialog)
    auth_data = ExecutionAuthObservedData(
        authorization_id=str(uuid.uuid4()),
        proposal_id=prop_id,
        decision_source="EXTERNAL_USER",
        approved=True,
        authorized_action={"action": "NAVIGATE", "url": "https://trusted.org"},
        timestamp=datetime.datetime.now(datetime.timezone.utc).isoformat(),
        cognitia_authority="NONE",
    )

    auth_env = MessageEnvelope(
        protocol_version="1.0",
        message_type=MessageType.EXECUTION_AUTHORIZATION_OBSERVED,
        message_id=str(uuid.uuid4()),
        session_id=session_id,
        sequence_number=2,
        timestamp=datetime.datetime.now(datetime.timezone.utc).isoformat(),
        source=SourceIdentity(application="thorium", adapter="host_governor"),
        payload=auth_data.to_dict(),
        correlation_id=prop_env.message_id,
        causation_id=prop_env.message_id,
    )

    ok, _ = daemon.process_incoming_envelope(auth_env)
    assert ok is True
    assert prop_id in daemon.authorizations_store

    # 3. Simulate Host Browser Execution Result Observation
    result_data = ExecutionResultObservedData(
        result_id=str(uuid.uuid4()),
        proposal_id=prop_id,
        authorization_id=auth_data.authorization_id,
        status=ExecutionStatus.EXECUTED,
        actual_outcome={"status_code": 200, "final_url": "https://trusted.org"},
        observed_at=datetime.datetime.now(datetime.timezone.utc).isoformat(),
        execution_duration_ms=145.2,
    )

    result_env = MessageEnvelope(
        protocol_version="1.0",
        message_type=MessageType.EXECUTION_RESULT_OBSERVED,
        message_id=str(uuid.uuid4()),
        session_id=session_id,
        sequence_number=3,
        timestamp=datetime.datetime.now(datetime.timezone.utc).isoformat(),
        source=SourceIdentity(application="thorium", adapter="cognitia_adapter"),
        payload=result_data.to_dict(),
        correlation_id=auth_env.message_id,
        causation_id=auth_env.message_id,
    )

    ok, _ = daemon.process_incoming_envelope(result_env)
    assert ok is True
    assert prop_id in daemon.execution_results_store
    assert daemon.execution_results_store[prop_id]["status"] == "EXECUTED"


def test_cognitia_cannot_be_authorization_source():
    with pytest.raises(ValueError) as exc_info:
        ExecutionAuthObservedData(
            authorization_id=str(uuid.uuid4()),
            proposal_id="prop_1",
            decision_source="COGNITIA",  # VIOLATION
            approved=True,
            authorized_action={},
            timestamp=datetime.datetime.now(datetime.timezone.utc).isoformat(),
            cognitia_authority="NONE",
        )
    assert "Decision source cannot be COGNITIA" in str(exc_info.value)
