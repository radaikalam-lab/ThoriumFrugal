#include "chrome/browser/privacy/privacy_policy/privacy_policy_manager.h"

#include <chrono>
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <random>

namespace lean_thorium {
namespace privacy {

namespace {

std::string CurrentIsoUtcTimestamp() {
  auto now = std::chrono::system_clock::now();
  auto in_time_t = std::chrono::system_clock::to_time_t(now);
  std::stringstream ss;
  ss << std::put_time(std::gmtime(&in_time_t), "%Y-%m-%dT%H:%M:%SZ");
  return ss.str();
}

std::string ToLower(std::string str) {
  std::transform(str.begin(), str.end(), str.begin(), [](unsigned char c) {
    return std::tolower(c);
  });
  return str;
}

bool IsSensitiveQueryKey(const std::string& key) {
  std::string k = ToLower(key);
  return (k == "token" || k == "auth" || k == "key" || k == "secret" ||
          k == "password" || k == "passwd" || k == "access_token" ||
          k == "api_key" || k == "apikey" || k == "session" || k == "session_id");
}

}  // namespace

PrivacyPolicyManager* PrivacyPolicyManager::GetInstance() {
  static PrivacyPolicyManager instance;
  return &instance;
}

PrivacyPolicyManager::PrivacyPolicyManager() {
  // Generate a random 4-hex process instance prefix
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<uint32_t> dis(0x1000, 0xFFFF);
  std::stringstream ss;
  ss << "lt_priv_" << std::hex << dis(gen) << "_";
  process_prefix_ = ss.str();
}

std::string PrivacyPolicyManager::GenerateEventId() {
  uint64_t count = event_counter_.fetch_add(1, std::memory_order_relaxed);
  std::stringstream ss;
  ss << process_prefix_ << std::setfill('0') << std::setw(6) << count;
  return ss.str();
}

void PrivacyPolicyManager::SetPrivacyProtectionEnabled(bool enabled) {
  enabled_ = enabled;
}

bool PrivacyPolicyManager::IsPrivacyProtectionEnabled() const {
  return enabled_;
}

void PrivacyPolicyManager::SetSiteException(const std::string& host_or_domain,
                                            SitePolicyException exception) {
  site_exceptions_.SetSiteException(host_or_domain, exception);
}

SitePolicyException PrivacyPolicyManager::GetSiteException(const std::string& host_or_domain) const {
  return site_exceptions_.GetSiteException(host_or_domain);
}

std::string PrivacyPolicyManager::SanitizeUrlForTelemetry(const std::string& raw_url) {
  if (raw_url.empty()) return "";

  // 1. Strip Userinfo (credentials) if present
  std::string url = raw_url;
  size_t scheme_pos = url.find("://");
  size_t start = (scheme_pos != std::string::npos) ? scheme_pos + 3 : 0;
  size_t at_pos = url.find('@', start);
  size_t slash_pos = url.find('/', start);

  if (at_pos != std::string::npos && (slash_pos == std::string::npos || at_pos < slash_pos)) {
    url = url.substr(0, start) + url.substr(at_pos + 1);
  }

  // 2. Redact sensitive query parameters
  size_t query_pos = url.find('?');
  if (query_pos == std::string::npos) {
    return url;
  }

  size_t hash_pos = url.find('#', query_pos);
  std::string base_part = url.substr(0, query_pos);
  std::string query_part = (hash_pos == std::string::npos)
                               ? url.substr(query_pos + 1)
                               : url.substr(query_pos + 1, hash_pos - (query_pos + 1));
  std::string fragment_part = (hash_pos != std::string::npos) ? url.substr(hash_pos) : "";

  std::stringstream ss(query_part);
  std::string pair;
  std::vector<std::string> sanitized_pairs;

  while (std::getline(ss, pair, '&')) {
    if (pair.empty()) continue;
    size_t eq_pos = pair.find('=');
    std::string key = (eq_pos != std::string::npos) ? pair.substr(0, eq_pos) : pair;
    if (IsSensitiveQueryKey(key)) {
      sanitized_pairs.push_back(key + "=[REDACTED]");
    } else {
      sanitized_pairs.push_back(pair);
    }
  }

  std::string new_query = "";
  for (size_t i = 0; i < sanitized_pairs.size(); ++i) {
    if (i > 0) new_query += "&";
    new_query += sanitized_pairs[i];
  }

  return base_part + (new_query.empty() ? "" : "?" + new_query) + fragment_part;
}

FilterResult PrivacyPolicyManager::SanitizeUrl(const std::string& url_str) {
  if (!enabled_) {
    FilterResult r;
    r.cleaned_url = url_str;
    r.parameters_removed = false;
    return r;
  }

  FilterResult result = parameter_filter_.StripTrackingParameters(url_str);
  if (result.parameters_removed) {
    PrivacyEvent event;
    event.event_id = GenerateEventId();
    event.event_type = "privacy.tracking_parameter_removed";
    event.host = ThirdPartyClassifier::ExtractHostname(result.cleaned_url);
    event.sanitized_url = SanitizeUrlForTelemetry(result.cleaned_url);
    event.stripped_parameters = result.removed_parameters;
    event.timestamp = CurrentIsoUtcTimestamp();
    event_dispatcher_.DispatchEvent(event);
  }
  return result;
}

PrivacyEvaluationResult PrivacyPolicyManager::EvaluateRequest(const std::string& target_url_str,
                                                              const std::string& initiator_origin_str,
                                                              ResourceType resource_type) {
  PrivacyEvaluationResult result;
  result.decision = PrivacyDecision::kAllow;
  result.party_context = ThirdPartyClassifier::Classify(target_url_str, initiator_origin_str);
  result.classification = TrackerClassification::kNotTracker;

  if (!enabled_) {
    return result;
  }

  std::string target_host = ThirdPartyClassifier::ExtractHostname(target_url_str);
  std::string init_host = ThirdPartyClassifier::ExtractHostname(initiator_origin_str);
  std::string init_domain = ThirdPartyClassifier::GetDomain(init_host);

  // 1. Check Per-Site Exceptions (Normative Precedence: Site exceptions override rule lists)
  SitePolicyException exception = site_exceptions_.GetSiteException(init_host);
  if (exception == SitePolicyException::kDefault && !init_domain.empty()) {
    exception = site_exceptions_.GetSiteException(init_domain);
  }

  if (exception == SitePolicyException::kAllowAll) {
    result.decision = PrivacyDecision::kAllow;
    result.exception_applied = true;
    result.reason = "Site exception: allow all third-party requests";

    PrivacyEvent event;
    event.event_id = GenerateEventId();
    event.event_type = "privacy.policy_exception_applied";
    event.host = target_host;
    event.sanitized_url = SanitizeUrlForTelemetry(target_url_str);
    event.initiator_origin = initiator_origin_str;
    event.party_context = result.party_context;
    event.resource_type = ResourceTypeToString(resource_type);
    event.timestamp = CurrentIsoUtcTimestamp();
    event_dispatcher_.DispatchEvent(event);

    return result;
  }

  // 2. Evaluate Tracker Rules via TrackerBlocker
  result = tracker_blocker_.EvaluateRequest(target_url_str, initiator_origin_str, resource_type);

  // 3. Dispatch Privacy Events (Local Telemetry)
  if (result.decision == PrivacyDecision::kBlock) {
    PrivacyEvent event;
    event.event_id = GenerateEventId();
    event.event_type = "privacy.request_blocked";
    event.host = target_host;
    event.sanitized_url = SanitizeUrlForTelemetry(target_url_str);
    event.initiator_origin = initiator_origin_str;
    event.party_context = result.party_context;
    event.resource_type = ResourceTypeToString(resource_type);
    event.classification = result.classification;
    event.rule_id = result.matched_rule.rule_id;
    event.timestamp = CurrentIsoUtcTimestamp();
    event_dispatcher_.DispatchEvent(event);
  } else if (result.classification == TrackerClassification::kKnownTracker) {
    PrivacyEvent event;
    event.event_id = GenerateEventId();
    event.event_type = "privacy.tracker_detected";
    event.host = target_host;
    event.sanitized_url = SanitizeUrlForTelemetry(target_url_str);
    event.initiator_origin = initiator_origin_str;
    event.party_context = result.party_context;
    event.resource_type = ResourceTypeToString(resource_type);
    event.classification = result.classification;
    event.rule_id = result.matched_rule.rule_id;
    event.timestamp = CurrentIsoUtcTimestamp();
    event_dispatcher_.DispatchEvent(event);
  }

  return result;
}

}  // namespace privacy
}  // namespace lean_thorium