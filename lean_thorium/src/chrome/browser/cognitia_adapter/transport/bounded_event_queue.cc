#include "chrome/browser/cognitia_adapter/transport/bounded_event_queue.h"

namespace cognitia {

BoundedEventQueue::BoundedEventQueue(size_t max_capacity)
    : max_capacity_(max_capacity) {}

BoundedEventQueue::~BoundedEventQueue() = default;

void BoundedEventQueue::Enqueue(std::string payload) {
  base::AutoLock auto_lock(lock_);
  if (queue_.size() >= max_capacity_) {
    // Drop the oldest event to enforce bounded memory constraint
    queue_.pop_front();
    dropped_count_++;
  }
  queue_.push_back(std::move(payload));
}

bool BoundedEventQueue::Dequeue(std::string* out_payload) {
  base::AutoLock auto_lock(lock_);
  if (queue_.empty()) {
    return false;
  }
  *out_payload = std::move(queue_.front());
  queue_.pop_front();
  return true;
}

size_t BoundedEventQueue::Size() const {
  base::AutoLock auto_lock(lock_);
  return queue_.size();
}

bool BoundedEventQueue::IsEmpty() const {
  base::AutoLock auto_lock(lock_);
  return queue_.empty();
}

void BoundedEventQueue::Clear() {
  base::AutoLock auto_lock(lock_);
  queue_.clear();
}

}  // namespace cognitia
