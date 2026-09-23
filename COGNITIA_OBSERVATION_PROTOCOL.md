# Cognitia Browser Observation Protocol Specification

**Protocol Version:** `1.0.0`  
**Cognitive ABI Compliance:** Cognitia ABI v1.0.0 (`E:\Cognitia\src\cognitia\abi\types.py`)

---

## 1. Schema Definition

All observation events emitted by Lean Thorium serialize to the following canonical JSON schema:

```json
{
  "$schema": "https://cognitia.ai/schemas/browser_observation_v1.json",
  "id": "urn:uuid:8f3c7a21-998b-4a5d-b0e2-124857b29a1c",
  "schema_version": "1.0.0",
  "created_at": "2026-09-23T15:30:00.124Z",
  "source_id": "thorium_browser_adapter",
  "payload": {
    "browser": {
      "name": "Lean Thorium",
      "version": "138.0.7204.306"
    },
    "event": {
      "type": "navigation_committed",
      "timestamp_utc": "2026-09-23T15:30:00.124Z"
    },
    "context": {
      "window_id": 1,
      "tab_id": 402,
      "is_active_tab": true,
      "is_incognito": false
    },
    "page": {
      "url": "https://docs.kernel.org/process/submitting-patches.html",
      "origin": "https://docs.kernel.org",
      "title": "Submitting patches: the essential guide — The Linux Kernel documentation"
    },
    "provenance": {
      "source_type": "sensor",
      "producer_id": "thorium_browser_adapter",
      "capability_id": "browser_observation"
    }
  }
}
```

---

## 2. Event Catalog

| Event Name | Trigger Subsystem | Payload Attributes Included |
| :--- | :--- | :--- |
| `browser_started` | Application Startup | Browser identity, initial window ID |
| `tab_created` | `TabStripModelObserver::TabInsertedAt` | `window_id`, `tab_id`, `is_active_tab` |
| `tab_activated` | `TabStripModelObserver::TabSelectedAt` | `window_id`, `tab_id`, `is_active_tab = true` |
| `tab_closed` | `TabStripModelObserver::TabClosingAt` | `window_id`, `tab_id` |
| `navigation_started` | `WebContentsObserver::DidStartNavigation` | `url` (sanitized), `tab_id` |
| `navigation_committed`| `WebContentsObserver::DidFinishNavigation`| `url`, `origin`, `title`, `tab_id`, `http_status` |
| `page_title_changed` | `WebContentsObserver::TitleWasSet` | `title`, `url`, `tab_id` |
| `page_load_completed` | `WebContentsObserver::DocumentOnLoadCompleted` | `url`, `tab_id`, `load_duration_ms` |

---

## 3. URL Sanitization Policy

1. **Credential Scrubbing:** Removes `user:password@` prefixes.
2. **Query Parameter Redaction:** Automatically redacts `token`, `access_token`, `auth`, `password`, `session_id`, `apikey`, `key`, `sig`, `code`, replacing value with `[REDACTED]`.
3. **Origin-Only Fallback:** If configured with `UrlExposurePolicy::kOriginOnly`, only the scheme, host, and port are transmitted.
