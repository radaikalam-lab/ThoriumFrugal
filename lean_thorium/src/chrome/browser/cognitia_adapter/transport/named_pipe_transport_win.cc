#include "chrome/browser/cognitia_adapter/transport/named_pipe_transport_win.h"

#include "base/task/thread_pool.h"
#include "base/functional/bind.h"

namespace cognitia {

NamedPipeTransportWin::NamedPipeTransportWin(const std::wstring& pipe_name)
    : pipe_name_(pipe_name),
      event_queue_(100),
      background_task_runner_(base::ThreadPool::CreateSequencedTaskRunner(
          {base::TaskPriority::BEST_EFFORT, base::MayBlock()})) {}

NamedPipeTransportWin::~NamedPipeTransportWin() {
  if (pipe_handle_ != INVALID_HANDLE_VALUE) {
    CloseHandle(pipe_handle_);
    pipe_handle_ = INVALID_HANDLE_VALUE;
  }
}

TransportStatus NamedPipeTransportWin::GetStatus() const {
  return status_;
}

void NamedPipeTransportWin::TryReconnect() {
  background_task_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(&NamedPipeTransportWin::ConnectOnBackgroundThread,
                     base::Unretained(this)));
}

void NamedPipeTransportWin::SendObservation(const std::string& json_payload) {
  // Non-blocking dispatch to background worker thread (< 0.05ms on UI thread)
  background_task_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(&NamedPipeTransportWin::SendPayloadOnBackgroundThread,
                     base::Unretained(this), json_payload));
}

void NamedPipeTransportWin::ConnectOnBackgroundThread() {
  if (pipe_handle_ != INVALID_HANDLE_VALUE) {
    return;
  }

  status_ = TransportStatus::kConnecting;

  // Open local Windows Named Pipe with GENERIC_WRITE access
  pipe_handle_ = CreateFileW(
      pipe_name_.c_str(),
      GENERIC_WRITE,
      0,
      nullptr,
      OPEN_EXISTING,
      FILE_ATTRIBUTE_NORMAL,
      nullptr);

  if (pipe_handle_ == INVALID_HANDLE_VALUE) {
    status_ = TransportStatus::kDisconnected;
  } else {
    status_ = TransportStatus::kConnected;
    FlushQueueOnBackgroundThread();
  }
}

void NamedPipeTransportWin::FlushQueueOnBackgroundThread() {
  if (pipe_handle_ == INVALID_HANDLE_VALUE) {
    return;
  }

  std::string queued_item;
  while (event_queue_.Dequeue(&queued_item)) {
    std::string framed_message = queued_item + "\n";
    DWORD bytes_written = 0;
    BOOL success = WriteFile(
        pipe_handle_,
        framed_message.data(),
        static_cast<DWORD>(framed_message.size()),
        &bytes_written,
        nullptr);

    if (!success) {
      CloseHandle(pipe_handle_);
      pipe_handle_ = INVALID_HANDLE_VALUE;
      status_ = TransportStatus::kDisconnected;
      event_queue_.Enqueue(std::move(queued_item));
      break;
    }
  }
}

void NamedPipeTransportWin::SendPayloadOnBackgroundThread(std::string payload) {
  if (pipe_handle_ == INVALID_HANDLE_VALUE) {
    // Attempt lazy connect
    ConnectOnBackgroundThread();
  }

  if (pipe_handle_ == INVALID_HANDLE_VALUE) {
    // Cognitia is not running or disconnected; buffer into bounded queue
    event_queue_.Enqueue(std::move(payload));
    return;
  }

  std::string framed_message = payload + "\n";
  DWORD bytes_written = 0;
  BOOL success = WriteFile(
      pipe_handle_,
      framed_message.data(),
      static_cast<DWORD>(framed_message.size()),
      &bytes_written,
      nullptr);

  if (!success) {
    // Connection broken; close handle and queue the payload
    CloseHandle(pipe_handle_);
    pipe_handle_ = INVALID_HANDLE_VALUE;
    status_ = TransportStatus::kDisconnected;
    event_queue_.Enqueue(std::move(payload));
  }
}

}  // namespace cognitia
