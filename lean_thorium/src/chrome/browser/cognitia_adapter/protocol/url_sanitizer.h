#ifndef CHROME_BROWSER_COGNITIA_ADAPTER_PROTOCOL_URL_SANITIZER_H_
#define CHROME_BROWSER_COGNITIA_ADAPTER_PROTOCOL_URL_SANITIZER_H_

#include <string>
#include "url/gurl.h"
#include "chrome/browser/cognitia_adapter/observation/observation_types.h"

namespace cognitia {

class UrlSanitizer {
 public:
  // Sanitizes a URL according to the active policy, removing embedded credentials,
  // session tokens, and sensitive query parameters (e.g., token, auth, session, key).
  static std::string SanitizeUrl(const GURL& raw_url, UrlExposurePolicy policy);

  // Checks if a given query parameter key is sensitive and must be redacted.
  static bool IsSensitiveQueryParam(const std::string& key);
};

}  // namespace cognitia

#endif  // CHROME_BROWSER_COGNITIA_ADAPTER_PROTOCOL_URL_SANITIZER_H_
