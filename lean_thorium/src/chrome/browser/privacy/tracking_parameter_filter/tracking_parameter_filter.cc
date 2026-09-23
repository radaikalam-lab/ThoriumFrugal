#include "chrome/browser/privacy/tracking_parameter_filter/tracking_parameter_filter.h"

#include <algorithm>
#include <sstream>

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

TrackingParameterFilter::TrackingParameterFilter() {
  // Canonical high-confidence tracking parameters
  tracking_params_ = {
      // Google Analytics / Urchin Traffic Monitor
      "utm_source",
      "utm_medium",
      "utm_campaign",
      "utm_term",
      "utm_content",
      "utm_id",
      "utm_source_platform",
      "utm_creative_format",
      "utm_marketing_tactic",
      // Google Ads / Click IDs
      "gclid",
      "gbraid",
      "wbraid",
      "dclid",
      // Facebook / Meta
      "fbclid",
      // Microsoft Ads / Bing
      "msclkid",
      // Mailchimp
      "mc_eid",
      "mc_cid",
      // Yandex
      "yclid",
      "_openstat",
      // Instagram
      "igshid",
      // Twitter / X
      "twclid",
      // Marketo
      "mkt_tok",
      // Hubspot
      "_hsenc",
      "_hsmi",
      // Oracle / Eloqua
      "elqtrackid",
      "elqaid",
      "elqat",
  };
}

void TrackingParameterFilter::AddTrackingParameter(const std::string& param_name) {
  tracking_params_.insert(ToLower(param_name));
}

bool TrackingParameterFilter::IsTrackingParameter(const std::string& key) const {
  return tracking_params_.count(ToLower(key)) > 0;
}

FilterResult TrackingParameterFilter::StripTrackingParameters(const std::string& url_str) const {
  FilterResult result;
  result.cleaned_url = url_str;
  result.parameters_removed = false;

  size_t query_pos = url_str.find('?');
  if (query_pos == std::string::npos) {
    return result;
  }

  size_t hash_pos = url_str.find('#', query_pos);
  std::string base_url = url_str.substr(0, query_pos);
  std::string query_str = (hash_pos == std::string::npos)
                              ? url_str.substr(query_pos + 1)
                              : url_str.substr(query_pos + 1, hash_pos - (query_pos + 1));
  std::string fragment_str = (hash_pos != std::string::npos) ? url_str.substr(hash_pos) : "";

  if (query_str.empty()) {
    return result;
  }

  std::vector<std::string> preserved_pairs;
  std::stringstream ss(query_str);
  std::string item;

  while (std::getline(ss, item, '&')) {
    if (item.empty()) continue;

    size_t eq_pos = item.find('=');
    std::string key = (eq_pos != std::string::npos) ? item.substr(0, eq_pos) : item;

    if (IsTrackingParameter(key)) {
      result.parameters_removed = true;
      result.removed_parameters.push_back(key);
    } else {
      preserved_pairs.push_back(item);
    }
  }

  if (!result.parameters_removed) {
    return result;
  }

  std::string new_query = "";
  for (size_t i = 0; i < preserved_pairs.size(); ++i) {
    if (i > 0) new_query += "&";
    new_query += preserved_pairs[i];
  }

  if (new_query.empty()) {
    result.cleaned_url = base_url + fragment_str;
  } else {
    result.cleaned_url = base_url + "?" + new_query + fragment_str;
  }

  return result;
}

}  // namespace privacy
}  // namespace lean_thorium