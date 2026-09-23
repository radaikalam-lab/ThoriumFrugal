#ifndef CHROME_BROWSER_COGNITIA_ADAPTER_TRANSPORT_COGNITIA_TRANSPORT_H_
#define CHROME_BROWSER_COGNITIA_ADAPTER_TRANSPORT_COGNITIA_TRANSPORT_H_

#include <string>

namespace cognitia {

enum class TransportStatus {
  kDisconnected,
  kConnecting,
  kConnected,
  kError
};

class CognitiaTransport {
 public:
  virtual ~CognitiaTransport() = default;

  // Asynchronously transmits a serialized JSON observation packet to Cognitia daemon
  virtual void SendObservation(const std::string& json_payload) = 0;

  // Returns current transport connection status
  virtual TransportStatus GetStatus() const = 0;

  // Initiates background reconnection attempt if disconnected
  virtual void TryReconnect() = 0;
};

}  // namespace cognitia

#endif  // CHROME_BROWSER_COGNITIA_ADAPTER_TRANSPORT_COGNITIA_TRANSPORT_H_
