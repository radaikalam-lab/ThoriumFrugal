#ifndef CHROME_BROWSER_PRIVACY_PRIVACY_EVENTS_PRIVACY_EVENT_H_
#define CHROME_BROWSER_PRIVACY_PRIVACY_EVENTS_PRIVACY_EVENT_H_

#include <string>
#include <vector>
#include "chrome/browser/privacy/privacy_policy/privacy_types.h"

namespace lean_thorium {
namespace privacy {

struct PrivacyEvent {
  std::string event_id;         // Robust monotonic UUID/token (e.g., lt_priv_000001)
  std::string event_type;       // e.g. "privacy.request_blocked", "privacy.tracking_parameter_removed"
  std::string host;             // Target host
  std::string sanitized_url;    // Explicitly sanitized URL (zero credentials/tokens/secrets)
  std::string initiator_origin; // Initiating top-level origin
  PartyContext party_context = PartyContext::kFirstParty;
  std::string resource_type;
  TrackerClassification classification = TrackerClassification::kNotTracker;
  std::string rule_id;
  std::vector<std::string> stripped_parameters;
  std::string timestamp;        // ISO-8601 UTC
};

}  // namespace privacy
}  // namespace lean_thorium

#endif  // CHROME_BROWSER_PRIVACY_PRIVACY_EVENTS_PRIVACY_EVENT_H_