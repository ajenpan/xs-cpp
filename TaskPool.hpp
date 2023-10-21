#pragma once

#include <thread>
#include <functional>

#include "Queue.hpp"
#include "Signal.hpp"

namespace xs {

class TaskPool {
  public:
    struct TaskWrap {
        enum Tasktype : char {
            eTaskStop = 0,
            eTask = 1,
        };
        Tasktype eType = eTask;
        std::function<void()> func = nullptr;
    };

    TaskPool(int cnt = 1) {
        m_bRun = true;
        for (int i = 0; i < cnt; i++) {
            auto pThread = std::make_unique<std::thread>(std::bind(&TaskPool::OnWork, this));
            m_Threads.emplace_back(std::move(pThread));
        }
    }

    bool PushTask(std::function<void()> func) {
        TaskWrap kItem;
        kItem.func = std::move(func);
        bool bPush = m_queTask.Push(kItem);
        return bPush;
    }

  protected:
    void OnWork() {
        TaskWrap kItem;
        while (m_bRun) {
            if (!m_queTask.Pop(kItem)) {
                continue;
            }

            if (kItem.eType == TaskWrap::eTaskStop) {
                return;
            }
            if (kItem.func) {
                try {
                    kItem.func();
                } catch (std::exception e) {
                    std::cout << e.what() << "\n";
                } catch (...) {
                    //LOG_E("");
                    std::cout << "[catch] receive TaskThread\n";
                }
            }
        }
    }

    Queue<TaskWrap> m_queTask;
    std::atomic<bool> m_bRun = false;
    std::vector<std::unique_ptr<std::thread>> m_Threads;
};
}