#ifndef CHROME_BROWSER_PRIVACY_TRACKER_BLOCKER_RULE_PARSER_H_
#define CHROME_BROWSER_PRIVACY_TRACKER_BLOCKER_RULE_PARSER_H_

#include <string>
#include <vector>
#include <memory>
#include "chrome/browser/privacy/privacy_policy/privacy_types.h"

namespace lean_thorium {
namespace privacy {

enum class RuleType {
  kBlock = 0,
  kAllow = 1,
};

struct ParsedRule {
  std::string rule_id;
  RuleType rule_type = RuleType::kBlock;
  std::string domain_pattern;
  std::string path_pattern;
  bool match_subdomains = true;
  bool require_third_party = false;
  bool require_first_party = false;
  unsigned int resource_type_mask = 0xFFFFFFFF;  // Match all by default
  std::string original_rule_text;
  bool is_valid = false;
};

class RuleParser {
 public:
  // Parses a single line rule in standard filter-list syntax (e.g., ||example.com^$third-party)
  static ParsedRule ParseLine(const std::string& line, const std::string& rule_id_prefix = "rule_");

  // Parses a list of rules from text content
  static std::vector<ParsedRule> ParseText(const std::string& text_content);

  // Helper to parse resource types from comma-separated options (e.g. script,image)
  static unsigned int ParseResourceTypeMask(const std::string& options_str);

  // Checks if a given resource type matches the mask
  static bool MatchesResourceType(ResourceType type, unsigned int mask);
};

}  // namespace privacy
}  // namespace lean_thorium

#endif  // CHROME_BROWSER_PRIVACY_TRACKER_BLOCKER_RULE_PARSER_H_