#ifndef CHROME_BROWSER_PRIVACY_THROTTLE_LEAN_PRIVACY_NAVIGATION_THROTTLE_H_
#define CHROME_BROWSER_PRIVACY_THROTTLE_LEAN_PRIVACY_NAVIGATION_THROTTLE_H_

#include <memory>
#include "content/public/browser/navigation_throttle.h"
#include "chrome/browser/privacy/privacy_policy/privacy_policy_manager.h"

namespace content {
class NavigationHandle;
}

namespace lean_thorium {
namespace privacy {

// Navigation throttle to sanitize and evaluate top-level and subframe navigations
class LeanPrivacyNavigationThrottle : public content::NavigationThrottle {
 public:
  static std::unique_ptr<content::NavigationThrottle> MaybeCreateThrottleFor(
      content::NavigationHandle* navigation_handle);

  explicit LeanPrivacyNavigationThrottle(content::NavigationHandle* handle);
  ~LeanPrivacyNavigationThrottle() override;

  // content::NavigationThrottle implementation:
  ThrottleCheckResult WillStartRequest() override;
  ThrottleCheckResult WillRedirectRequest() override;
  const char* GetNameForLogging() override;

 private:
  ThrottleCheckResult CheckPrivacyPolicy();
};

}  // namespace privacy
}  // namespace lean_thorium

#endif  // CHROME_BROWSER_PRIVACY_THROTTLE_LEAN_PRIVACY_NAVIGATION_THROTTLE_H_