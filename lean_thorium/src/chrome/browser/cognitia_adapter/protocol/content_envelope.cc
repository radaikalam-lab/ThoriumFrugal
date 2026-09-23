#include "chrome/browser/cognitia_adapter/protocol/content_envelope.h"

#include "base/json/json_writer.h"
#include "base/strings/utf_string_conversions.h"
#include "base/unguessable_token.h"
#include "base/time/time.h"

namespace cognitia {

ContentEnvelope::ContentEnvelope(ContentExtractionMode mode,
                                 int window_id,
                                 int tab_id,
                                 const std::string& sanitized_url,
                                 const std::string& origin,
                                 const std::u16string& title,
                                 const std::string& extracted_text,
                                 bool is_truncated)
    : id_(base::UnguessableToken::Create().ToString()),
      mode_(mode),
      window_id_(window_id),
      tab_id_(tab_id),
      url_(sanitized_url),
      origin_(origin),
      title_(base::UTF16ToUTF8(title)),
      extracted_text_(extracted_text),
      is_truncated_(is_truncated) {
  base::Time::Exploded exploded;
  base::Time::Now().UTCExplode(&exploded);
  char buffer[64];
  snprintf(buffer, sizeof(buffer), "%04d-%02d-%02dT%02d:%02d:%02d.%03dZ",
           exploded.year, exploded.month, exploded.day_of_month,
           exploded.hour, exploded.minute, exploded.second,
           exploded.millisecond);
  created_at_ = std::string(buffer);
}

ContentEnvelope::~ContentEnvelope() = default;

base::Value::Dict ContentEnvelope::ToValueDict() const {
  base::Value::Dict root;
  root.Set("id", id_);
  root.Set("schema_version", schema_version_);
  root.Set("created_at", created_at_);
  root.Set("source_id", "thorium_browser_adapter");

  // User authorization block
  base::Value::Dict auth;
  auth.Set("mode", ContentExtractionModeToString(mode_));
  auth.Set("user_initiated", true);
  auth.Set("timestamp_utc", created_at_);
  auth.Set("scope", mode_ == ContentExtractionMode::kSelectedText ? "selection" : "page");
  root.Set("content_authorization", std::move(auth));

  base::Value::Dict payload;

  // Browser Identity
  base::Value::Dict browser;
  browser.Set("name", "Lean Thorium");
  browser.Set("version", "138.0.7204.306");
  payload.Set("browser", std::move(browser));

  // Page Context
  base::Value::Dict page;
  page.Set("url", url_);
  page.Set("origin", origin_);
  page.Set("title", title_);
  page.Set("window_id", window_id_);
  page.Set("tab_id", tab_id_);
  payload.Set("page", std::move(page));

  // Content Payload - Tagged explicitly as UNTRUSTED DATA
  base::Value::Dict content;
  content.Set("type", "untrusted_web_content");
  content.Set("text", extracted_text_);
  content.Set("byte_length", static_cast<int>(extracted_text_.size()));
  content.Set("is_truncated", is_truncated_);
  payload.Set("content", std::move(content));

  // Provenance metadata
  base::Value::Dict provenance;
  provenance.Set("source_type", "sensor");
  provenance.Set("producer_id", "thorium_browser_adapter");
  provenance.Set("capability_id", "authorized_content_extraction");
  provenance.Set("authorization_method", "explicit_user_action");
  payload.Set("provenance", std::move(provenance));

  root.Set("payload", std::move(payload));
  return root;
}

std::string ContentEnvelope::ToJson() const {
  std::string json_output;
  base::JSONWriter::WriteWithOptions(
      ToValueDict(), base::JSONWriter::OPTIONS_OMIT_DOUBLE_TYPE_PRESERVATION,
      &json_output);
  return json_output;
}

}  // namespace cognitia
