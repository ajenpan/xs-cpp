#pragma once

#include <atomic>
#include <map>
#include <memory>

#include <event2/listener.h>

#include "TCPConn.hpp"

namespace base_on_libevent {

class TCPServer : public CallbackBase {
  public:
    using AutoLock = std::lock_guard<std::mutex>;

    TCPServer(uint16_t workercnt = 1)
        : _threads(workercnt) {
        _threads.Start();
    }

    virtual ~TCPServer() {
        _threads.Stop();
        Stop();
    }

    int Start(std::string strAddr, EventLoopPtr pBase = nullptr) {
        auto [strIP, nPort] = Helper::SpilitHostPort(strAddr);
        _listenAddr.sin_family = AF_INET;
        _listenAddr.sin_port = htons(nPort);
        _listenAddr.sin_addr.s_addr = strIP.empty() ? 0 : inet_addr(strIP.c_str());

        _pBase = pBase;
        _bCreateBase = _pBase == nullptr;

        if (_bCreateBase) {
            auto pTemp = std::make_shared<EventLoopThread>();
            pTemp->Start();
            _pBase = pTemp;
        }

        _listener = ::evconnlistener_new_bind(_pBase->event_base(), &TCPServer::connlistener_cb, this, LEV_OPT_CLOSE_ON_FREE, -1, (sockaddr*)&_listenAddr, sizeof(_listenAddr));

        if (!_listener) {
            SocketError("could not create a listener!");
            return -1;
        }

        // get real listen address
        socklen_t addr_len = sizeof(_listenAddr);
        getsockname(evconnlistener_get_fd(_listener), (struct sockaddr*)&_listenAddr, &addr_len);

        _bRun.store(true);
        return ntohs(_listenAddr.sin_port);
    }

    EventLoopPtr EventBase() {
        return _pBase;
    }

    void Stop() {
        if (_bRun.exchange(false) == false) {
            return;
        }

        if (_pBase == nullptr) {
            return;
        }
        _pBase->RunInLoop([this]() {
            if (this->_listener) {
                evconnlistener_free(this->_listener);
                this->_listener = nullptr;
            }
        });
        if (_bCreateBase) {
            _pBase->Stop();
        }
        _pBase = nullptr;
    }

    void* GetIOSocketData(TCPSocketID id) {
        auto conn = GetConn(id);
        if (conn == nullptr) {
            return nullptr;
        }
        return conn->GetUserData();
    }
    void SetIOSocketData(TCPSocketID id, void* ud) {
        auto conn = GetConn(id);
        if (conn == nullptr) {
            return;
        }
        conn->SetUserData(ud);
    }

    void CloseIOSocket(TCPSocketID id) {
        auto conn = GetConn(id);
        if (conn == nullptr) {
            return;
        }
        conn->Close();
    }

    bool Send(TCPSocketID id, const char* data, size_t len) {
        auto conn = GetConn(id);
        if (conn == nullptr) {
            return false;
        }
        return conn->Send(data, len);
    }

    TCPConnPtr GetConn(TCPSocketID id) {
        std::lock_guard<std::mutex> lg(_kConnLock);
        auto it = _mapConns.find(id);
        if (it == _mapConns.end()) {
            return nullptr;
        }
        return it->second;
    }

    void RemoveConn(TCPSocketID id) {
        std::lock_guard<std::mutex> lg(_kConnLock);
        auto it = _mapConns.find(id);
        if (it != _mapConns.end()) {
            auto& conn = it->second;
            _mapConns.erase(it);
        }
    }

    size_t ConnCount() {
        std::lock_guard<std::mutex> lg(_kConnLock);
        return _mapConns.size();
    }

    EventLoopPtr NextWorker() {
        if (_threads.Size() == 0) {
            return _pBase;
        }
        return _threads.GetNextLoop();
    }

  protected:
    void OnError(const std::string& msg) {
        std::cout << "OnError:" << msg << std::endl;
    }

    void SocketError(const std::string& msg) {
        int errcode = EVUTIL_SOCKET_ERROR();
        auto errstr = evutil_socket_error_to_string(errcode);
        std::cout << "sockmsg:" << errstr << ",sockcode:" << errcode << "\n";
        OnError(msg);
    }

    void AddConn(TCPSocketID id, TCPConnPtr pConn) {
        std::lock_guard<std::mutex> lg(_kConnLock);
        auto kInsert = _mapConns.insert(std::make_pair(id, pConn));
        if (!kInsert.second) {
            OnError("AddConn failed!");
        }
    }

    static void connlistener_cb(struct evconnlistener* listener, evutil_socket_t fd, struct sockaddr* sa, int socklen, void* ud) {
        TCPServer* pSvr = static_cast<TCPServer*>(ud);
        auto evbase = pSvr->NextWorker();

        evbase->RunInLoop([pSvr, evbase, fd, sa, socklen]() {
            if (pSvr->onAccept) {
                bool accept = pSvr->onAccept(fd, sa, socklen);

                if (!accept) {
                    evutil_closesocket(fd);
                    return;
                }
            }

            auto id = TCPConn::GenSocketID();

            const static int nodelay = 1;
            int n = setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (const char*)&nodelay, sizeof(nodelay));

            auto evbuff = bufferevent_socket_new(evbase->event_base(), fd, BEV_OPT_CLOSE_ON_FREE | BEV_OPT_THREADSAFE);
            if (evbuff == nullptr) {
                pSvr->OnError("bufferevent_socket_new");
                return;
            }

            auto pClient = std::make_shared<TCPConn>(evbase, evbuff);

            pClient->_id = id;
            pClient->_evbuff = evbuff;
            pClient->_evbase = evbase;
            pClient->onFliterStream = pSvr->onFliterStream;
            pClient->onConnRecv = pSvr->onConnRecv;
            pClient->onConnEnable = [pSvr](TCPConnPtr conn, bool enable) {
                if (pSvr->onConnEnable) {
                    pSvr->onConnEnable(conn, enable);
                }

                if (!enable) {
                    if (conn->_evbuff) {
                        // auto fd = bufferevent_getfd(conn->_evbuff);
                        // evutil_closesocket(fd);
                        bufferevent_free(conn->_evbuff);
                        conn->_evbuff = nullptr;
                    }

                    pSvr->RemoveConn(conn->ID());
                }
            };

            pSvr->AddConn(id, pClient);

            bufferevent_enable(pClient->_evbuff, EV_READ | EV_WRITE);
            pClient->_enable.store(true);
            pClient->OnConnEnable();
        });
    }

    bool _bCreateBase = false;
    EventLoopPtr _pBase = nullptr;
    evconnlistener* _listener = nullptr;
    sockaddr_in _listenAddr;

    EventLoopThreadPool _threads;

    std::atomic_bool _bRun;

    std::mutex _kConnLock;
    std::map<TCPSocketID, TCPConnPtr> _mapConns;
};
}; // namespace base_on_libevent
