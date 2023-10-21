#pragma once

#include <stdint.h>
#include <queue>

#include "Signal.hpp"
#include "Lock.hpp"

namespace xs {
template <typename T>
class Queue {
  public:
    bool Push(const T& t) {
        AutoLock kLock(m_kMutex);
        m_queue.push(t);
        m_kSignal.Notify();
        return true;
    }

    bool Pop(T& t) {
        m_kSignal.Wait();

        AutoLock kLock(m_kMutex);
        if (m_queue.empty()) {
            return false;
        }
        t = std::move(m_queue.front());
        m_queue.pop();
        return true;
    }

    int Size() const {
        AutoLock kLock(m_kMutex);
        return m_queue.size();
    }

    void Clean() {
        AutoLock kLock(m_kMutex);
        std::queue<T> _temp;
        m_queue.swap(_temp);
    }

  private:
    std::queue<T> m_queue;
    Signal m_kSignal;
    Lock m_kMutex;
};
}