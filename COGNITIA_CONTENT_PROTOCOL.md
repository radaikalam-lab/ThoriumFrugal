# Cognitia Authorized Content Extraction Protocol Specification

**Protocol Version:** `1.1.0` (Phase 3B Content Extension)  
**Cognitive ABI Compliance:** Cognitia ABI v1.0.0 (`E:\Cognitia\src\cognitia\abi\types.py`)

---

## 1. Schema Definition

Authorized webpage content extractions serialize to the following canonical JSON schema:

```json
{
  "$schema": "https://cognitia.ai/schemas/browser_content_v1.json",
  "id": "urn:uuid:e4b7c120-1122-4889-a5d6-847291a50b3f",
  "schema_version": "1.0.0",
  "created_at": "2026-09-23T15:40:00.000Z",
  "source_id": "thorium_browser_adapter",
  "content_authorization": {
    "mode": "selected_text",
    "user_initiated": true,
    "timestamp_utc": "2026-09-23T15:40:00.000Z",
    "scope": "selection"
  },
  "payload": {
    "browser": {
      "name": "Lean Thorium",
      "version": "138.0.7204.306"
    },
    "page": {
      "url": "https://en.wikipedia.org/wiki/Thorium",
      "origin": "https://en.wikipedia.org",
      "title": "Thorium - Wikipedia",
      "window_id": 1,
      "tab_id": 102
    },
    "content": {
      "type": "untrusted_web_content",
      "text": "Thorium is a weakly radioactive metallic chemical element with the atomic number 90...",
      "byte_length": 87,
      "is_truncated": false
    },
    "provenance": {
      "source_type": "sensor",
      "producer_id": "thorium_browser_adapter",
      "capability_id": "authorized_content_extraction",
      "authorization_method": "explicit_user_action"
    }
  }
}
```

---

## 2. Content Extraction Modes

| Mode | Trigger Action | Payload Content | Size Limit |
| :--- | :--- | :--- | :--- |
| `selected_text` | Context Menu -> "Analyze Selected Text with Cognitia" | User-selected text block | **64 KB** |
| `main_page_content` | Side Panel / Menu -> "Analyze This Page with Cognitia" | Sanitized main document text (stripped of scripts, styles, hidden elements, inputs) | **256 KB** |
| `metadata_only` | Passive Metadata Mode | Title, URL, origin, and high-level headings | **16 KB** |

---

## 3. Disconnect & Privacy Retention Policy

- When Cognitia is **disconnected or not running**, content-bearing payloads are **immediately DROPPED**.
- Extracted webpage content is **never persisted to disk** and **never buffered in memory queues**.
