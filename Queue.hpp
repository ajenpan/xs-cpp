#pragma once

#include <queue>
#include <mutex>

#include "Signal.hpp"

namespace xs {
template <typename T>
class Queue {
  public:
    bool Push(const T& t) {
        std::lock_guard<std::mutex> kLock(_lock);
        _queue.push(t);
        _signal.Notify();
        return true;
    }

    bool WaitPop(T& t) {
        _signal.Wait();
        return Pop(t);
    }

    bool Pop(T& t) {
        std::lock_guard<std::mutex> kLock(_lock);
        if (_queue.empty()) {
            return false;
        }
        t = std::move(_queue.front());
        _queue.pop();
        return true;
    }

    int Size() const {
        std::lock_guard<std::mutex> kLock(_lock);
        return _queue.size();
    }

    void Clear() {
        std::lock_guard<std::mutex> kLock(_lock);
        std::queue<T> _temp;
        _queue.swap(_temp);
    }

  private:
    std::queue<T> _queue;
    Signal _signal;
    std::mutex _lock;
};
} // namespace xs