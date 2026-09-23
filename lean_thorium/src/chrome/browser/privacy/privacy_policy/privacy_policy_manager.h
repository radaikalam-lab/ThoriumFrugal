#ifndef CHROME_BROWSER_PRIVACY_PRIVACY_POLICY_PRIVACY_POLICY_MANAGER_H_
#define CHROME_BROWSER_PRIVACY_PRIVACY_POLICY_PRIVACY_POLICY_MANAGER_H_

#include <memory>
#include <string>
#include <atomic>
#include "chrome/browser/privacy/privacy_policy/privacy_types.h"
#include "chrome/browser/privacy/privacy_policy/site_exceptions.h"
#include "chrome/browser/privacy/tracker_blocker/tracker_blocker.h"
#include "chrome/browser/privacy/tracking_parameter_filter/tracking_parameter_filter.h"
#include "chrome/browser/privacy/privacy_events/privacy_event_dispatcher.h"

namespace lean_thorium {
namespace privacy {

class PrivacyPolicyManager {
 public:
  static PrivacyPolicyManager* GetInstance();

  PrivacyPolicyManager();
  ~PrivacyPolicyManager() = default;

  // Evaluates a network request against active privacy policies, rules, and per-site exceptions
  PrivacyEvaluationResult EvaluateRequest(const std::string& target_url_str,
                                         const std::string& initiator_origin_str,
                                         ResourceType resource_type);

  // Strips tracking parameters from a URL prior to navigation/dispatch
  FilterResult SanitizeUrl(const std::string& url_str);

  // Sanitizes a URL strictly for telemetry/privacy events (redacts credentials and sensitive tokens)
  static std::string SanitizeUrlForTelemetry(const std::string& raw_url);

  // Generates a unique, collision-free, thread-safe monotonic event ID
  std::string GenerateEventId();

  // Global toggle for privacy protection (Default: Enabled)
  void SetPrivacyProtectionEnabled(bool enabled);
  bool IsPrivacyProtectionEnabled() const;

  // Site exceptions management
  void SetSiteException(const std::string& host_or_domain, SitePolicyException exception);
  SitePolicyException GetSiteException(const std::string& host_or_domain) const;

  // Accessors for subsystems
  TrackerBlocker& GetTrackerBlocker() { return tracker_blocker_; }
  TrackingParameterFilter& GetParameterFilter() { return parameter_filter_; }
  PrivacyEventDispatcher& GetEventDispatcher() { return event_dispatcher_; }

 private:
  bool enabled_ = true;
  std::atomic<uint64_t> event_counter_{1};
  std::string process_prefix_;
  TrackerBlocker tracker_blocker_;
  TrackingParameterFilter parameter_filter_;
  SiteExceptionsManager site_exceptions_;
  PrivacyEventDispatcher event_dispatcher_;
};

}  // namespace privacy
}  // namespace lean_thorium

#endif  // CHROME_BROWSER_PRIVACY_PRIVACY_POLICY_PRIVACY_POLICY_MANAGER_H_