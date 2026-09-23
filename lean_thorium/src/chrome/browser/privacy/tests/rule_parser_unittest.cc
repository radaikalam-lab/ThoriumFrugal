#include "chrome/browser/privacy/tracker_blocker/rule_parser.h"
#include <cassert>
#include <iostream>

namespace lean_thorium {
namespace privacy {

void TestRuleParser() {
  std::cout << "[TEST] RuleParser: Standard domain block rule..." << std::endl;
  auto r1 = RuleParser::ParseLine("||google-analytics.com^$third-party,script");
  assert(r1.is_valid);
  assert(r1.rule_type == RuleType::kBlock);
  assert(r1.domain_pattern == "google-analytics.com");
  assert(r1.match_subdomains == true);
  assert(r1.require_third_party == true);
  assert(RuleParser::MatchesResourceType(ResourceType::kScript, r1.resource_type_mask));
  assert(!RuleParser::MatchesResourceType(ResourceType::kImage, r1.resource_type_mask));

  std::cout << "[TEST] RuleParser: Allow exception rule..." << std::endl;
  auto r2 = RuleParser::ParseLine("@@||cdnjs.cloudflare.com^");
  assert(r2.is_valid);
  assert(r2.rule_type == RuleType::kAllow);
  assert(r2.domain_pattern == "cdnjs.cloudflare.com");

  std::cout << "[TEST] RuleParser: Path rule..." << std::endl;
  auto r3 = RuleParser::ParseLine("||facebook.com/tr^$third-party");
  assert(r3.is_valid);
  assert(r3.domain_pattern == "facebook.com");
  assert(r3.path_pattern == "/tr");

  std::cout << "[TEST] RuleParser: Malformed and comments..." << std::endl;
  auto r4 = RuleParser::ParseLine("! Just a comment");
  assert(!r4.is_valid);
  auto r5 = RuleParser::ParseLine("   ");
  assert(!r5.is_valid);

  std::cout << "[TEST] RuleParser: Oversized rule protection..." << std::endl;
  std::string huge_line = "||" + std::string(2000, 'x') + "^";
  auto r6 = RuleParser::ParseLine(huge_line);
  assert(!r6.is_valid);

  std::cout << "[PASS] RuleParser tests passed!" << std::endl;
}

}  // namespace privacy
}  // namespace lean_thorium