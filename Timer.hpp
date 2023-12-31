#pragma once

#include <chrono>
#include <functional>
#include <memory>
#include <mutex>
#include <queue>
/*
Author: Ajen
Mail: ajenpan@gmail.com
Date: 20160815
*/
namespace xs {

class Timer {
  public:
    typedef std::chrono::seconds TimeDuration;
    typedef std::chrono::system_clock::time_point TimePoint;
    typedef std::chrono::milliseconds MilliSeconds;
    typedef std::chrono::seconds Seconds;
    typedef std::chrono::minutes Minutes;
    typedef std::chrono::hours Hours;

    typedef std::mutex Lock;
    typedef std::lock_guard<std::mutex> AutoLock;

    static Timer& Instace() {
        // it's safe in c++11 and later:
        // https://en.cppreference.com/w/cpp/language/storage_duration#Static_local_variables
        static Timer _instance;
        return _instance;
    }

  public:
    static inline std::tm Localtime(const std::time_t& time_tt) {
#ifdef _WIN32
        std::tm tm;
        localtime_s(&tm, &time_tt);
#else
        std::tm tm;
        localtime_r(&time_tt, &tm);
#endif
        return tm;
    }

    static inline std::tm Localtime() {
        return Localtime(time(nullptr));
    }

    static inline std::tm GMTime(const std::time_t& time_tt) {
#ifdef _WIN32
        std::tm tm;
        gmtime_s(&tm, &time_tt);
#else
        std::tm tm;
        gmtime_r(&time_tt, &tm);
#endif
        return tm;
    }

    static inline std::tm GMTime() {
        std::time_t now_t = time(nullptr);
        return GMTime(now_t);
    }

    static TimePoint NowTime() {
        return std::chrono::system_clock::now();
    }

    struct TimeTask;
    typedef std::shared_ptr<TimeTask> TimeTaskPtr;
    struct TimeTask {
        int32_t nRepeat = 1;
        Seconds nDelayTime = Seconds(0);
        Seconds nInterval = Seconds(1);
        std::function<void()> funcCallback;
        TimePoint nNextCallTime;

        bool IsPunctual(const TimePoint& nNow) {
            return nNow >= nNextCallTime;
        }

        bool UpdataNextCallTime(const TimePoint& nNow) {
            if (nRepeat == 0) {
                return false;
            }

            if (nDelayTime.count() != 0) {
                nNextCallTime = nNow + nDelayTime;
                nDelayTime = TimeDuration(0);
            } else {
                nNextCallTime = nNow + nInterval;
            }
            if (nRepeat > 0) {
                --nRepeat;
            }
            return true;
        }

        friend bool operator>(const TimeTaskPtr& a, const TimeTaskPtr& b) {
            return a->nNextCallTime < b->nNextCallTime;
        }
        friend bool operator<(const TimeTaskPtr& a, const TimeTaskPtr& b) {
            return a->nNextCallTime > b->nNextCallTime;
        }
        friend bool operator>(const TimeTask& a, const TimeTask& b) {
            return a.nNextCallTime < b.nNextCallTime;
        }
        friend bool operator<(const TimeTask& a, const TimeTask& b) {
            return a.nNextCallTime > b.nNextCallTime;
        }
    };

  public:
    struct EveryBase {
        virtual Seconds GetNextCallDelay() = 0;
        virtual Seconds GetInterval() = 0;
    };

    struct EveryHour : public EveryBase {
        EveryHour(uint8_t minute = 0, uint8_t second = 0)
            : nMinute(minute), nSecond(second) {
        }
        friend class Timer;

        virtual Seconds GetNextCallDelay() override {
            std::time_t nNowT = time(NULL);
            tm nNowTM = Localtime(nNowT);
            nNowTM.tm_min = nMinute;
            nNowTM.tm_sec = nSecond;
            std::time_t nTodayT = mktime(&nNowTM);
            return nTodayT > nNowT ? Seconds(nTodayT - nNowT) : Seconds(nTodayT - nNowT + GetInterval().count());
        }

