"""
Tests for Deterministic Event Recording and Trace Replay.
"""

import pytest
import uuid
import datetime
import tempfile
import os
from mock.protocol_types import (
    MessageType,
    MessageEnvelope,
    SourceIdentity,
)
from mock.mock_thorium_adapter import MockThoriumAdapter
from mock.replay_harness import EventRecorder, ReplayHarness


def test_deterministic_recording_and_replay():
    # 1. Setup simulated trace
    adapter = MockThoriumAdapter(instance_id="replay_thorium_inst")
    recorder = EventRecorder()

    # Step 1: Hello
    hello_env = adapter.start_session(auth_token="mock_secret_token")
    recorder.record_envelope(hello_env)

    # Step 2: Navigations
    nav1 = adapter.observe_navigation(url="https://site.org/home", tab_id=1, title="Home")
    assert nav1 is not None
    recorder.record_envelope(nav1)

    nav2 = adapter.observe_navigation(url="https://site.org/about?token=secret123", tab_id=1, title="About")
    assert nav2 is not None
    recorder.record_envelope(nav2)

    # Step 3: Content
    content1 = adapter.observe_content_extraction(
        url="https://site.org/article",
        tab_id=1,
        extracted_text="Safe plain text article without injections.",
    )
    assert content1 is not None
    recorder.record_envelope(content1)

    # Step 4: Close
    close_env = adapter.close_session()
    assert close_env is not None
    recorder.record_envelope(close_env)

    # Verify SHA-256 is stable
    digest1 = recorder.get_trace_sha256()
    assert len(digest1) == 64

    # 2. Replay trace through ReplayHarness
    harness = ReplayHarness(daemon_token="mock_secret_token")
    result = harness.replay_events(recorder.recorded_events)

    assert result["status"] == "PASS"
    assert result["processed_count"] == 5
    assert result["observations_count"] == 3
    assert result["trace_sha256"] == digest1


def test_replay_file_io_persistence():
    with tempfile.NamedTemporaryFile(suffix=".jsonl", delete=False) as tf:
        trace_path = tf.name

    try:
        adapter = MockThoriumAdapter(instance_id="io_thorium_inst")
        recorder = EventRecorder(trace_file_path=trace_path)

        hello_env = adapter.start_session(auth_token="mock_secret_token")
        recorder.record_envelope(hello_env)

        obs = adapter.observe_navigation("https://news.ycombinator.com", tab_id=2)
        assert obs is not None
        recorder.record_envelope(obs)

        # Read back from file
        events_from_file = []
        with open(trace_path, "r", encoding="utf-8") as f:
            for line in f:
                if line.strip():
                    events_from_file.append(MessageEnvelope.from_json(line).to_dict())

        harness = ReplayHarness()
        replay_result = harness.replay_events(events_from_file)
        assert replay_result["status"] == "PASS"
        assert replay_result["processed_count"] == 2
        assert replay_result["observations_count"] == 1
    finally:
        if os.path.exists(trace_path):
            os.remove(trace_path)
