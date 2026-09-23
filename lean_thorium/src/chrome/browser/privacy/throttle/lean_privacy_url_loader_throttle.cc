#include "chrome/browser/privacy/throttle/lean_privacy_url_loader_throttle.h"

#include "net/base/net_errors.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/mojom/url_response_head.mojom.h"

namespace lean_thorium {
namespace privacy {

std::unique_ptr<LeanPrivacyURLLoaderThrottle> LeanPrivacyURLLoaderThrottle::MaybeCreate() {
  if (!PrivacyPolicyManager::GetInstance()->IsPrivacyProtectionEnabled()) {
    return nullptr;
  }
  return std::make_unique<LeanPrivacyURLLoaderThrottle>();
}

LeanPrivacyURLLoaderThrottle::LeanPrivacyURLLoaderThrottle() = default;
LeanPrivacyURLLoaderThrottle::~LeanPrivacyURLLoaderThrottle() = default;

ResourceType LeanPrivacyURLLoaderThrottle::ConvertResourceType(int request_resource_type) {
  // Maps Chromium blink/network resource types to Privacy P1 ResourceType
  switch (request_resource_type) {
    case 0: return ResourceType::kMainFrame;
    case 1: return ResourceType::kSubFrame;
    case 2: return ResourceType::kStylesheet;
    case 3: return ResourceType::kScript;
    case 4: return ResourceType::kImage;
    case 5: return ResourceType::kFont;
    case 6: return ResourceType::kSubFrame;
    case 7: return ResourceType::kOther;
    case 8: return ResourceType::kMedia;
    case 9: return ResourceType::kWebSocket;
    case 10: return ResourceType::kPingBeacon;
    case 11: return ResourceType::kXhrFetch;
    default: return ResourceType::kOther;
  }
}

void LeanPrivacyURLLoaderThrottle::WillStartRequest(network::ResourceRequest* request, bool* defer) {
  if (!request) return;

  // 1. Query parameter sanitization
  FilterResult filter_res = PrivacyPolicyManager::GetInstance()->SanitizeUrl(request->url.spec());
  if (filter_res.parameters_removed) {
    request->url = GURL(filter_res.cleaned_url);
  }

  // 2. Extract Initiator
  std::string initiator_str = "";
  if (request->request_initiator.has_value()) {
    initiator_str = request->request_initiator->GetURL().spec();
  }

  // 3. Privacy Evaluation
  ResourceType res_type = ConvertResourceType(request->resource_type);
  PrivacyEvaluationResult result = PrivacyPolicyManager::GetInstance()->EvaluateRequest(
      request->url.spec(), initiator_str, res_type);

  // 4. Enforce Decision
  if (result.decision == PrivacyDecision::kBlock) {
    // Deny request deterministically inside Chromium network stack
    delegate_->CancelWithError(net::ERR_BLOCKED_BY_CLIENT, "Lean Thorium Privacy Blocked");
    return;
  }
}

void LeanPrivacyURLLoaderThrottle::WillRedirectRequest(
    net::RedirectInfo* redirect_info,
    const network::mojom::URLResponseHead& response_head,
    bool* defer,
    std::vector<std::string>* to_be_removed_headers,
    net::HttpRequestHeaders* modified_headers,
    net::HttpRequestHeaders* modified_cors_exempt_headers) {
  if (!redirect_info) return;

  // 1. Sanitize redirect target URL
  FilterResult filter_res = PrivacyPolicyManager::GetInstance()->SanitizeUrl(redirect_info->new_url.spec());
  if (filter_res.parameters_removed) {
    redirect_info->new_url = GURL(filter_res.cleaned_url);
  }

  // 2. Evaluate redirected URL
  std::string initiator_str = "";
  if (redirect_info->new_initiator.has_value()) {
    initiator_str = redirect_info->new_initiator->GetURL().spec();
  }

  PrivacyEvaluationResult result = PrivacyPolicyManager::GetInstance()->EvaluateRequest(
      redirect_info->new_url.spec(), initiator_str, ResourceType::kOther);

  if (result.decision == PrivacyDecision::kBlock) {
    delegate_->CancelWithError(net::ERR_BLOCKED_BY_CLIENT, "Lean Thorium Privacy Blocked Redirect");
    return;
  }
}

}  // namespace privacy
}  // namespace lean_thorium