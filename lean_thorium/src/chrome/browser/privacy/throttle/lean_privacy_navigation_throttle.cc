#include "chrome/browser/privacy/throttle/lean_privacy_navigation_throttle.h"

#include "content/public/browser/navigation_handle.h"
#include "net/base/net_errors.h"

namespace lean_thorium {
namespace privacy {

std::unique_ptr<content::NavigationThrottle>
LeanPrivacyNavigationThrottle::MaybeCreateThrottleFor(
    content::NavigationHandle* navigation_handle) {
  if (!PrivacyPolicyManager::GetInstance()->IsPrivacyProtectionEnabled()) {
    return nullptr;
  }
  return std::make_unique<LeanPrivacyNavigationThrottle>(navigation_handle);
}

LeanPrivacyNavigationThrottle::LeanPrivacyNavigationThrottle(
    content::NavigationHandle* handle)
    : content::NavigationThrottle(handle) {}

LeanPrivacyNavigationThrottle::~LeanPrivacyNavigationThrottle() = default;

const char* LeanPrivacyNavigationThrottle::GetNameForLogging() {
  return "LeanPrivacyNavigationThrottle";
}

content::NavigationThrottle::ThrottleCheckResult
LeanPrivacyNavigationThrottle::CheckPrivacyPolicy() {
  content::NavigationHandle* handle = navigation_handle();
  if (!handle) return PROCEED;

  const GURL& url = handle->GetURL();
  std::string initiator_str = "";
  if (handle->GetInitiatorOrigin().has_value()) {
    initiator_str = handle->GetInitiatorOrigin()->GetURL().spec();
  }

  ResourceType res_type = handle->IsInMainFrame() ? ResourceType::kMainFrame : ResourceType::kSubFrame;
  PrivacyEvaluationResult result = PrivacyPolicyManager::GetInstance()->EvaluateRequest(
      url.spec(), initiator_str, res_type);

  if (result.decision == PrivacyDecision::kBlock) {
    return ThrottleCheckResult(CANCEL, net::ERR_BLOCKED_BY_CLIENT, "Lean Thorium Privacy Blocked");
  }

  return PROCEED;
}

content::NavigationThrottle::ThrottleCheckResult
LeanPrivacyNavigationThrottle::WillStartRequest() {
  return CheckPrivacyPolicy();
}

content::NavigationThrottle::ThrottleCheckResult
LeanPrivacyNavigationThrottle::WillRedirectRequest() {
  return CheckPrivacyPolicy();
}

}  // namespace privacy
}  // namespace lean_thorium