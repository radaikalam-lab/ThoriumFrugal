#ifndef CHROME_BROWSER_COGNITIA_ADAPTER_COGNITIA_ADAPTER_BOUNDARY_H_
#define CHROME_BROWSER_COGNITIA_ADAPTER_COGNITIA_ADAPTER_BOUNDARY_H_

#include <string>
#include <memory>
#include "base/time/time.h"
#include "base/values.h"
#include "chrome/browser/cognitia_adapter/protocol/observation_envelope.h"
#include "chrome/browser/cognitia_adapter/protocol/content_envelope.h"

namespace cognitia {

// Interface boundary for Cognitia observation and advisory connection.
// Epistemic invariant: Cognitia possesses ZERO browser-control or command execution authority.
class CognitiaAdapterBoundary {
 public:
  virtual ~CognitiaAdapterBoundary() = default;

  // Emits an asynchronous, non-blocking observation event to the local IPC channel
  virtual void EmitObservation(const ObservationEnvelope& envelope) = 0;

  // Emits an explicit user-authorized content extraction event
  virtual void EmitAuthorizedContent(const ContentEnvelope& envelope) = 0;

  // Receives candidate advisory suggestions for presentation to the user authority layer
  // Invariant: Proposals are advisory; human authorization is required before any action.
  virtual void HandleAdvisoryProposal(base::Value::Dict proposal) = 0;
};

}  // namespace cognitia

#endif  // CHROME_BROWSER_COGNITIA_ADAPTER_COGNITIA_ADAPTER_BOUNDARY_H_
