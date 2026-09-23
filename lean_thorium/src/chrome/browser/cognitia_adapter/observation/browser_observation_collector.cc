#include "chrome/browser/cognitia_adapter/observation/browser_observation_collector.h"

#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/web_contents.h"
#include "chrome/browser/cognitia_adapter/protocol/observation_envelope.h"
#include "chrome/browser/cognitia_adapter/protocol/url_sanitizer.h"

namespace cognitia {

BrowserObservationCollector::BrowserObservationCollector(
    std::unique_ptr<CognitiaTransport> transport)
    : transport_(std::move(transport)) {}

BrowserObservationCollector::~BrowserObservationCollector() = default;

void BrowserObservationCollector::EmitEvent(BrowserEventType event_type,
                                            int window_id,
                                            int tab_id,
                                            bool is_active_tab,
                                            const GURL& url,
                                            const std::u16string& title,
                                            bool is_incognito) {
  if (!is_enabled_ || !transport_) {
    return;
  }

  std::string sanitized_url = UrlSanitizer::SanitizeUrl(url, url_policy_);
  std::string origin = url.DeprecatedGetOriginAsURL().spec();

  ObservationEnvelope envelope(event_type, window_id, tab_id, is_active_tab,
                              sanitized_url, origin, title, is_incognito);

  // Dispatch strictly serialized JSON asynchronously to transport
  transport_->SendObservation(envelope.ToJson());
}

void BrowserObservationCollector::OnTabStripModelChanged(
    TabStripModel* tab_strip_model,
    const TabStripModelChange& change,
    const TabStripSelectionChange& selection) {
  if (!is_enabled_) return;

  if (change.type() == TabStripModelChange::kInserted) {
    EmitEvent(BrowserEventType::kTabCreated, 1, selection.new_model.active_index(),
              true, GURL("about:blank"), u"New Tab", false);
  } else if (change.type() == TabStripModelChange::kRemoved) {
    EmitEvent(BrowserEventType::kTabClosed, 1, selection.old_model.active_index(),
              false, GURL(), u"", false);
  }

  if (selection.active_tab_changed()) {
    EmitEvent(BrowserEventType::kTabActivated, 1, selection.new_model.active_index(),
              true, GURL(), u"", false);
  }
}

void BrowserObservationCollector::DidStartNavigation(
    content::NavigationHandle* navigation_handle) {
  if (!is_enabled_ || !navigation_handle->IsInPrimaryMainFrame()) return;

  EmitEvent(BrowserEventType::kNavigationStarted, 1, 0, true,
            navigation_handle->GetURL(), u"", false);
}

void BrowserObservationCollector::DidFinishNavigation(
    content::NavigationHandle* navigation_handle) {
  if (!is_enabled_ || !navigation_handle->IsInPrimaryMainFrame() ||
      !navigation_handle->HasCommitted()) return;

  EmitEvent(BrowserEventType::kNavigationCommitted, 1, 0, true,
            navigation_handle->GetURL(), u"", false);
}

void BrowserObservationCollector::TitleWasSet(content::NavigationEntry* entry) {
  if (!is_enabled_ || !web_contents()) return;

  EmitEvent(BrowserEventType::kPageTitleChanged, 1, 0, true,
            web_contents()->GetVisibleURL(), web_contents()->GetTitle(), false);
}

}  // namespace cognitia
