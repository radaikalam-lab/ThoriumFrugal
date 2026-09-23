#ifndef CHROME_BROWSER_COGNITIA_ADAPTER_PROTOCOL_OBSERVATION_ENVELOPE_H_
#define CHROME_BROWSER_COGNITIA_ADAPTER_PROTOCOL_OBSERVATION_ENVELOPE_H_

#include <string>
#include "base/time/time.h"
#include "base/values.h"
#include "chrome/browser/cognitia_adapter/observation/observation_types.h"

namespace cognitia {

// Immutable, versioned observation envelope adhering to Cognitia Cognitive ABI v1.0.0
class ObservationEnvelope {
 public:
  ObservationEnvelope(BrowserEventType event_type,
                      int window_id,
                      int tab_id,
                      bool is_active_tab,
                      const std::string& sanitized_url,
                      const std::string& origin,
                      const std::u16string& title,
                      bool is_incognito);

  ~ObservationEnvelope();

  // Serialization to standard base::Value dictionary structure
  base::Value::Dict ToValueDict() const;

  // Serialized canonical JSON string
  std::string ToJson() const;

  // Accessors
  const std::string& id() const { return id_; }
  const std::string& schema_version() const { return schema_version_; }
  const std::string& created_at() const { return created_at_; }
  BrowserEventType event_type() const { return event_type_; }
  int tab_id() const { return tab_id_; }
  int window_id() const { return window_id_; }
  const std::string& url() const { return url_; }
  const std::string& origin() const { return origin_; }
  const std::string& title() const { return title_; }

 private:
  std::string id_;
  std::string schema_version_ = "1.0.0";
  std::string created_at_;
  BrowserEventType event_type_;
  int window_id_;
  int tab_id_;
  bool is_active_tab_;
  std::string url_;
  std::string origin_;
  std::string title_;
  bool is_incognito_;
};

}  // namespace cognitia

#endif  // CHROME_BROWSER_COGNITIA_ADAPTER_PROTOCOL_OBSERVATION_ENVELOPE_H_
