#include "chrome/browser/privacy/tracker_blocker/tracker_blocker.h"
#include "chrome/browser/privacy/tracker_blocker/rule_parser.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace lean_thorium {
namespace privacy {

class TrackerBlockerTest : public testing::Test {
 protected:
  void SetUp() override {
    blocker_ = std::make_unique<TrackerBlocker>();
    auto db = blocker_->GetDatabase();
    db->LoadFromText(
        "||google-analytics.com^$third-party\n"
        "||doubleclick.net^$third-party\n"
        "||facebook.com/tr^$third-party\n"
        "@@||cdnjs.cloudflare.com^\n"
        "||tracker.example.com^$script\n");
  }

  std::unique_ptr<TrackerBlocker> blocker_;
};

TEST_F(TrackerBlockerTest, BlocksKnownThirdPartyTracker) {
  auto result = blocker_->EvaluateRequest(
      "https://www.google-analytics.com/analytics.js",
      "https://news.example.com",
      ResourceType::kScript);
  EXPECT_EQ(result.decision, PrivacyDecision::kBlock);
  EXPECT_EQ(result.classification, TrackerClassification::kKnownTracker);
  EXPECT_EQ(result.party_context, PartyContext::kThirdParty);
}

TEST_F(TrackerBlockerTest, AllowsFirstPartyTracker) {
  // First-party request to own domain should not be blocked by $third-party rule
  auto result = blocker_->EvaluateRequest(
      "https://google-analytics.com/dashboard",
      "https://google-analytics.com",
      ResourceType::kMainFrame);
  EXPECT_EQ(result.decision, PrivacyDecision::kAllow);
}

TEST_F(TrackerBlockerTest, AllowsUnknownThirdParty) {
  auto result = blocker_->EvaluateRequest(
      "https://images.unsplash.com/photo.jpg",
      "https://news.example.com",
      ResourceType::kImage);
  EXPECT_EQ(result.decision, PrivacyDecision::kAllow);
  EXPECT_EQ(result.classification, TrackerClassification::kNotTracker);
}

TEST_F(TrackerBlockerTest, RespectsAllowExceptionRule) {
  auto result = blocker_->EvaluateRequest(
      "https://cdnjs.cloudflare.com/ajax/libs/react/18.0.0/react.min.js",
      "https://app.example.com",
      ResourceType::kScript);
  EXPECT_EQ(result.decision, PrivacyDecision::kAllow);
  EXPECT_NE(result.matched_rule_id, "");
}

TEST_F(TrackerBlockerTest, MatchesSubdomainsCorrectly) {
  auto result = blocker_->EvaluateRequest(
      "https://ad.doubleclick.net/pixel",
      "https://shopping.example.com",
      ResourceType::kImage);
  EXPECT_EQ(result.decision, PrivacyDecision::kBlock);
  EXPECT_EQ(result.classification, TrackerClassification::kKnownTracker);
}

}  // namespace privacy
}  // namespace lean_thorium