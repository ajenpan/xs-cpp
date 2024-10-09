#pragma once

#include <chrono>
#include <functional>
#include <memory>
#include <mutex>
#include <queue>

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
    // 时间戳转换成 tm 类型当地时间
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

    // 取本地时间
    static inline std::tm Localtime() {
        return Localtime(time(nullptr));
    }

    // 把时间戳 转换 tm类型 格林威治时间
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

    // c 语言 tm 类型格林威治时间
    static inline std::tm GMTime() {
        std::time_t now_t = time(nullptr);
        return GMTime(now_t);
    }
    // 当前时间
    static TimePoint NowTime() {
        return std::chrono::system_clock::now();
    }

    // 任务基类
    struct Task {
        // 取消任务执行
        virtual bool Cancel() = 0;
        virtual const TimePoint& NextCallTimePoint() = 0;
    };

    struct TimeTask;
    // 任务类指针
    using TimeTaskPtr = std::shared_ptr<TimeTask>;

    // 任务类
    struct TimeTask : public Task {
        int32_t nRepeat = 1;
        Seconds nDelayTime = Seconds(0);
        Seconds nInterval = Seconds(1);
        std::function<void()> fnCallback = nullptr;
        TimePoint nNextCallTime;
        std::atomic_bool bCancel = {false};
        // 取消任务执行
        virtual bool Cancel() override {
            bCancel.store(true);
            return true;
        }
        virtual const TimePoint& NextCallTimePoint() {
            return nNextCallTime;
        }
        // 执行调用
        bool Call() {
            if (bCancel.load() || !fnCallback) {
                return false;
            }
            fnCallback();
            return true;
        }
        // 清理
        void Clear() {
            fnCallback = nullptr;
        }

        // 是否到达触发时间
        bool IsPunctual(const TimePoint& nNow) {
            return nNow >= nNextCallTime;
        }

        // 更新下次调用事件
        bool UpdataNextCallTime(const TimePoint& nNow) {
            if (bCancel) {
                return false;
            }
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
    };

    struct TimeTaskPtrCmp {
        bool operator()(const TimeTaskPtr& a, const TimeTaskPtr& b) {
            return a->nNextCallTime > b->nNextCallTime;
        }
    };

  public:
    // 循环基类
    struct EveryBase {
        // 获取当前到下次调用直接的时间, 单位秒
        virtual Seconds GetNextCallDelay() = 0;
        // 获取任务的时间间隔. 如每小时调用, 间隔就是60m
        virtual Seconds GetInterval() = 0;
    };
    // 每小时循环调用
    // 每小时几分几秒调用
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
    // 每天循环调用
    // 每天几点几分几秒调用
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

    // 每周循环调用, tm_wday 中 wday 范围 0-6, 0是星期一, 6是星期日
    // 每周星期几的几点几分几秒调用
    // 如 EveryWeek(0,12,30,0)
    struct EveryWeek : EveryDay {
        EveryWeek(uint8_t weekday, uint8_t hour, uint8_t minute = 0, uint8_t second = 0)
            : EveryDay(hour, minute, second), nWeekday(weekday) {
        }
        virtual Seconds GetNextCallDelay() override {
            time_t nNowT = time(NULL);
            tm nNowTM = Localtime(nNowT);
            nNowTM.tm_hour = nHour;
            nNowTM.tm_min = nMinute;
            nNowTM.tm_sec = nSecond;
            nNowTM.tm_mday += 7 - std::abs(nNowTM.tm_wday - nWeekday);
            time_t nTodayT = mktime(&nNowTM);
            return nTodayT > nNowT ? Seconds(nTodayT - nNowT) : Seconds(nTodayT - nNowT + GetInterval().count());
        }
        virtual Seconds GetInterval() override {
            return std::chrono::hours(24 * 7);
        }
        uint8_t nWeekday; // 0-6
    };

  public:
    // 延迟调用 1 次
    // @param nDelaySec 延迟多少秒
    // @param func 调用函数
    static std::shared_ptr<Task> After(uint32_t nDelaySec, const std::function<void()>& func) {
        auto pTask = Instace().NewTask();
        pTask->fnCallback = func;
        pTask->nDelayTime = Seconds(nDelaySec);
        pTask->nInterval = Seconds(0);
        pTask->nRepeat = 1;
        pTask->UpdataNextCallTime(NowTime());
        return pTask;
    }

    // 调度调用 n 次
    // @param func 调用函数
    // @param nInterval 调用间隔
    // @param nRepeat 调用次数, -1 为无限次
    // @param nDelayTime 延迟多少秒调用
    static std::shared_ptr<Task> Schedule(const std::function<void()>& func, Seconds nInterval, int32_t nRepeat = -1, Seconds nDelayTime = Seconds(0)) {
        if (!func || nInterval.count() < 0 || nRepeat == 0 || nDelayTime.count() < 0) {
            return nullptr;
        }
        auto pTask = Instace().NewTask();
        pTask->fnCallback = func;
        pTask->nInterval = nInterval;
        pTask->nRepeat = nRepeat;
        pTask->nDelayTime = nDelayTime;
        pTask->UpdataNextCallTime(NowTime());
        return pTask;
    }

    // 添加任务
    // @param pTask 任务
    void AddTask(TimeTaskPtr pTask) {
        AutoLock k(_lock);
        _cache.emplace(pTask);
    }

    // 时间触发回调
    void OnTime() {
        TransferTask();
        TimePoint nNow = NowTime();

        do {
            if (_queue.empty()) {
                break;
            }
            auto pTop = _queue.top();
            if (!pTop->IsPunctual(nNow)) {
                break;
            }
            _queue.pop();

            pTop->Call();

            if (pTop->UpdataNextCallTime(nNow)) {
                AddTask(pTop);
            } else {
                pTop->Clear();
            }
        } while (true);
    }

    // 任务转移
    void TransferTask() {
        AutoLock k(_lock);
        while (!_cache.empty()) {
            _queue.push(_cache.front());
            _cache.pop();
        }
    }

    // 新建任务
    TimeTaskPtr NewTask() {
        auto pTask = std::make_shared<TimeTask>();
        AddTask(pTask);
        return pTask;
    }

    Lock _lock;
    std::queue<TimeTaskPtr> _cache;
    std::priority_queue<TimeTaskPtr, std::vector<TimeTaskPtr>, TimeTaskPtrCmp> _queue;
};
} // namespace xs
