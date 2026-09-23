#ifndef CHROME_BROWSER_PRIVACY_PRIVACY_POLICY_SITE_EXCEPTIONS_H_
#define CHROME_BROWSER_PRIVACY_PRIVACY_POLICY_SITE_EXCEPTIONS_H_

#include <string>
#include <unordered_map>
#include <mutex>

namespace lean_thorium {
namespace privacy {

enum class SitePolicyException {
  kDefault = 0,   // Apply standard privacy filtering
  kAllowAll = 1,  // Disable tracker blocking for this site
  kBlockAll = 2,  // Strict mode: block all third-party requests
};

class SiteExceptionsManager {
 public:
  SiteExceptionsManager() = default;
  ~SiteExceptionsManager() = default;

  void SetSiteException(const std::string& host_or_domain, SitePolicyException exception) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (exception == SitePolicyException::kDefault) {
      exceptions_.erase(host_or_domain);
    } else {
      exceptions_[host_or_domain] = exception;
    }
  }

  SitePolicyException GetSiteException(const std::string& host_or_domain) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = exceptions_.find(host_or_domain);
    if (it != exceptions_.end()) {
      return it->second;
    }
    return SitePolicyException::kDefault;
  }

  bool HasException(const std::string& host_or_domain) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return exceptions_.find(host_or_domain) != exceptions_.end();
  }

  void Clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    exceptions_.clear();
  }

 private:
  mutable std::mutex mutex_;
  std::unordered_map<std::string, SitePolicyException> exceptions_;
};

}  // namespace privacy
}  // namespace lean_thorium

#endif  // CHROME_BROWSER_PRIVACY_PRIVACY_POLICY_SITE_EXCEPTIONS_H_