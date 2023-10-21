#pragma once

#include <mutex>
#include <condition_variable>
#include <atomic>

namespace xs {
class Signal final {
  public:
    void Wait() {
        std::unique_lock<std::mutex> kLock(m_kMutex);
        m_kCondition.wait(kLock, [this]() -> bool {
            return m_nCargo > 0;
        });
        --m_nCargo;
    }
    void Notify() {
        std::lock_guard<std::mutex> kLock(m_kMutex);
        ++m_nCargo;
        m_kCondition.notify_one();
    }

  protected:
    std::mutex m_kMutex;
    std::atomic_int m_nCargo = 0;
    std::condition_variable m_kCondition;
};
}