#ifndef CHROME_BROWSER_COGNITIA_ADAPTER_PROTOCOL_CONTENT_ENVELOPE_H_
#define CHROME_BROWSER_COGNITIA_ADAPTER_PROTOCOL_CONTENT_ENVELOPE_H_

#include <string>
#include "base/time/time.h"
#include "base/values.h"
#include "chrome/browser/cognitia_adapter/content_extraction/content_types.h"

namespace cognitia {

// Immutable envelope for user-authorized webpage content analysis
// Invariant: Content is tagged as untrusted data; user authorization is strictly recorded.
class ContentEnvelope {
 public:
  ContentEnvelope(ContentExtractionMode mode,
                  int window_id,
                  int tab_id,
                  const std::string& sanitized_url,
                  const std::string& origin,
                  const std::u16string& title,
                  const std::string& extracted_text,
                  bool is_truncated);

  ~ContentEnvelope();

  base::Value::Dict ToValueDict() const;
  std::string ToJson() const;

  const std::string& id() const { return id_; }
  const std::string& schema_version() const { return schema_version_; }
  ContentExtractionMode mode() const { return mode_; }
  const std::string& extracted_text() const { return extracted_text_; }
  bool is_truncated() const { return is_truncated_; }

 private:
  std::string id_;
  std::string schema_version_ = "1.0.0";
  std::string created_at_;
  ContentExtractionMode mode_;
  int window_id_;
  int tab_id_;
  std::string url_;
  std::string origin_;
  std::string title_;
  std::string extracted_text_;
  bool is_truncated_;
};

}  // namespace cognitia

#endif  // CHROME_BROWSER_COGNITIA_ADAPTER_PROTOCOL_CONTENT_ENVELOPE_H_
