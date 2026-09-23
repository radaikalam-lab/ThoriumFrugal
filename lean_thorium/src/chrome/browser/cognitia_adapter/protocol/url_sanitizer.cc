#include "chrome/browser/cognitia_adapter/protocol/url_sanitizer.h"

#include <vector>
#include "base/strings/string_util.h"
#include "net/base/url_util.h"

namespace cognitia {

namespace {

const char* const kSensitiveQueryParams[] = {
    "token", "access_token", "auth", "authtoken", "auth_token",
    "password", "pass", "pwd", "secret", "session", "session_id",
    "apikey", "api_key", "key", "signature", "sig", "code", "refresh_token"
};

}  // namespace

bool UrlSanitizer::IsSensitiveQueryParam(const std::string& key) {
  std::string lower_key = base::ToLowerASCII(key);
  for (const char* sensitive : kSensitiveQueryParams) {
    if (lower_key == sensitive) {
      return true;
    }
  }
  return false;
}

std::string UrlSanitizer::SanitizeUrl(const GURL& raw_url, UrlExposurePolicy policy) {
  if (!raw_url.is_valid() || policy == UrlExposurePolicy::kOmitted) {
    return "";
  }

  if (policy == UrlExposurePolicy::kOriginOnly) {
    return raw_url.DeprecatedGetOriginAsURL().spec();
  }

  // Full URL sanitized policy:
  // 1. Strip username and password (http://user:pass@host -> http://host)
  GURL scrubbed_url = raw_url.GetWithoutRef();
  if (scrubbed_url.has_username() || scrubbed_url.has_password()) {
    GURL::Replacements replacements;
    replacements.ClearUsername();
    replacements.ClearPassword();
    scrubbed_url = scrubbed_url.ReplaceComponents(replacements);
  }

  // 2. Redact sensitive query parameters
  if (scrubbed_url.has_query()) {
    std::string sanitized_query;
    for (net::QueryIterator it(scrubbed_url); !it.IsAtEnd(); it.Advance()) {
      if (!sanitized_query.empty()) {
        sanitized_query += "&";
      }
      std::string key = it.GetKey();
      if (IsSensitiveQueryParam(key)) {
        sanitized_query += key + "=[REDACTED]";
      } else {
        sanitized_query += key + "=" + it.GetValue();
      }
    }
    GURL::Replacements replacements;
    replacements.SetQueryStr(sanitized_query);
    scrubbed_url = scrubbed_url.ReplaceComponents(replacements);
  }

  return scrubbed_url.spec();
}

}  // namespace cognitia
