#include "chrome/browser/privacy/privacy_policy/privacy_policy_manager.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace lean_thorium {
namespace privacy {

class PrivacyPolicyManagerTest : public testing::Test {
 protected:
  void SetUp() override {
    manager_ = std::make_unique<PrivacyPolicyManager>();
    manager_->GetTrackerBlocker().GetDatabase()->LoadFromText(
        "||google-analytics.com^$third-party\n"
        "||criteo.com^$third-party\n");
  }

  std::unique_ptr<PrivacyPolicyManager> manager_;
};

TEST_F(PrivacyPolicyManagerTest, EvaluatesStandardThirdPartyTracker) {
  auto result = manager_->EvaluateRequest(
      "https://google-analytics.com/collect",
      "https://site.example.com",
      ResourceType::kXhrFetch);
  EXPECT_EQ(result.decision, PrivacyDecision::kBlock);
  EXPECT_EQ(result.classification, TrackerClassification::kKnownTracker);
}

TEST_F(PrivacyPolicyManagerTest, HonorsPerSiteAllowException) {
  manager_->SetSiteException("site.example.com", SitePolicyException::kAllowAll);
  auto result = manager_->EvaluateRequest(
      "https://google-analytics.com/collect",
      "https://site.example.com",
      ResourceType::kXhrFetch);
  EXPECT_EQ(result.decision, PrivacyDecision::kAllow);
  EXPECT_TRUE(result.exception_applied);
}

TEST_F(PrivacyPolicyManagerTest, GlobalToggleDisablesBlocking) {
  manager_->SetPrivacyProtectionEnabled(false);
  auto result = manager_->EvaluateRequest(
      "https://google-analytics.com/collect",
      "https://site.example.com",
      ResourceType::kXhrFetch);
  EXPECT_EQ(result.decision, PrivacyDecision::kAllow);
}

}  // namespace privacy
}  // namespace lean_thorium