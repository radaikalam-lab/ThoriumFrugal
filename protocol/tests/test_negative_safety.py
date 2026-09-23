"""
Tests for Negative Protocol Safety and Boundary Violations.
All negative safety tests must fail closed.
"""

import pytest
import uuid
import datetime
from mock.protocol_types import (
    MessageType,
    MessageEnvelope,
    SourceIdentity,
)
from mock.protocol_validator import ProtocolValidator, ProtocolValidationError
from mock.mock_thorium_adapter import MockThoriumAdapter, BoundedEventQueue


def test_unsupported_major_version_fails_closed():
    with pytest.raises(ValueError) as exc_info:
        MessageEnvelope(
            protocol_version="2.0",  # Incompatible major version
            message_type=MessageType.SESSION_HELLO,
            message_id=str(uuid.uuid4()),
            session_id=str(uuid.uuid4()),
            sequence_number=1,
            timestamp=datetime.datetime.now(datetime.timezone.utc).isoformat(),
            source=SourceIdentity(),
            payload={},
        )
    assert "Unsupported protocol major version" in str(exc_info.value)


def test_malformed_message_id_fails_closed():
    with pytest.raises(ValueError) as exc_info:
        MessageEnvelope(
            protocol_version="1.0",
            message_type=MessageType.SESSION_HELLO,
            message_id="not-a-valid-uuid-12345",
            session_id=str(uuid.uuid4()),
            sequence_number=1,
            timestamp=datetime.datetime.now(datetime.timezone.utc).isoformat(),
            source=SourceIdentity(),
            payload={},
        )
    assert "Invalid message_id UUID" in str(exc_info.value)


def test_invalid_sequence_number_fails_closed():
    with pytest.raises(ValueError) as exc_info:
        MessageEnvelope(
            protocol_version="1.0",
            message_type=MessageType.SESSION_HELLO,
            message_id=str(uuid.uuid4()),
            session_id=str(uuid.uuid4()),
            sequence_number=0,  # Invalid: must be >= 1
            timestamp=datetime.datetime.now(datetime.timezone.utc).isoformat(),
            source=SourceIdentity(),
            payload={},
        )
    assert "Sequence number must be >= 1" in str(exc_info.value)


def test_out_of_order_sequence_rejection():
    validator = ProtocolValidator()
    session_id = str(uuid.uuid4())
    src = SourceIdentity()

    # Hello seq 1
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
    validator.validate_envelope(hello)

    # Obs seq 2
    obs1 = MessageEnvelope(
        protocol_version="1.0",
        message_type=MessageType.OBSERVATION,
        message_id=str(uuid.uuid4()),
        session_id=session_id,
        sequence_number=2,
        timestamp=datetime.datetime.now(datetime.timezone.utc).isoformat(),
        source=src,
        payload={"observation_type": "NAV", "context": {"tab_id": 1, "url": "https://a.com"}},
    )
    validator.validate_envelope(obs1)

    # Send obs with seq 1 again
    obs_regress = MessageEnvelope(
        protocol_version="1.0",
        message_type=MessageType.OBSERVATION,
        message_id=str(uuid.uuid4()),
        session_id=session_id,
        sequence_number=1,
        timestamp=datetime.datetime.now(datetime.timezone.utc).isoformat(),
        source=src,
        payload={"observation_type": "NAV", "context": {"tab_id": 1, "url": "https://b.com"}},
    )
    with pytest.raises(ProtocolValidationError) as exc_info:
        validator.validate_envelope(obs_regress)
    assert exc_info.value.code in ("ERR_DUPLICATE_SEQUENCE", "ERR_OUT_OF_ORDER_SEQUENCE")


def test_stale_session_rejection():
    validator = ProtocolValidator()
    session_1 = str(uuid.uuid4())
    session_2 = str(uuid.uuid4())
    src = SourceIdentity()

    # Establish session 1
    hello1 = MessageEnvelope(
        protocol_version="1.0",
        message_type=MessageType.SESSION_HELLO,
        message_id=str(uuid.uuid4()),
        session_id=session_1,
        sequence_number=1,
        timestamp=datetime.datetime.now(datetime.timezone.utc).isoformat(),
        source=src,
        payload={},
    )
    validator.validate_envelope(hello1)

    accept1 = MessageEnvelope(
        protocol_version="1.0",
        message_type=MessageType.SESSION_ACCEPT,
        message_id=str(uuid.uuid4()),
        session_id=session_1,
        sequence_number=1,
        timestamp=datetime.datetime.now(datetime.timezone.utc).isoformat(),
        source=SourceIdentity(application="cognitia"),
        payload={},
    )
    validator.validate_envelope(accept1)

    # Now send a message with session_2 without hello
    stale_msg = MessageEnvelope(
        protocol_version="1.0",
        message_type=MessageType.OBSERVATION,
        message_id=str(uuid.uuid4()),
        session_id=session_2,
        sequence_number=2,
        timestamp=datetime.datetime.now(datetime.timezone.utc).isoformat(),
        source=src,
        payload={"observation_type": "NAV", "context": {"tab_id": 1, "url": "https://a.com"}},
    )
    with pytest.raises(ProtocolValidationError) as exc_info:
        validator.validate_envelope(stale_msg)
    assert exc_info.value.code == "ERR_STALE_SESSION"


def test_bounded_queue_backpressure_drop_oldest():
    q = BoundedEventQueue(capacity=3)
    src = SourceIdentity()
    sess = str(uuid.uuid4())

    env1 = MessageEnvelope("1.0", MessageType.OBSERVATION, str(uuid.uuid4()), sess, 1, "2026-01-01T00:00:00Z", src, {})
    env2 = MessageEnvelope("1.0", MessageType.OBSERVATION, str(uuid.uuid4()), sess, 2, "2026-01-01T00:00:00Z", src, {})
    env3 = MessageEnvelope("1.0", MessageType.OBSERVATION, str(uuid.uuid4()), sess, 3, "2026-01-01T00:00:00Z", src, {})
    env4 = MessageEnvelope("1.0", MessageType.OBSERVATION, str(uuid.uuid4()), sess, 4, "2026-01-01T00:00:00Z", src, {})

    q.enqueue(env1)
    q.enqueue(env2)
    q.enqueue(env3)
    assert q.size() == 3
    assert q.total_dropped == 0

    # Overflows capacity
    q.enqueue(env4)
    assert q.size() == 3
    assert q.total_dropped == 1

    # Oldest (env1) was dropped
    first_popped = q.pop()
    assert first_popped is not None
    assert first_popped.sequence_number == 2


def test_kill_switch_stops_all_observations():
    adapter = MockThoriumAdapter(instance_id="killswitch_test")
    adapter.start_session()
    assert adapter.event_queue.size() == 1

    # Activate kill switch
    adapter.set_kill_switch(disabled=True)
    assert adapter.is_enabled is False
    assert adapter.event_queue.size() == 0  # Cleared queue

    # Attempt observations while disabled
    obs = adapter.observe_navigation("https://example.com", tab_id=1)
    assert obs is None
    assert adapter.event_queue.size() == 0
