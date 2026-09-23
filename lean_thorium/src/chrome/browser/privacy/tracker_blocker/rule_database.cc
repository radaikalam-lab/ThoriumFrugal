#include "chrome/browser/privacy/tracker_blocker/rule_database.h"

#include <algorithm>

namespace lean_thorium {
namespace privacy {

namespace {

std::string ToLower(std::string str) {
  std::transform(str.begin(), str.end(), str.begin(), [](unsigned char c) {
    return std::tolower(c);
  });
  return str;
}

}  // namespace

bool RuleDatabase::AddRule(const ParsedRule& rule) {
  if (!rule.is_valid) return false;

  std::lock_guard<std::mutex> lock(mutex_);
  if (total_rules_ >= 50000) {
    return false;  // Memory bound protection
  }

  if (rule.rule_type == RuleType::kAllow) {
    if (!rule.domain_pattern.empty()) {
      allow_rules_by_domain_[rule.domain_pattern].push_back(rule);
    } else {
      generic_allow_rules_.push_back(rule);
    }
  } else {
    if (!rule.domain_pattern.empty()) {
      block_rules_by_domain_[rule.domain_pattern].push_back(rule);
    } else {
      generic_block_rules_.push_back(rule);
    }
  }

  total_rules_++;
  return true;
}

size_t RuleDatabase::LoadFromText(const std::string& text_content) {
  std::vector<ParsedRule> parsed = RuleParser::ParseText(text_content);
  size_t loaded = 0;
  for (const auto& r : parsed) {
    if (AddRule(r)) {
      loaded++;
    }
  }
  return loaded;
}

bool RuleDatabase::MatchesPattern(const ParsedRule& rule,
                                  const std::string& host,
                                  const std::string& path) const {
  std::string lower_host = ToLower(host);
  std::string lower_pattern = ToLower(rule.domain_pattern);

  // Exact domain match
  bool domain_matched = false;
  if (lower_host == lower_pattern) {
    domain_matched = true;
  } else if (rule.match_subdomains) {
    // Subdomain match (e.g., host="ad.tracker.com", pattern="tracker.com")
    if (lower_host.length() > lower_pattern.length() + 1 &&
        lower_host.rfind("." + lower_pattern) == (lower_host.length() - lower_pattern.length() - 1)) {
      domain_matched = true;
    }
  }

  if (!domain_matched) {
    return false;
  }

  // Path prefix match
  if (!rule.path_pattern.empty()) {
    if (path.find(rule.path_pattern) == std::string::npos) {
      return false;
    }
  }

  return true;
}

std::vector<ParsedRule> RuleDatabase::FindMatchingAllowRules(const std::string& host,
                                                             const std::string& path) const {
  std::lock_guard<std::mutex> lock(mutex_);
  std::vector<ParsedRule> matches;

  // Search domain index
  for (const auto& pair : allow_rules_by_domain_) {
    for (const auto& rule : pair.second) {
      if (MatchesPattern(rule, host, path)) {
        matches.push_back(rule);
      }
    }
  }

  // Search generic
  for (const auto& rule : generic_allow_rules_) {
    if (rule.path_pattern.empty() || path.find(rule.path_pattern) != std::string::npos) {
      matches.push_back(rule);
    }
  }

  return matches;
}

std::vector<ParsedRule> RuleDatabase::FindMatchingBlockRules(const std::string& host,
                                                             const std::string& path) const {
  std::lock_guard<std::mutex> lock(mutex_);
  std::vector<ParsedRule> matches;

  // Search domain index
  for (const auto& pair : block_rules_by_domain_) {
    for (const auto& rule : pair.second) {
      if (MatchesPattern(rule, host, path)) {
        matches.push_back(rule);
      }
    }
  }

  // Search generic
  for (const auto& rule : generic_block_rules_) {
    if (rule.path_pattern.empty() || path.find(rule.path_pattern) != std::string::npos) {
      matches.push_back(rule);
    }
  }

  return matches;
}

bool RuleDatabase::IsKnownTrackerHost(const std::string& host) const {
  std::lock_guard<std::mutex> lock(mutex_);
  std::string lower_host = ToLower(host);
  for (const auto& pair : block_rules_by_domain_) {
    if (lower_host == pair.first ||
        (lower_host.length() > pair.first.length() + 1 &&
         lower_host.rfind("." + pair.first) == (lower_host.length() - pair.first.length() - 1))) {
      return true;
    }
  }
  return false;
}

size_t RuleDatabase::RuleCount() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return total_rules_;
}

void RuleDatabase::Clear() {
  std::lock_guard<std::mutex> lock(mutex_);
  allow_rules_by_domain_.clear();
  block_rules_by_domain_.clear();
  generic_allow_rules_.clear();
  generic_block_rules_.clear();
  total_rules_ = 0;
}

}  // namespace privacy
}  // namespace lean_thorium