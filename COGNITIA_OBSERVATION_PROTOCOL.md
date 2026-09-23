# Cognitia Browser Observation & Content Protocol Specification

**Protocol Version:** `1.1.0` (Phase 3A Observation + Phase 3B Content Extension)  
**Cognitive ABI Compliance:** Cognitia ABI v1.0.0 (`E:\Cognitia\src\cognitia\abi\types.py`)

---

## 1. Schema Catalog

1. **Observation Envelope Schema:** Used for passive metadata and lifecycle events (documented in Section 2).
2. **Authorized Content Envelope Schema:** Used when the user explicitly triggers page or selected text analysis (documented in [`COGNITIA_CONTENT_PROTOCOL.md`](file:///E:/Thorium/COGNITIA_CONTENT_PROTOCOL.md)).

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
| `authorized_content_extracted` | User Context Action / Button | `mode`, `url`, `origin`, `title`, `extracted_text` (untrusted) |

---

## 3. URL Sanitization Policy

1. **Credential Scrubbing:** Removes `user:password@` prefixes.
2. **Query Parameter Redaction:** Automatically redacts `token`, `access_token`, `auth`, `password`, `session_id`, `apikey`, `key`, `sig`, `code`, replacing value with `[REDACTED]`.
3. **Origin-Only Fallback:** If configured with `UrlExposurePolicy::kOriginOnly`, only the scheme, host, and port are transmitted.
