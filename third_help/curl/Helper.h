#pragma once

#include <atomic>
#include <memory>
#include <string>

#if _WIN32
#pragma comment(lib, "ws2_32.lib")   // Socket编程需用的动态链接库
#pragma comment(lib, "Kernel32.lib") // IOCP需要用到的动态链接库
#pragma comment(lib, "mswsock.lib")
#else
#include <arpa/inet.h>
#endif

extern "C" {
#include <event2/event.h>
#include <event2/buffer.h>
#include <event2/bufferevent.h>
#include <event2/event_struct.h>
#include <event2/thread.h>
#include <event2/util.h>
#include <event2/http.h>
#include <event2/listener.h>
}

namespace base_on_libevent {

using EventBasePtr = std::shared_ptr<event_base>;

using TCPSocketID = uint64_t;

class Helper {
  public:
    static bool Init() {
        bool ok = true;
        ok = ok && InitNetwork();
        ok = ok && InitThread();
        return ok;
    }

    static bool InitThread() {
        // TODO:
        // static std::once_flag flag;
        // std::call_once(flag, []() {} -> int);

        int nInitThread = -1;
#ifdef _WIN32
        nInitThread = evthread_use_windows_threads();
#else
        // #ifdef EVENT__HAVE_PTHREADS
        nInitThread = evthread_use_pthreads();
        // #endif
#endif
        return nInitThread == 0;
    }

    static bool InitNetwork() {
#ifdef _WIN32
        WSADATA wsa_data;
        return WSAStartup(0x0201, &wsa_data) == 0;
#else
        return true;
#endif
    }

    static EventBasePtr CreateEventBase() {
        return EventBasePtr(::event_base_new(), [](event_base* pBase) {
            event_base_free(pBase);
        });
    }

    static int EventBaseDispatch(EventBasePtr base) {
        return event_base_loop(base.get(), EVLOOP_NO_EXIT_ON_EMPTY);
    }

    static int EventBaseLoopexit(EventBasePtr base) {
        return event_base_loopexit(base.get(), nullptr);
    }

    static std::tuple<std::string, int32_t> SpilitHostPort(const std::string addr) {
        std::string host;
        int32_t port = 0;
        auto nFind = addr.rfind(":");
        if (nFind != std::string::npos) {
            host = addr.substr(0, nFind);
            port = atoi(addr.c_str() + nFind + 1);
        }
        return std::make_tuple(host, port);
    }

    static TCPSocketID GenSocketID() {
        static std::atomic<uint32_t> s_ID(0);
        auto low = (uint64_t)++s_ID;
        auto high = (uint64_t)std::time(nullptr);
        return (high << 32) | low;
    }

    //static std::shared_ptr<evconnlistener> NewConnListener() {
    //    auto p = evconnlistener_new_bind(base, NULL, NULL, LEV_OPT_CLOSE_ON_FREE, -1, (struct sockaddr*)&addr, sizeof(addr));
    //    return std::shared_ptr<evconnlistener>(p, [](evconnlistener* p) { evconnlistener_free(p); });
    //}
};
} // namespace base_on_libevent
