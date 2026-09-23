#ifndef CHROME_BROWSER_PRIVACY_THIRD_PARTY_CLASSIFIER_THIRD_PARTY_CLASSIFIER_H_
#define CHROME_BROWSER_PRIVACY_THIRD_PARTY_CLASSIFIER_THIRD_PARTY_CLASSIFIER_H_

#include <string>
#include "chrome/browser/privacy/privacy_policy/privacy_types.h"

namespace lean_thorium {
namespace privacy {

class ThirdPartyClassifier {
 public:
  // Classifies the relationship between the target URL and the initiating origin/document
  static PartyContext Classify(const std::string& target_url_str,
                               const std::string& initiator_origin_str);

  // Extracts the registrable domain (eTLD+1) from a hostname
  static std::string GetDomain(const std::string& hostname);

  // Extracts hostname from a URL string
  static std::string ExtractHostname(const std::string& url_str);

 private:
  static bool IsKnownTwoPartTld(const std::string& suffix);
};

}  // namespace privacy
}  // namespace lean_thorium

#endif  // CHROME_BROWSER_PRIVACY_THIRD_PARTY_CLASSIFIER_THIRD_PARTY_CLASSIFIER_H_