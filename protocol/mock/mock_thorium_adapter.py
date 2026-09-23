"""
Mock Thorium Adapter.
Simulates Lean Thorium BrowserObservationCollector, UrlSanitizer, BoundedEventQueue, and NamedPipeTransport.
Strictly observation-only with zero execution authority.
"""

from __future__ import annotations
from typing import Dict, Any, List, Optional, Deque
from collections import deque
import uuid
import datetime
from .protocol_types import (
    MessageType,
    MessageEnvelope,
    SourceIdentity,
    ObservationContext,
    ObservationData,
    SessionHelloData,
    SessionCloseData,
)
from .security_validator import SecurityValidator


class BoundedEventQueue:
    """
    Fixed-capacity FIFO queue for IPC message dispatch.
    Implements DROP_OLDEST policy on backpressure to prevent blocking the UI thread.
    """
    def __init__(self, capacity: int = 100):
        self.capacity = capacity
        self.queue: Deque[MessageEnvelope] = deque(maxlen=capacity)
        self.total_enqueued = 0
        self.total_dropped = 0

    def enqueue(self, item: MessageEnvelope) -> bool:
        if len(self.queue) >= self.capacity:
            self.total_dropped += 1
        self.queue.append(item)
        self.total_enqueued += 1
        return True

    def pop(self) -> Optional[MessageEnvelope]:
        if self.queue:
            return self.queue.popleft()
        return None

    def size(self) -> int:
        return len(self.queue)

    def is_empty(self) -> bool:
        return len(self.queue) == 0

    def clear(self) -> None:
        self.queue.clear()


class MockThoriumAdapter:
    """
    Simulates the Thorium C++ Cognitia Adapter.
    Collects passive observations, sanitizes URLs, queues envelopes, and dispatches to IPC.
    Contains ZERO browser execution or mutation APIs.
    """

    def __init__(
        self,
        instance_id: str = "thorium_proc_1",
        queue_capacity: int = 100,
        enable_kill_switch: bool = False
    ):
        self.instance_id = instance_id
        self.is_enabled = not enable_kill_switch
        self.session_id: Optional[str] = None
        self.sequence_number = 0
        self.event_queue = BoundedEventQueue(capacity=queue_capacity)
        self.security_validator = SecurityValidator()
        self.source = SourceIdentity(
            application="thorium",
            adapter="cognitia_adapter",
            component="observation_collector",
            instance_id=self.instance_id,
        )

    def set_kill_switch(self, disabled: bool) -> None:
        """Enable or disable observation collection immediately."""
        self.is_enabled = not disabled
        if disabled:
            self.event_queue.clear()

    def start_session(self, auth_token: str = "mock_secret_token") -> MessageEnvelope:
        """Initiate protocol handshake."""
        self.session_id = str(uuid.uuid4())
        self.sequence_number = 1
        hello_payload = SessionHelloData(
            client_version="1.0.0",
            client_capabilities=["OBSERVATION", "SANITIZATION_V1"],
            auth_token=auth_token,
            nonce=str(uuid.uuid4()),
        ).to_dict()

        envelope = MessageEnvelope(
            protocol_version="1.0",
            message_type=MessageType.SESSION_HELLO,
            message_id=str(uuid.uuid4()),
            session_id=self.session_id,
            sequence_number=self.sequence_number,
            timestamp=datetime.datetime.now(datetime.timezone.utc).isoformat(),
            source=self.source,
            payload=hello_payload,
        )
        self.event_queue.enqueue(envelope)
        return envelope

    def close_session(self, reason: str = "USER_REQUEST") -> Optional[MessageEnvelope]:
        """Send session close."""
        if not self.session_id:
            return None
        self.sequence_number += 1
        envelope = MessageEnvelope(
            protocol_version="1.0",
            message_type=MessageType.SESSION_CLOSE,
            message_id=str(uuid.uuid4()),
            session_id=self.session_id,
            sequence_number=self.sequence_number,
            timestamp=datetime.datetime.now(datetime.timezone.utc).isoformat(),
            source=self.source,
            payload={"reason": reason},
        )
        self.event_queue.enqueue(envelope)
        return envelope

    def observe_navigation(
        self,
        url: str,
        tab_id: int,
        profile_id: str = "profile_default",
        title: str = "",
        is_incognito: bool = False,
        navigation_id: Optional[str] = None
    ) -> Optional[MessageEnvelope]:
        """Capture a navigation observation (sanitizing URL)."""
        if not self.is_enabled or not self.session_id:
            return None

        clean_url = self.security_validator.sanitize_url(url)
        context = ObservationContext(
            profile_id=profile_id if not is_incognito else "incognito_transient",
            tab_id=tab_id,
            is_incognito=is_incognito,
            url=clean_url,
            title=title,
            navigation_id=navigation_id or str(uuid.uuid4()),
        )

        obs_data = ObservationData(
            observation_type="PAGE_NAVIGATION",
            context=context,
            data={"url": clean_url, "title": title},
            epistemic_status="UNRESOLVED",
            source_type="SENSOR",
        )

        self.sequence_number += 1
        envelope = MessageEnvelope(
            protocol_version="1.0",
            message_type=MessageType.OBSERVATION,
            message_id=str(uuid.uuid4()),
            session_id=self.session_id,
            sequence_number=self.sequence_number,
            timestamp=datetime.datetime.now(datetime.timezone.utc).isoformat(),
            source=self.source,
            payload=obs_data.to_dict(),
        )
        self.event_queue.enqueue(envelope)
        return envelope

    def observe_content_extraction(
        self,
        url: str,
        tab_id: int,
        extracted_text: str,
        selector: str = "body",
        is_incognito: bool = False
    ) -> Optional[MessageEnvelope]:
        """Capture visible text content observation."""
        if not self.is_enabled or not self.session_id:
            return None

        clean_url = self.security_validator.sanitize_url(url)
        has_prompt_injection = self.security_validator.detect_prompt_injection_in_observation(extracted_text)

        context = ObservationContext(
            profile_id="incognito_transient" if is_incognito else "profile_default",
            tab_id=tab_id,
            is_incognito=is_incognito,
            url=clean_url,
        )

        obs_data = ObservationData(
            observation_type="CONTENT_EXTRACTION",
            context=context,
            data={
                "extracted_text": extracted_text,
                "selector": selector,
                "quarantined_suspicious": has_prompt_injection
            },
            epistemic_status="UNRESOLVED",
            source_type="SENSOR",
        )

        self.sequence_number += 1
        envelope = MessageEnvelope(
            protocol_version="1.0",
            message_type=MessageType.OBSERVATION,
            message_id=str(uuid.uuid4()),
            session_id=self.session_id,
            sequence_number=self.sequence_number,
            timestamp=datetime.datetime.now(datetime.timezone.utc).isoformat(),
            source=self.source,
            payload=obs_data.to_dict(),
        )
        self.event_queue.enqueue(envelope)
        return envelope
