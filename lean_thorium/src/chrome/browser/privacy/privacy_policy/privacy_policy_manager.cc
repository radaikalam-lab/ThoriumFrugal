#include "chrome/browser/privacy/privacy_policy/privacy_policy_manager.h"

#include <chrono>
#include <iomanip>
#include <sstream>

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

}  // namespace

PrivacyPolicyManager* PrivacyPolicyManager::GetInstance() {
  static PrivacyPolicyManager instance;
  return &instance;
}

PrivacyPolicyManager::PrivacyPolicyManager() = default;

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
    event.event_id = "ev_" + std::to_string(std::chrono::system_clock::now().time_since_epoch().count());
    event.event_type = "privacy.tracking_parameter_removed";
    event.sanitized_url = result.cleaned_url;
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

  std::string init_host = ThirdPartyClassifier::ExtractHostname(initiator_origin_str);
  std::string init_domain = ThirdPartyClassifier::GetDomain(init_host);

  // 1. Check Per-Site Exceptions
  SitePolicyException exception = site_exceptions_.GetSiteException(init_host);
  if (exception == SitePolicyException::kDefault && !init_domain.empty()) {
    exception = site_exceptions_.GetSiteException(init_domain);
  }

  if (exception == SitePolicyException::kAllowAll) {
    result.decision = PrivacyDecision::kAllow;
    result.exception_applied = true;
    result.reason = "Site exception: allow all third-party requests";

    PrivacyEvent event;
    event.event_id = "ev_" + std::to_string(std::chrono::system_clock::now().time_since_epoch().count());
    event.event_type = "privacy.policy_exception_applied";
    event.sanitized_url = target_url_str;
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
    event.event_id = "ev_" + std::to_string(std::chrono::system_clock::now().time_since_epoch().count());
    event.event_type = "privacy.request_blocked";
    event.sanitized_url = target_url_str;
    event.initiator_origin = initiator_origin_str;
    event.party_context = result.party_context;
    event.resource_type = ResourceTypeToString(resource_type);
    event.classification = result.classification;
    event.rule_id = result.matched_rule_id;
    event.timestamp = CurrentIsoUtcTimestamp();
    event_dispatcher_.DispatchEvent(event);
  } else if (result.classification == TrackerClassification::kKnownTracker) {
    PrivacyEvent event;
    event.event_id = "ev_" + std::to_string(std::chrono::system_clock::now().time_since_epoch().count());
    event.event_type = "privacy.tracker_detected";
    event.sanitized_url = target_url_str;
    event.initiator_origin = initiator_origin_str;
    event.party_context = result.party_context;
    event.resource_type = ResourceTypeToString(resource_type);
    event.classification = result.classification;
    event.rule_id = result.matched_rule_id;
    event.timestamp = CurrentIsoUtcTimestamp();
    event_dispatcher_.DispatchEvent(event);
  }

  return result;
}

}  // namespace privacy
}  // namespace lean_thorium