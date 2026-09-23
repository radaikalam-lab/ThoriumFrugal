#ifndef CHROME_BROWSER_PRIVACY_TRACKER_BLOCKER_TRACKER_BLOCKER_H_
#define CHROME_BROWSER_PRIVACY_TRACKER_BLOCKER_TRACKER_BLOCKER_H_

#include <string>
#include <memory>
#include "chrome/browser/privacy/privacy_policy/privacy_types.h"
#include "chrome/browser/privacy/tracker_blocker/rule_database.h"
#include "chrome/browser/privacy/third_party_classifier/third_party_classifier.h"

namespace lean_thorium {
namespace privacy {

class TrackerBlocker {
 public:
  explicit TrackerBlocker(std::shared_ptr<RuleDatabase> database = nullptr);
  ~TrackerBlocker() = default;

  // Evaluates whether a network request should be allowed or blocked
  PrivacyEvaluationResult EvaluateRequest(const std::string& target_url_str,
                                         const std::string& initiator_origin_str,
                                         ResourceType resource_type);

  // Directly sets or updates the underlying rule database
  void SetDatabase(std::shared_ptr<RuleDatabase> database);

  // Returns the active rule database
  std::shared_ptr<RuleDatabase> GetDatabase() const;

 private:
  std::shared_ptr<RuleDatabase> database_;
};

}  // namespace privacy
}  // namespace lean_thorium

#endif  // CHROME_BROWSER_PRIVACY_TRACKER_BLOCKER_TRACKER_BLOCKER_H_