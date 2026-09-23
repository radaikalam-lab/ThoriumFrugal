#ifndef CHROME_BROWSER_PRIVACY_THROTTLE_LEAN_PRIVACY_URL_LOADER_THROTTLE_H_
#define CHROME_BROWSER_PRIVACY_THROTTLE_LEAN_PRIVACY_URL_LOADER_THROTTLE_H_

#include <memory>
#include <string>
#include <vector>
#include "third_party/blink/public/common/loader/url_loader_throttle.h"
#include "chrome/browser/privacy/privacy_policy/privacy_policy_manager.h"

namespace lean_thorium {
namespace privacy {

// Intercepts and evaluates subresource and network requests before network dispatch
class LeanPrivacyURLLoaderThrottle : public blink::URLLoaderThrottle {
 public:
  static std::unique_ptr<LeanPrivacyURLLoaderThrottle> MaybeCreate();

  LeanPrivacyURLLoaderThrottle();
  ~LeanPrivacyURLLoaderThrottle() override;

  // blink::URLLoaderThrottle implementation:
  void WillStartRequest(network::ResourceRequest* request, bool* defer) override;
  void WillRedirectRequest(
      net::RedirectInfo* redirect_info,
      const network::mojom::URLResponseHead& response_head,
      bool* defer,
      std::vector<std::string>* to_be_removed_headers,
      net::HttpRequestHeaders* modified_headers,
      net::HttpRequestHeaders* modified_cors_exempt_headers) override;

 private:
  ResourceType ConvertResourceType(int request_resource_type);
};

}  // namespace privacy
}  // namespace lean_thorium

#endif  // CHROME_BROWSER_PRIVACY_THROTTLE_LEAN_PRIVACY_URL_LOADER_THROTTLE_H_