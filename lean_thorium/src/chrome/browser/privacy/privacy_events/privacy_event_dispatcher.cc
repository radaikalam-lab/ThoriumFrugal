#include "chrome/browser/privacy/privacy_events/privacy_event_dispatcher.h"

namespace lean_thorium {
namespace privacy {

void PrivacyEventDispatcher::AddListener(PrivacyEventListener listener) {
  std::lock_guard<std::mutex> lock(mutex_);
  listeners_.push_back(std::move(listener));
}

void PrivacyEventDispatcher::ClearListeners() {
  std::lock_guard<std::mutex> lock(mutex_);
  listeners_.clear();
}

void PrivacyEventDispatcher::DispatchEvent(const PrivacyEvent& event) {
  std::vector<PrivacyEventListener> local_listeners;
  {
    std::lock_guard<std::mutex> lock(mutex_);
    local_listeners = listeners_;
  }

  for (const auto& listener : local_listeners) {
    if (listener) {
      listener(event);
    }
  }
}

}  // namespace privacy
}  // namespace lean_thorium