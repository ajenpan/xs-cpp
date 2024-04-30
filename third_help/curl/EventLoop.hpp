#pragma once

#include <string>
#include <atomic>
#include <functional>
#include <vector>
#include <thread>
#include <mutex>

#include "Helper.h"
#include "ServerStatus.hpp"

namespace base_on_libevent {

class EventLoop {
  public:
    EventLoop() {
        _status = ServerStatus::Status::kNull;
        struct event_config* cfg = event_config_new();
        // Does not cache time to get a preciser timer
        event_config_set_flag(cfg, EVENT_BASE_FLAG_NO_CACHE_TIME);
        _evbase = event_base_new_with_config(cfg);
        event_config_free(cfg);
    }

    virtual ~EventLoop() {
        if (_evbase != nullptr) {
            event_base_free(_evbase);
            _evbase = nullptr;
        }
    }

    virtual void Run() {
        _status.store(ServerStatus::kRunning);
        _tid = std::this_thread::get_id();
        auto rc = event_base_loop(_evbase, EVLOOP_NO_EXIT_ON_EMPTY);
        _status.store(ServerStatus::kStopped);
    }

    virtual int Stop() {
        return event_base_loopexit(_evbase, nullptr);
    }

    struct event_base* event_base() {
        return _evbase;
    }

    bool RunInLoop(std::function<void()> fn, int aftersec = 0) {
        auto ew = new EventWrap();
        ew->fn = std::move(fn);
        timeval tv{(long)aftersec, 0};
        auto code = event_base_once(_evbase, -1, EV_TIMEOUT, OnEvent, (void*)ew, &tv);
        return code == 0;
    }

  protected:
    struct EventWrap {
        std::function<void()> fn;
    };

    static void OnEvent(evutil_socket_t fd, short what, void* arg) {
        auto ew = (EventWrap*)arg;
        if (ew->fn) {
            ew->fn();
        }
        delete ew;
    }

  private:
    std::atomic<ServerStatus::Status> _status;
    struct event_base* _evbase;
    std::thread::id _tid;
};

using EventLoopPtr = std::shared_ptr<EventLoop>;

class EventLoopThread : public EventLoop {
  public:
    EventLoopThread()
        : EventLoop() {
    }
    ~EventLoopThread() {
        Join();
    }
    void Start() {
        _thread.reset(new std::thread(std::bind(&EventLoopThread::Run, this)));
    }

    void Join() {
        std::lock_guard<std::mutex> guard(_mutex);
        if (_thread && _thread->joinable()) {
            try {
                _thread->join();
            } catch (const std::system_error& e) {
                // std::cout << e.what() << std::endl;
            }
        }
        _thread.reset();
    }

  private:
    std::mutex _mutex;
    std::shared_ptr<std::thread> _thread;
};
using EventLoopThreadPtr = std::shared_ptr<EventLoopThread>;

}