#ifndef CHROME_BROWSER_PRIVACY_TRACKING_PARAMETER_FILTER_TRACKING_PARAMETER_FILTER_H_
#define CHROME_BROWSER_PRIVACY_TRACKING_PARAMETER_FILTER_TRACKING_PARAMETER_FILTER_H_

#include <string>
#include <vector>
#include <unordered_set>

namespace lean_thorium {
namespace privacy {

struct FilterResult {
  std::string cleaned_url;
  bool parameters_removed = false;
  std::vector<std::string> removed_parameters;
};

class TrackingParameterFilter {
 public:
  TrackingParameterFilter();
  ~TrackingParameterFilter() = default;

  // Strips tracking parameters from the URL, preserving functional parameters, ordering, and fragments
  FilterResult StripTrackingParameters(const std::string& url_str) const;

  // Checks if a given query parameter key is a known tracking parameter
  bool IsTrackingParameter(const std::string& key) const;

  // Adds a custom parameter to the strip list
  void AddTrackingParameter(const std::string& param_name);

 private:
  std::unordered_set<std::string> tracking_params_;
};

}  // namespace privacy
}  // namespace lean_thorium

#endif  // CHROME_BROWSER_PRIVACY_TRACKING_PARAMETER_FILTER_TRACKING_PARAMETER_FILTER_H_