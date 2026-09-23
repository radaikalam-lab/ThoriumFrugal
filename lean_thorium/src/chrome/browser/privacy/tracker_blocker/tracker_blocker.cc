#include "chrome/browser/privacy/tracker_blocker/tracker_blocker.h"

namespace lean_thorium {
namespace privacy {

namespace {

std::string ExtractPath(const std::string& url_str) {
  size_t scheme_pos = url_str.find("://");
  size_t start = (scheme_pos != std::string::npos) ? scheme_pos + 3 : 0;
  size_t slash_pos = url_str.find('/', start);
  if (slash_pos == std::string::npos) return "/";

  size_t query_pos = url_str.find_first_of("?#", slash_pos);
  if (query_pos == std::string::npos) {
    return url_str.substr(slash_pos);
  }
  return url_str.substr(slash_pos, query_pos - slash_pos);
}

}  // namespace

TrackerBlocker::TrackerBlocker(std::shared_ptr<RuleDatabase> database)
    : database_(database ? database : std::make_shared<RuleDatabase>()) {}

void TrackerBlocker::SetDatabase(std::shared_ptr<RuleDatabase> database) {
  database_ = database;
}

std::shared_ptr<RuleDatabase> TrackerBlocker::GetDatabase() const {
  return database_;
}

PrivacyEvaluationResult TrackerBlocker::EvaluateRequest(const std::string& target_url_str,
                                                        const std::string& initiator_origin_str,
                                                        ResourceType resource_type) {
  PrivacyEvaluationResult result;
  result.decision = PrivacyDecision::kAllow;
  result.party_context = ThirdPartyClassifier::Classify(target_url_str, initiator_origin_str);
  result.classification = TrackerClassification::kNotTracker;

  std::string host = ThirdPartyClassifier::ExtractHostname(target_url_str);
  std::string path = ExtractPath(target_url_str);

  if (host.empty() || !database_) {
    return result;
  }

  // Check if host is a known tracker
  if (database_->IsKnownTrackerHost(host)) {
    result.classification = TrackerClassification::kKnownTracker;
  }

  // 1. Evaluate Allow Rules First (Allow exceptions override block rules)
  std::vector<ParsedRule> allow_rules = database_->FindMatchingAllowRules(host, path);
  for (const auto& rule : allow_rules) {
    if (!RuleParser::MatchesResourceType(resource_type, rule.resource_type_mask)) {
      continue;
    }
    if (rule.require_third_party && result.party_context != PartyContext::kThirdParty) {
      continue;
    }
    if (rule.require_first_party && result.party_context != PartyContext::kFirstParty &&
        result.party_context != PartyContext::kSameSite) {
      continue;
    }

    // Match found: explicitly allowed
    result.decision = PrivacyDecision::kAllow;
    result.matched_rule_id = rule.rule_id;
    result.reason = "Matched allow rule: " + rule.original_rule_text;
    return result;
  }

  // 2. Evaluate Block Rules
  std::vector<ParsedRule> block_rules = database_->FindMatchingBlockRules(host, path);
  for (const auto& rule : block_rules) {
    if (!RuleParser::MatchesResourceType(resource_type, rule.resource_type_mask)) {
      continue;
    }
    if (rule.require_third_party && result.party_context != PartyContext::kThirdParty) {
      continue;
    }
    if (rule.require_first_party && result.party_context != PartyContext::kFirstParty &&
        result.party_context != PartyContext::kSameSite) {
      continue;
    }

    // Match found: Block rule applies
    result.classification = TrackerClassification::kKnownTracker;
    // Conservative P1 Policy: Known Tracker + Third Party -> BLOCK
    // Known Tracker + First Party -> ALLOW (or configurable)
    if (result.party_context == PartyContext::kThirdParty || !rule.require_third_party) {
      result.decision = PrivacyDecision::kBlock;
      result.matched_rule_id = rule.rule_id;
      result.reason = "Matched block rule: " + rule.original_rule_text;
      return result;
    }
  }

  return result;
}

}  // namespace privacy
}  // namespace lean_thorium