# Cognitia Content Extraction Architecture & Lifecycle Specification

**Document Version:** 1.0.0 (Phase 3 Final)

---

## 1. Content Extraction Lifecycle

```text
┌─────────────────────────────────────────────────────────────────────────────────────────┐
│                              CONTENT EXTRACTION LIFECYCLE                               │
│                                                                                         │
│   [User Action] ──► Context Menu ("Analyze Selected Text with Cognitia")                │
│                            │                                                            │
│                            ▼                                                            │
│   [ContentExtractor] ─────► Asynchronous DOM Text Extraction                            │
│                            • Max Size Bounds: Selected (64 KB), Page (256 KB)           │
│                            • Structural Exclusions: <input type=password>, cookies      │
│                            • URL Sanitizer: Scrub user:pass and query tokens            │
│                            │                                                            │
│                            ▼                                                            │
│   [ContentEnvelope] ──────► Canonical Observation Payload Serialization                 │
│                            • Tagged: "type": "untrusted_web_content"                    │
│                            • Authorization: "user_initiated": true                      │
│                            │                                                            │
│                            ▼                                                            │
│   [NamedPipeTransport] ───► Local Pipe (\\.\pipe\cognitia_browser_stream)               │
│                            • If DISCONNECTED: Payload DROPPED immediately (0 retention) │
│                            • If CONNECTED: Streamed asynchronously to Cognitia daemon   │
└─────────────────────────────────────────────────────────────────────────────────────────┘
```

---

## 2. Content Modes & Policy Invariants

1. **Mode A — Selected Text:** Bounded to 64 KB; captures user-highlighted text block.
2. **Mode B — Main Page Content:** Bounded to 256 KB; extracts readable body text stripped of scripts, styles, inputs, and cross-origin frames.
3. **Mode C — Page Metadata Only:** Bounded to 16 KB; extracts document title, origin, URL, and headings.
4. **Automatic Content Capture = OFF:** Content extraction NEVER occurs passively on navigation.
