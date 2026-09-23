#ifndef CHROME_BROWSER_COGNITIA_ADAPTER_COGNITIA_ADAPTER_BOUNDARY_H_
#define CHROME_BROWSER_COGNITIA_ADAPTER_COGNITIA_ADAPTER_BOUNDARY_H_

#include <string>
#include <memory>
#include "base/time/time.h"
#include "base/values.h"

namespace cognitia {

// Read-only observation envelope describing browser events for external advisory intelligence.
// Epistemic invariant: This data structure is strictly read-only and conveys zero authority.
struct ObservationEnvelope {
  std::string envelope_version = "1.0";
  base::Time timestamp;
  std::string observation_type; // e.g., "PAGE_COMMITTED", "TAB_ACTIVATED", "SELECTION_CHANGED"
  int window_id = 0;
  int tab_id = 0;
  bool is_active_tab = false;
  std::string url;
  std::string title;
  int http_status = 200;
  std::string selected_text;
  bool is_incognito = false;

  base::Value::Dict ToValueDict() const;
};

// Interface boundary for future Cognitia advisory connection.
// No active LLM or agent dependency is bundled into Lean Thorium.
class CognitiaAdapterBoundary {
 public:
  virtual ~CognitiaAdapterBoundary() = default;

  // Emits an asynchronous, non-blocking observation event to the IPC channel
  virtual void EmitObservation(const ObservationEnvelope& envelope) = 0;

  // Receives candidate advisory suggestions for presentation to the user authority layer
  virtual void HandleAdvisoryProposal(base::Value::Dict proposal) = 0;
};

}  // namespace cognitia

#endif  // CHROME_BROWSER_COGNITIA_ADAPTER_COGNITIA_ADAPTER_BOUNDARY_H_
