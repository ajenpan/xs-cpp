#pragma once

#include <mutex>
#include <condition_variable>
#include <atomic>

namespace xs {
class Signal final {
  public:
    Signal()
        : _cargo(0) {
    }
    void Wait() {
        std::unique_lock<std::mutex> kLock(_lock);
        _condition.wait(kLock, [this]() -> bool {
            return _cargo > 0;
        });
        --_cargo;
    }

    bool WaitFor(std::chrono::milliseconds d) {
        std::unique_lock<std::mutex> kLock(_lock);
        bool ok = _condition.wait_for(kLock, d, [this]() -> bool {
            return _cargo > 0;
        });
        if (ok) {
            --_cargo;
        }
        return ok;
    }

    void Notify() {
        std::lock_guard<std::mutex> kLock(_lock);
        ++_cargo;
        _condition.notify_one();
    }

  protected:
    std::mutex _lock;
    std::atomic<int32_t> _cargo;
    std::condition_variable _condition;
};
} // namespace xs