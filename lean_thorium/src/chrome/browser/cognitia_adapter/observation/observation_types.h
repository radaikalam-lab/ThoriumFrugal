#ifndef CHROME_BROWSER_COGNITIA_ADAPTER_OBSERVATION_OBSERVATION_TYPES_H_
#define CHROME_BROWSER_COGNITIA_ADAPTER_OBSERVATION_OBSERVATION_TYPES_H_

#include <string>

namespace cognitia {

enum class BrowserEventType {
  kBrowserStarted,
  kTabCreated,
  kTabActivated,
  kTabClosed,
  kNavigationStarted,
  kNavigationCommitted,
  kNavigationFinished,
  kPageTitleChanged,
  kPageLoadCompleted,
  kTextSelectionChanged
};

inline const char* BrowserEventTypeToString(BrowserEventType type) {
  switch (type) {
    case BrowserEventType::kBrowserStarted:
      return "browser_started";
    case BrowserEventType::kTabCreated:
      return "tab_created";
    case BrowserEventType::kTabActivated:
      return "tab_activated";
    case BrowserEventType::kTabClosed:
      return "tab_closed";
    case BrowserEventType::kNavigationStarted:
      return "navigation_started";
    case BrowserEventType::kNavigationCommitted:
      return "navigation_committed";
    case BrowserEventType::kNavigationFinished:
      return "navigation_finished";
    case BrowserEventType::kPageTitleChanged:
      return "page_title_changed";
    case BrowserEventType::kPageLoadCompleted:
      return "page_load_completed";
    case BrowserEventType::kTextSelectionChanged:
      return "text_selection_changed";
  }
  return "unknown_event";
}

enum class UrlExposurePolicy {
  kFullUrlSanitized,  // Full URL with credentials/tokens scrubbed
  kOriginOnly,        // Scheme + Host + Port only (e.g., https://example.com)
  kOmitted            // URL completely omitted from observation
};

}  // namespace cognitia

#endif  // CHROME_BROWSER_COGNITIA_ADAPTER_OBSERVATION_OBSERVATION_TYPES_H_
