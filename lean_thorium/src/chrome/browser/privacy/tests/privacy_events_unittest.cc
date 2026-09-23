#include "chrome/browser/privacy/privacy_events/privacy_event_dispatcher.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace lean_thorium {
namespace privacy {

TEST(PrivacyEventsTest, DispatchesEventsToRegisteredListeners) {
  PrivacyEventDispatcher dispatcher;
  std::vector<PrivacyEvent> received_events;

  dispatcher.AddListener([&received_events](const PrivacyEvent& ev) {
    received_events.push_back(ev);
  });

  PrivacyEvent ev1;
  ev1.event_id = "ev_1";
  ev1.event_type = "privacy.request_blocked";
  ev1.sanitized_url = "https://tracker.com/pixel";
  ev1.party_context = PartyContext::kThirdParty;

  dispatcher.DispatchEvent(ev1);

  EXPECT_EQ(received_events.size(), 1u);
  EXPECT_EQ(received_events[0].event_type, "privacy.request_blocked");
  EXPECT_EQ(received_events[0].sanitized_url, "https://tracker.com/pixel");
}

}  // namespace privacy
}  // namespace lean_thorium