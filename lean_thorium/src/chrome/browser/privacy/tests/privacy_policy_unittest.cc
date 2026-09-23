#include "chrome/browser/privacy/privacy_policy/privacy_policy_manager.h"
#include <cassert>
#include <iostream>

namespace lean_thorium {
namespace privacy {

void TestPrivacyPolicyManager() {
  std::cout << "[TEST] PrivacyPolicyManager: Standard block evaluation..." << std::endl;
  auto* mgr = PrivacyPolicyManager::GetInstance();
  mgr->GetTrackerBlocker().GetDatabase()->LoadFromText(
      "||google-analytics.com^$third-party\n"
      "||criteo.com^$third-party\n"
      "@@||cdnjs.cloudflare.com^\n");

  auto res1 = mgr->EvaluateRequest(
      "https://google-analytics.com/collect", "https://site.example.com", ResourceType::kXhrFetch);
  assert(res1.decision == PrivacyDecision::kBlock);
  assert(res1.classification == TrackerClassification::kKnownTracker);

  std::cout << "[TEST] PrivacyPolicyManager: Allow exception rule..." << std::endl;
  auto res2 = mgr->EvaluateRequest(
      "https://cdnjs.cloudflare.com/lib.js", "https://site.example.com", ResourceType::kScript);
  assert(res2.decision == PrivacyDecision::kAllow);

  std::cout << "[TEST] PrivacyPolicyManager: Per-site exception..." << std::endl;
  mgr->SetSiteException("site.example.com", SitePolicyException::kAllowAll);
  auto res3 = mgr->EvaluateRequest(
      "https://google-analytics.com/collect", "https://site.example.com", ResourceType::kXhrFetch);
  assert(res3.decision == PrivacyDecision::kAllow);
  assert(res3.exception_applied == true);

  std::cout << "[TEST] PrivacyPolicyManager: Telemetry URL sanitization..." << std::endl;
  std::string dirty_url = "https://user:password123@example.com/api?token=secret99&id=42";
  std::string clean_url = PrivacyPolicyManager::SanitizeUrlForTelemetry(dirty_url);
  assert(clean_url.find("password") == std::string::npos);
  assert(clean_url.find("token=secret99") == std::string::npos);
  assert(clean_url.find("token=[REDACTED]") != std::string::npos);
  assert(clean_url.find("id=42") != std::string::npos);

  std::cout << "[TEST] PrivacyPolicyManager: Event ID monotonicity..." << std::endl;
  std::string id1 = mgr->GenerateEventId();
  std::string id2 = mgr->GenerateEventId();
  assert(id1 != id2);
  assert(id1.find("lt_priv_") != std::string::npos);

  std::cout << "[PASS] PrivacyPolicyManager tests passed!" << std::endl;
}

}  // namespace privacy
}  // namespace lean_thorium