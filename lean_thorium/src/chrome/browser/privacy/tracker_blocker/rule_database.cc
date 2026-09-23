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

std::vector<std::string> RuleDatabase::GenerateCandidateDomains(const std::string& host) {
  std::vector<std::string> candidates;
  std::string lower_host = ToLower(host);
  if (lower_host.empty()) return candidates;

  // Strip trailing dot if present
  if (lower_host.back() == '.') {
    lower_host.pop_back();
  }

  candidates.push_back(lower_host);
  size_t dot_pos = lower_host.find('.');
  while (dot_pos != std::string::npos && dot_pos + 1 < lower_host.length()) {
    std::string parent = lower_host.substr(dot_pos + 1);
    if (!parent.empty()) {
      candidates.push_back(parent);
    }
    dot_pos = lower_host.find('.', dot_pos + 1);
  }

  return candidates;
}

bool RuleDatabase::AddRule(const ParsedRule& rule) {
  if (!rule.is_valid) return false;

  std::lock_guard<std::mutex> lock(mutex_);
  if (total_rules_ >= 50000) {
    return false;  // Bounded memory protection
  }

  std::string domain_key = ToLower(rule.domain_pattern);
  if (rule.rule_type == RuleType::kAllow) {
    if (!domain_key.empty()) {
      allow_rules_by_domain_[domain_key].push_back(rule);
    } else {
      generic_allow_rules_.push_back(rule);
    }
  } else {
    if (!domain_key.empty()) {
      block_rules_by_domain_[domain_key].push_back(rule);
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

bool RuleDatabase::MatchesPathAndOptions(const ParsedRule& rule, const std::string& path) const {
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
  std::vector<std::string> candidates = GenerateCandidateDomains(host);

  // Indexed lookup: Check only buckets for exact host and parent subdomains: O(labels)
  for (const auto& domain : candidates) {
    auto it = allow_rules_by_domain_.find(domain);
    if (it != allow_rules_by_domain_.end()) {
      for (const auto& rule : it->second) {
        // If domain is parent, match_subdomains must be true
        if (domain != candidates[0] && !rule.match_subdomains) {
          continue;
        }
        if (MatchesPathAndOptions(rule, path)) {
          matches.push_back(rule);
        }
      }
    }
  }

  // Check generic allow rules
  for (const auto& rule : generic_allow_rules_) {
    if (MatchesPathAndOptions(rule, path)) {
      matches.push_back(rule);
    }
  }

  return matches;
}

std::vector<ParsedRule> RuleDatabase::FindMatchingBlockRules(const std::string& host,
                                                             const std::string& path) const {
  std::lock_guard<std::mutex> lock(mutex_);
  std::vector<ParsedRule> matches;
  std::vector<std::string> candidates = GenerateCandidateDomains(host);

  // Indexed lookup: Check only buckets for exact host and parent subdomains: O(labels)
  for (const auto& domain : candidates) {
    auto it = block_rules_by_domain_.find(domain);
    if (it != block_rules_by_domain_.end()) {
      for (const auto& rule : it->second) {
        if (domain != candidates[0] && !rule.match_subdomains) {
          continue;
        }
        if (MatchesPathAndOptions(rule, path)) {
          matches.push_back(rule);
        }
      }
    }
  }

  // Check generic block rules
  for (const auto& rule : generic_block_rules_) {
    if (MatchesPathAndOptions(rule, path)) {
      matches.push_back(rule);
    }
  }

  return matches;
}

bool RuleDatabase::IsKnownTrackerHost(const std::string& host) const {
  std::lock_guard<std::mutex> lock(mutex_);
  std::vector<std::string> candidates = GenerateCandidateDomains(host);

  for (const auto& domain : candidates) {
    auto it = block_rules_by_domain_.find(domain);
    if (it != block_rules_by_domain_.end()) {
      for (const auto& rule : it->second) {
        if (domain == candidates[0] || rule.match_subdomains) {
          return true;
        }
      }
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