#include "chrome/browser/privacy/third_party_classifier/third_party_classifier.h"
#include <cassert>
#include <iostream>

namespace lean_thorium {
namespace privacy {

void TestThirdPartyClassifier() {
  std::cout << "[TEST] ThirdPartyClassifier: Exact host matching..." << std::endl;
  assert(ThirdPartyClassifier::Classify("https://example.com/api", "https://example.com") == PartyContext::kFirstParty);

  std::cout << "[TEST] ThirdPartyClassifier: Same-site subdomain matching..." << std::endl;
  assert(ThirdPartyClassifier::Classify("https://api.example.com/v1", "https://example.com") == PartyContext::kSameSite);
  assert(ThirdPartyClassifier::Classify("https://cdn.example.co.uk/style.css", "https://example.co.uk") == PartyContext::kSameSite);

  std::cout << "[TEST] ThirdPartyClassifier: Cross-domain third-party..." << std::endl;
  assert(ThirdPartyClassifier::Classify("https://google-analytics.com/ga.js", "https://news.example.com") == PartyContext::kThirdParty);
  assert(ThirdPartyClassifier::Classify("https://doubleclick.net/ad", "https://news.co.uk") == PartyContext::kThirdParty);

  std::cout << "[TEST] ThirdPartyClassifier: Multi-part TLD extraction..." << std::endl;
  assert(ThirdPartyClassifier::GetDomain("sub.portal.gov.au") == "portal.gov.au");
  assert(ThirdPartyClassifier::GetDomain("api.tokyo.co.jp") == "tokyo.co.jp");

  std::cout << "[TEST] ThirdPartyClassifier: Localhost and IP addresses..." << std::endl;
  assert(ThirdPartyClassifier::GetDomain("127.0.0.1") == "127.0.0.1");
  assert(ThirdPartyClassifier::GetDomain("localhost") == "localhost");

  std::cout << "[PASS] ThirdPartyClassifier tests passed!" << std::endl;
}

}  // namespace privacy
}  // namespace lean_thorium