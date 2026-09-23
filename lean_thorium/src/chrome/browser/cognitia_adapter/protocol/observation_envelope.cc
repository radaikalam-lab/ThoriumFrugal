#include "chrome/browser/cognitia_adapter/protocol/observation_envelope.h"

#include "base/json/json_writer.h"
#include "base/strings/utf_string_conversions.h"
#include "base/unguessable_token.h"
#include "base/time/time.h"

namespace cognitia {

ObservationEnvelope::ObservationEnvelope(BrowserEventType event_type,
                                         int window_id,
                                         int tab_id,
                                         bool is_active_tab,
                                         const std::string& sanitized_url,
                                         const std::string& origin,
                                         const std::u16string& title,
                                         bool is_incognito)
    : id_(base::UnguessableToken::Create().ToString()),
      event_type_(event_type),
      window_id_(window_id),
      tab_id_(tab_id),
      is_active_tab_(is_active_tab),
      url_(sanitized_url),
      origin_(origin),
      title_(base::UTF16ToUTF8(title)),
      is_incognito_(is_incognito) {
  // Format current UTC timestamp in ISO-8601
  base::Time::Exploded exploded;
  base::Time::Now().UTCExplode(&exploded);
  char buffer[64];
  snprintf(buffer, sizeof(buffer), "%04d-%02d-%02dT%02d:%02d:%02d.%03dZ",
           exploded.year, exploded.month, exploded.day_of_month,
           exploded.hour, exploded.minute, exploded.second,
           exploded.millisecond);
  created_at_ = std::string(buffer);
}

ObservationEnvelope::~ObservationEnvelope() = default;

base::Value::Dict ObservationEnvelope::ToValueDict() const {
  base::Value::Dict root;
  root.Set("id", id_);
  root.Set("schema_version", schema_version_);
  root.Set("created_at", created_at_);
  root.Set("source_id", "thorium_browser_adapter");
  root.Set("metadata", base::Value::Dict());

  // Construct payload adhering to Cognitia ABI
  base::Value::Dict payload;

  // Browser Identity
  base::Value::Dict browser;
  browser.Set("name", "Lean Thorium");
  browser.Set("version", "138.0.7204.306");
  payload.Set("browser", std::move(browser));

  // Event Context
  base::Value::Dict event;
  event.Set("type", BrowserEventTypeToString(event_type_));
  event.Set("timestamp_utc", created_at_);
  payload.Set("event", std::move(event));

  // Window/Tab Context
  base::Value::Dict context;
  context.Set("window_id", window_id_);
  context.Set("tab_id", tab_id_);
  context.Set("is_active_tab", is_active_tab_);
  context.Set("is_incognito", is_incognito_);
  payload.Set("context", std::move(context));

  // Page Metadata
  base::Value::Dict page;
  page.Set("url", url_);
  page.Set("origin", origin_);
  page.Set("title", title_);
  payload.Set("page", std::move(page));

  // Provenance
  base::Value::Dict provenance;
  provenance.Set("source_type", "sensor");
  provenance.Set("producer_id", "thorium_browser_adapter");
  provenance.Set("capability_id", "browser_observation");
  payload.Set("provenance", std::move(provenance));

  root.Set("payload", std::move(payload));
  return root;
}

std::string ObservationEnvelope::ToJson() const {
  std::string json_output;
  base::JSONWriter::WriteWithOptions(
      ToValueDict(), base::JSONWriter::OPTIONS_OMIT_DOUBLE_TYPE_PRESERVATION,
      &json_output);
  return json_output;
}

}  // namespace cognitia