        virtual Seconds GetInterval() override {
            return Hours(1);
        }
        uint8_t nMinute; // 0-59
        uint8_t nSecond; // 0-59
    };
    struct EveryDay : EveryBase {
        EveryDay(uint8_t hour, uint8_t minute = 0, uint8_t second = 0)
            : nHour(hour), nMinute(minute), nSecond(second) {
        }
        friend class Timer;

        virtual Seconds GetNextCallDelay() override {
            time_t nNowT = time(NULL);
            tm nNowTM = Localtime(nNowT);
            nNowTM.tm_hour = nHour;
            nNowTM.tm_min = nMinute;
            nNowTM.tm_sec = nSecond;
            time_t nTodayT = mktime(&nNowTM);
            return nTodayT > nNowT ? Seconds(nTodayT - nNowT) : Seconds(nTodayT - nNowT + GetInterval().count());
        }

        virtual Seconds GetInterval() override {
            return std::chrono::hours(24);
        }
        uint8_t nHour;   // 0-23
        uint8_t nMinute; // 0-59
        uint8_t nSecond; // 0-59
    };
    struct EveryWeek : EveryDay {
        EveryWeek(uint8_t week, uint8_t hour, uint8_t minute = 0, uint8_t second = 0)
            : EveryDay(hour, minute, second), nWeek(week) {
        }
        virtual Seconds GetNextCallDelay() override {
            time_t nNowT = time(NULL);
            tm nNowTM = Localtime(nNowT);
            nNowTM.tm_hour = nHour;
            nNowTM.tm_min = nMinute;
            nNowTM.tm_sec = nSecond;
            nNowTM.tm_mday += 7 - std::abs(nNowTM.tm_wday - nWeek);
            time_t nTodayT = mktime(&nNowTM);
            return nTodayT > nNowT ? Seconds(nTodayT - nNowT) : Seconds(nTodayT - nNowT + GetInterval().count());
        }
        virtual Seconds GetInterval() override {
            return std::chrono::hours(24 * 7);
        }
        uint8_t nWeek; // 0-6
    };

  public:
    bool After(uint32_t nDelayTime, const std::function<void()>& func) {
        auto pTask = std::make_shared<TimeTask>();
        pTask->funcCallback = func;
        pTask->nDelayTime = Seconds(nDelayTime);
        pTask->nInterval = Seconds(1);
        pTask->nRepeat = 1;
        pTask->UpdataNextCallTime(NowTime());
        return AddTask(pTask);
    }

    bool Schedule(const std::function<void()>& func, Seconds nInterval, int32_t nRepeat = -1, Seconds nDelayTime = Seconds(0)) {
        if (!func || nInterval.count() < 0 || nRepeat == 0 || nDelayTime.count() < 0) {
            return false;
        }

        auto pTask = std::make_shared<TimeTask>();
        pTask->funcCallback = func;
        pTask->nInterval = nInterval;
        pTask->nRepeat = nRepeat;
        pTask->nDelayTime = nDelayTime;
        pTask->UpdataNextCallTime(NowTime());
        return AddTask(pTask);
    }

    bool AddTask(TimeTaskPtr pTask) {
        AutoLock k(m_lock);
        m_cache.emplace(pTask);
        return true;
    }

    void OnTime() {
        TransferTask();

        do {
            if (m_queue.empty()) {
                break;
            }
            auto pTop = m_queue.top();

            TimePoint nNow = NowTime();
            if (!pTop->IsPunctual(nNow)) {
                break;
            }
            if (pTop->funcCallback) {
                pTop->funcCallback();
            }
            m_queue.pop();
            if (pTop->UpdataNextCallTime(nNow)) {
                m_queue.push(pTop);
            }
        } while (true);
    }
    void TransferTask() {
        AutoLock k(m_lock);
        while (!m_cache.empty()) {
            m_queue.push(m_cache.front());
            m_cache.pop();
        }
    }

    Lock m_lock;
    std::queue<TimeTaskPtr> m_cache;
    std::priority_queue<TimeTaskPtr> m_queue;
};
} // namespace xs
