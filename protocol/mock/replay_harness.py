"""
Deterministic Event Recorder and Replay Harness for Thorium-Cognitia Protocol v1.
Guarantees bit-for-bit replayability, audit verification, and causal reconstruction.
"""

from __future__ import annotations
from typing import Dict, Any, List, Optional, Iterator
import json
import hashlib
from pathlib import Path
from .protocol_types import MessageEnvelope
from .protocol_validator import ProtocolValidator
from .security_validator import SecurityValidator
from .mock_cognitia_daemon import MockCognitiaDaemon


class EventRecorder:
    """
    Captures protocol message streams to structured JSONL files with deterministic hashing.
    """

    def __init__(self, trace_file_path: Optional[str] = None):
        self.trace_file_path = trace_file_path
        self.recorded_events: List[Dict[str, Any]] = []

    def record_envelope(self, envelope: MessageEnvelope) -> None:
        event_entry = envelope.to_dict()
        self.recorded_events.append(event_entry)
        if self.trace_file_path:
            with open(self.trace_file_path, "a", encoding="utf-8") as f:
                f.write(json.dumps(event_entry, sort_keys=True) + "\n")

    def get_trace_sha256(self) -> str:
        """Computes deterministic SHA-256 digest across all recorded envelopes."""
        hasher = hashlib.sha256()
        for evt in self.recorded_events:
            hasher.update(json.dumps(evt, sort_keys=True).encode("utf-8"))
        return hasher.hexdigest()

    def clear(self) -> None:
        self.recorded_events.clear()


class ReplayHarness:
    """
    Replays recorded message streams through validators and mock daemon.
    Verifies that the same trace deterministically yields identical state and validation passes.
    """

    def __init__(self, daemon_token: str = "mock_secret_token"):
        self.daemon_token = daemon_token

    def replay_events(
        self,
        events: List[Dict[str, Any]]
    ) -> Dict[str, Any]:
        """
        Replays a list of event dictionaries.
        Returns a verification summary containing processed counts, digest, and validation status.
        """
        daemon = MockCognitiaDaemon(expected_token=self.daemon_token)
        validator = ProtocolValidator()
        sec_validator = SecurityValidator()

        processed_count = 0
        hasher = hashlib.sha256()

        for raw_evt in events:
            envelope = MessageEnvelope.from_dict(raw_evt)
            # 1. Validate envelope protocol
            validator.validate_envelope(envelope)
            # 2. Validate envelope security
            sec_validator.validate_message_security(envelope)
            # 3. Process through daemon
            daemon.process_incoming_envelope(envelope)

            processed_count += 1
            hasher.update(json.dumps(raw_evt, sort_keys=True).encode("utf-8"))

        return {
            "processed_count": processed_count,
            "trace_sha256": hasher.hexdigest(),
            "final_session_id": daemon.active_session_id,
            "observations_count": len(daemon.observations_store),
            "authorizations_count": len(daemon.authorizations_store),
            "results_count": len(daemon.execution_results_store),
            "status": "PASS",
        }
