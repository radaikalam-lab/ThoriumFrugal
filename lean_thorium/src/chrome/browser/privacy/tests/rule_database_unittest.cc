#include "chrome/browser/privacy/tracker_blocker/rule_database.h"
#include <cassert>
#include <iostream>

namespace lean_thorium {
namespace privacy {

void TestRuleDatabase() {
  std::cout << "[TEST] RuleDatabase: Indexed lookup and candidate generation..." << std::endl;
  RuleDatabase db;
  auto candidates = RuleDatabase::GenerateCandidateDomains("ads.sub.doubleclick.net");
  assert(candidates.size() == 4);
  assert(candidates[0] == "ads.sub.doubleclick.net");
  assert(candidates[1] == "sub.doubleclick.net");
  assert(candidates[2] == "doubleclick.net");
  assert(candidates[3] == "net");

  db.LoadFromText(
      "||doubleclick.net^$third-party\n"
      "||google-analytics.com/ga.js^$third-party\n"
      "@@||cdnjs.cloudflare.com^\n");

  assert(db.RuleCount() == 3);
  assert(db.IsKnownTrackerHost("ad.doubleclick.net"));
  assert(db.IsKnownTrackerHost("doubleclick.net"));
  assert(!db.IsKnownTrackerHost("example.com"));

  auto block_rules = db.FindMatchingBlockRules("ad.doubleclick.net", "/pixel");
  assert(block_rules.size() == 1);
  assert(block_rules[0].domain_pattern == "doubleclick.net");

  auto allow_rules = db.FindMatchingAllowRules("cdnjs.cloudflare.com", "/react.js");
  assert(allow_rules.size() == 1);
  assert(allow_rules[0].domain_pattern == "cdnjs.cloudflare.com");

  std::cout << "[PASS] RuleDatabase tests passed!" << std::endl;
}

}  // namespace privacy
}  // namespace lean_thorium