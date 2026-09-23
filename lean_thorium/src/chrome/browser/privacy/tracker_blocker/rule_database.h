#ifndef CHROME_BROWSER_PRIVACY_TRACKER_BLOCKER_RULE_DATABASE_H_
#define CHROME_BROWSER_PRIVACY_TRACKER_BLOCKER_RULE_DATABASE_H_

#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>
#include "chrome/browser/privacy/tracker_blocker/rule_parser.h"

namespace lean_thorium {
namespace privacy {

class RuleDatabase {
 public:
  RuleDatabase() = default;
  ~RuleDatabase() = default;

  // Adds a parsed rule to the database
  bool AddRule(const ParsedRule& rule);

  // Loads rules from raw filter list text
  size_t LoadFromText(const std::string& text_content);

  // Finds all matching allow rules for a host and path
  std::vector<ParsedRule> FindMatchingAllowRules(const std::string& host, const std::string& path) const;

  // Finds all matching block rules for a host and path
  std::vector<ParsedRule> FindMatchingBlockRules(const std::string& host, const std::string& path) const;

  // Checks if a host is directly indexed as a known tracker domain
  bool IsKnownTrackerHost(const std::string& host) const;

  size_t RuleCount() const;
  void Clear();

 private:
  bool MatchesPattern(const ParsedRule& rule, const std::string& host, const std::string& path) const;

  mutable std::mutex mutex_;
  std::unordered_map<std::string, std::vector<ParsedRule>> allow_rules_by_domain_;
  std::unordered_map<std::string, std::vector<ParsedRule>> block_rules_by_domain_;
  std::vector<ParsedRule> generic_allow_rules_;
  std::vector<ParsedRule> generic_block_rules_;
  size_t total_rules_ = 0;
};

}  // namespace privacy
}  // namespace lean_thorium

#endif  // CHROME_BROWSER_PRIVACY_TRACKER_BLOCKER_RULE_DATABASE_H_