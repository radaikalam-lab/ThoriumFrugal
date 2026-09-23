#ifndef CHROME_BROWSER_PRIVACY_PRIVACY_EVENTS_PRIVACY_EVENT_DISPATCHER_H_
#define CHROME_BROWSER_PRIVACY_PRIVACY_EVENTS_PRIVACY_EVENT_DISPATCHER_H_

#include <functional>
#include <vector>
#include <mutex>
#include "chrome/browser/privacy/privacy_events/privacy_event.h"

namespace lean_thorium {
namespace privacy {

using PrivacyEventListener = std::function<void(const PrivacyEvent&)>;

class PrivacyEventDispatcher {
 public:
  PrivacyEventDispatcher() = default;
  ~PrivacyEventDispatcher() = default;

  // Dispatches a privacy event to all registered local listeners
  void DispatchEvent(const PrivacyEvent& event);

  // Registers a local event listener (e.g. for telemetry, UI indicators, or Cognitia observation)
  void AddListener(PrivacyEventListener listener);

  // Clears registered listeners
  void ClearListeners();

 private:
  std::mutex mutex_;
  std::vector<PrivacyEventListener> listeners_;
};

}  // namespace privacy
}  // namespace lean_thorium

#endif  // CHROME_BROWSER_PRIVACY_PRIVACY_EVENTS_PRIVACY_EVENT_DISPATCHER_H_