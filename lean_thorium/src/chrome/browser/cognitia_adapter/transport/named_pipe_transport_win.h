#ifndef CHROME_BROWSER_COGNITIA_ADAPTER_TRANSPORT_NAMED_PIPE_TRANSPORT_WIN_H_
#define CHROME_BROWSER_COGNITIA_ADAPTER_TRANSPORT_NAMED_PIPE_TRANSPORT_WIN_H_

#include <windows.h>
#include <memory>
#include <string>

#include "base/memory/scoped_refptr.h"
#include "base/task/sequenced_task_runner.h"
#include "chrome/browser/cognitia_adapter/transport/cognitia_transport.h"
#include "chrome/browser/cognitia_adapter/transport/bounded_event_queue.h"

namespace cognitia {

class NamedPipeTransportWin : public CognitiaTransport {
 public:
  explicit NamedPipeTransportWin(const std::wstring& pipe_name = L"\\\\.\\pipe\\cognitia_browser_stream");
  ~NamedPipeTransportWin() override;

  void SendObservation(const std::string& json_payload) override;
  TransportStatus GetStatus() const override;
  void TryReconnect() override;

 private:
  void ConnectOnBackgroundThread();
  void SendPayloadOnBackgroundThread(std::string payload);
  void FlushQueueOnBackgroundThread();

  const std::wstring pipe_name_;
  HANDLE pipe_handle_ = INVALID_HANDLE_VALUE;
  mutable TransportStatus status_ = TransportStatus::kDisconnected;
  BoundedEventQueue event_queue_;
  scoped_refptr<base::SequencedTaskRunner> background_task_runner_;
};

}  // namespace cognitia

#endif  // CHROME_BROWSER_COGNITIA_ADAPTER_TRANSPORT_NAMED_PIPE_TRANSPORT_WIN_H_
