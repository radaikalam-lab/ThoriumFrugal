"""
Tests for Protocol v1 Framing, Handshake, Sequencing, and Correlation.
"""

import pytest
import uuid
import datetime
from mock.protocol_types import (
    MessageType,
    MessageEnvelope,
    SourceIdentity,
    ObservationContext,
    ObservationData,
)
from mock.protocol_validator import ProtocolValidator, ProtocolValidationError
from mock.mock_thorium_adapter import MockThoriumAdapter
from mock.mock_cognitia_daemon import MockCognitiaDaemon


def test_handshake_flow_success():
    adapter = MockThoriumAdapter(instance_id="thorium_test_1")
    daemon = MockCognitiaDaemon(expected_token="mock_secret_token")

    hello_env = adapter.start_session(auth_token="mock_secret_token")
    assert hello_env.message_type == MessageType.SESSION_HELLO
    assert hello_env.sequence_number == 1

    success, accept_env = daemon.process_incoming_envelope(hello_env)
    assert success is True
    assert accept_env is not None
    assert accept_env.message_type == MessageType.SESSION_ACCEPT
    assert accept_env.session_id == hello_env.session_id
    assert accept_env.correlation_id == hello_env.message_id
    assert accept_env.causation_id == hello_env.message_id


def test_handshake_authentication_failure():
    adapter = MockThoriumAdapter(instance_id="thorium_test_2")
    daemon = MockCognitiaDaemon(expected_token="mock_secret_token")

    hello_env = adapter.start_session(auth_token="invalid_token")
    success, reject_env = daemon.process_incoming_envelope(hello_env)
    assert success is False
    assert reject_env is not None
    assert reject_env.message_type == MessageType.SESSION_REJECT
    assert reject_env.payload["reason_code"] == "AUTH_FAILED"


def test_sequence_monotonicity_and_gap_detection():
    validator = ProtocolValidator()
    session_id = str(uuid.uuid4())
    src = SourceIdentity()

    # Session Hello (seq = 1)
    hello = MessageEnvelope(
        protocol_version="1.0",
        message_type=MessageType.SESSION_HELLO,
        message_id=str(uuid.uuid4()),
        session_id=session_id,
        sequence_number=1,
        timestamp=datetime.datetime.now(datetime.timezone.utc).isoformat(),
        source=src,
        payload={"client_version": "1.0.0"},
    )
    ok, _ = validator.validate_envelope(hello)
    assert ok is True

    # Next valid message (seq = 2)
    obs1 = MessageEnvelope(
        protocol_version="1.0",
        message_type=MessageType.OBSERVATION,
        message_id=str(uuid.uuid4()),
        session_id=session_id,
        sequence_number=2,
        timestamp=datetime.datetime.now(datetime.timezone.utc).isoformat(),
        source=src,
        payload={
            "observation_type": "PAGE_NAVIGATION",
            "context": {"tab_id": 1, "url": "https://example.com"},
            "data": {},
        },
    )
    ok, _ = validator.validate_envelope(obs1)
    assert ok is True

    # Sequence gap (seq = 4 instead of 3)
    obs_gap = MessageEnvelope(
        protocol_version="1.0",
        message_type=MessageType.OBSERVATION,
        message_id=str(uuid.uuid4()),
        session_id=session_id,
        sequence_number=4,
        timestamp=datetime.datetime.now(datetime.timezone.utc).isoformat(),
        source=src,
        payload={
            "observation_type": "PAGE_NAVIGATION",
            "context": {"tab_id": 1, "url": "https://example.com"},
            "data": {},
        },
    )
    with pytest.raises(ProtocolValidationError) as exc_info:
        validator.validate_envelope(obs_gap)
    assert exc_info.value.code == "ERR_SEQUENCE_GAP"


def test_duplicate_message_id_rejection():
    validator = ProtocolValidator()
    session_id = str(uuid.uuid4())
    msg_id = str(uuid.uuid4())
    src = SourceIdentity()

    hello = MessageEnvelope(
        protocol_version="1.0",
        message_type=MessageType.SESSION_HELLO,
        message_id=msg_id,
        session_id=session_id,
        sequence_number=1,
        timestamp=datetime.datetime.now(datetime.timezone.utc).isoformat(),
        source=src,
        payload={"client_version": "1.0.0"},
    )
    validator.validate_envelope(hello)

    # Re-using the same message_id
    duplicate_msg = MessageEnvelope(
        protocol_version="1.0",
        message_type=MessageType.OBSERVATION,
        message_id=msg_id,
        session_id=session_id,
        sequence_number=2,
        timestamp=datetime.datetime.now(datetime.timezone.utc).isoformat(),
        source=src,
        payload={
            "observation_type": "PAGE_NAVIGATION",
            "context": {"tab_id": 1, "url": "https://example.com"},
            "data": {},
        },
    )
    with pytest.raises(ProtocolValidationError) as exc_info:
        validator.validate_envelope(duplicate_msg)
    assert exc_info.value.code == "ERR_DUPLICATE_MESSAGE_ID"


def test_json_serialization_roundtrip():
    src = SourceIdentity(application="thorium", adapter="cognitia_adapter", instance_id="inst_1")
    env = MessageEnvelope(
        protocol_version="1.0",
        message_type=MessageType.OBSERVATION,
        message_id=str(uuid.uuid4()),
        session_id=str(uuid.uuid4()),
        sequence_number=5,
        timestamp=datetime.datetime.now(datetime.timezone.utc).isoformat(),
        source=src,
        payload={"key": "value", "count": 42},
        correlation_id="corr_123",
        causation_id="cause_123",
    )
    json_str = env.to_json()
    reconstructed = MessageEnvelope.from_json(json_str)

    assert reconstructed.protocol_version == env.protocol_version
    assert reconstructed.message_type == env.message_type
    assert reconstructed.message_id == env.message_id
    assert reconstructed.session_id == env.session_id
    assert reconstructed.sequence_number == env.sequence_number
    assert reconstructed.payload == env.payload
    assert reconstructed.correlation_id == env.correlation_id
    assert reconstructed.causation_id == env.causation_id
