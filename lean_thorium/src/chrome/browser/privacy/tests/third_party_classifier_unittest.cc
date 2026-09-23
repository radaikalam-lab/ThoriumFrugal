#include "chrome/browser/privacy/third_party_classifier/third_party_classifier.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace lean_thorium {
namespace privacy {

TEST(ThirdPartyClassifierTest, ClassifiesExactHostMatchAsFirstParty) {
  EXPECT_EQ(ThirdPartyClassifier::Classify("https://example.com/api", "https://example.com"),
            PartyContext::kFirstParty);
}

TEST(ThirdPartyClassifierTest, ClassifiesSubdomainAsSameSite) {
  EXPECT_EQ(ThirdPartyClassifier::Classify("https://api.example.com/v1", "https://example.com"),
            PartyContext::kSameSite);
  EXPECT_EQ(ThirdPartyClassifier::Classify("https://cdn.example.com/app.js", "https://app.example.com"),
            PartyContext::kSameSite);
}

TEST(ThirdPartyClassifierTest, ClassifiesDifferentDomainAsThirdParty) {
  EXPECT_EQ(ThirdPartyClassifier::Classify("https://google-analytics.com/ga.js", "https://example.com"),
            PartyContext::kThirdParty);
  EXPECT_EQ(ThirdPartyClassifier::Classify("https://doubleclick.net/ad", "https://news.co.uk"),
            PartyContext::kThirdParty);
}

TEST(ThirdPartyClassifierTest, HandlesTwoPartTldsCorrectly) {
  EXPECT_EQ(ThirdPartyClassifier::GetDomain("sub.example.co.uk"), "example.co.uk");
  EXPECT_EQ(ThirdPartyClassifier::GetDomain("api.portal.gov.au"), "portal.gov.au");
  EXPECT_EQ(ThirdPartyClassifier::Classify("https://cdn.example.co.uk/style.css", "https://example.co.uk"),
            PartyContext::kSameSite);
  EXPECT_EQ(ThirdPartyClassifier::Classify("https://tracker.co.uk/pixel", "https://example.co.uk"),
            PartyContext::kThirdParty);
}

}  // namespace privacy
}  // namespace lean_thorium