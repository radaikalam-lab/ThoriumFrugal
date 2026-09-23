#ifndef CHROME_BROWSER_COGNITIA_ADAPTER_TRANSPORT_BOUNDED_EVENT_QUEUE_H_
#define CHROME_BROWSER_COGNITIA_ADAPTER_TRANSPORT_BOUNDED_EVENT_QUEUE_H_

#include <deque>
#include <string>
#include "base/synchronization/lock.h"

namespace cognitia {

class BoundedEventQueue {
 public:
  explicit BoundedEventQueue(size_t max_capacity = 100);
  ~BoundedEventQueue();

  // Pushes a serialized observation JSON payload into the queue.
  // If queue exceeds max_capacity, the oldest event is dropped to prevent memory growth.
  void Enqueue(std::string payload);

  // Pops the oldest event from the queue. Returns false if empty.
  bool Dequeue(std::string* out_payload);

  size_t Size() const;
  bool IsEmpty() const;
  void Clear();
  size_t dropped_count() const { return dropped_count_; }

 private:
  const size_t max_capacity_;
  mutable base::Lock lock_;
  std::deque<std::string> queue_;
  size_t dropped_count_ = 0;
};

}  // namespace cognitia

#endif  // CHROME_BROWSER_COGNITIA_ADAPTER_TRANSPORT_BOUNDED_EVENT_QUEUE_H_
