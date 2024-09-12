#pragma once
#include <functional>
#include <memory>
#include <mutex>
#include <string>

#include "EventLoop.hpp"

namespace base_on_libevent {

using TCPSocketID = uint64_t;

class TCPConn;
using TCPConnPtr = std::shared_ptr<TCPConn>;

typedef std::function<bool(evutil_socket_t fd, struct sockaddr* sa, int socklen)> funcOnAccept;
typedef std::function<void(TCPConnPtr conn, bool enable)> funcOnConnEnable;
typedef std::function<void(TCPConnPtr conn, const char* data, uint32_t len)> funcOnDataRecv;
typedef std::function<int32_t(TCPConnPtr conn, const char* data, uint32_t len)> funcFliterStream;

struct CallbackBase {
    funcFliterStream onFliterStream = nullptr;
    funcOnDataRecv onConnRecv = nullptr;
    funcOnConnEnable onConnEnable = nullptr;
    funcOnAccept onAccept = nullptr;
};

class TCPConn : public std::enable_shared_from_this<TCPConn>, public CallbackBase {
  public:
    friend class TCPServer;

    TCPConn(EventLoopPtr evbase, bufferevent* bf) : _evbase(evbase), _evbuff(bf) {
        _enable = false;

        _input = bufferevent_get_input(_evbuff);
        _output = bufferevent_get_output(_evbuff);

        bufferevent_setcb(_evbuff, &TCPConn::conn_readcb, nullptr, &TCPConn::conn_eventcb, this);
    }

    virtual ~TCPConn() {}

    static TCPSocketID GenSocketID() {
        static std::atomic<uint32_t> _id_idex(0);
        return ++_id_idex;
        //auto high = (uint64_t)std::time(nullptr);
        //return high << 32 | low;
    }

    std::mutex _ud_lock;
    void* _ud = nullptr;

    void SetUserData(void* ud) {
        std::lock_guard<std::mutex> lock(_ud_lock);
        _ud = ud;
    }

    void* GetUserData() {
        std::lock_guard<std::mutex> lock(_ud_lock);
        return _ud;
    }

    bool Send(const char* pdata, size_t len) {
        if (pdata == nullptr || len == 0) {
            return true;
        }
        if (!_enable.load()) {
            return false;
        }

        //lock
        bufferevent_lock(_evbuff);
        auto n = bufferevent_write(_evbuff, pdata, len);
        bufferevent_unlock(_evbuff);
        return 0 == n;
    }

    const TCPSocketID& ID() {
        return _id;
    }

    void Close() {
        auto bf = _enable.exchange(false);
        if (bf == false) {
            return;
        }

        _evbase->RunInLoop([this]() {
            OnConnEnable();
        });
    }

    //protected:
    void OnConnEnable() {
        if (onConnEnable) {
            onConnEnable(shared_from_this(), _enable.load());
        }
    }

    static void conn_readcb(struct bufferevent* pBev, void* pUserData) {
        auto pClient = static_cast<TCPConn*>(pUserData);
        if (!pClient->_enable.load()) {
            return;
        }
        
        auto& pInput = pClient->_input;

        auto nBufferLen = evbuffer_get_length(pInput);
        const char* pBufferData = (const char*)evbuffer_pullup(pInput, nBufferLen);
        if (pBufferData == nullptr || nBufferLen == 0) {
            return;
        }

        int32_t nPackageLen = 0;
        uint32_t nInvalidLen = 0;

        while (nBufferLen > 0) {
            if (pClient->onFliterStream) {
                nPackageLen = pClient->onFliterStream(pClient->shared_from_this(), pBufferData, nBufferLen);
            } else {
                nPackageLen = nBufferLen;
            }

            if (nPackageLen < 0 || (size_t)nPackageLen > nBufferLen) {
                // error
                pClient->_enable.store(false);
                pClient->OnConnEnable();
                return;
            } else if (nPackageLen == 0) { // data not enough
                break;
            }

            if (pClient->onConnRecv) {
                pClient->onConnRecv(pClient->shared_from_this(), pBufferData, nPackageLen);
            }

            nInvalidLen += nPackageLen;
            nBufferLen -= nPackageLen;
            pBufferData += nPackageLen;
        }

        if (nInvalidLen > 0) {
            evbuffer_drain(pInput, nInvalidLen);
        }
    }

    static void conn_eventcb(struct bufferevent* pBev, short nEvent, void* arg) {
        auto* pClient = static_cast<TCPConn*>(arg);
        // client will set BEV_EVENT_CONNECTED
        auto ok = (nEvent & BEV_EVENT_CONNECTED) == BEV_EVENT_CONNECTED;
        auto oldok = pClient->_enable.exchange(ok);
        if (oldok != ok) {
            pClient->OnConnEnable();
        }
    }

    TCPSocketID _id = 0;
    std::atomic_bool _enable = {false};

    EventLoopPtr _evbase = nullptr;
    bufferevent* _evbuff = nullptr;

    struct evbuffer* _input = nullptr;
    struct evbuffer* _output = nullptr;
};

} // namespace base_on_libevent