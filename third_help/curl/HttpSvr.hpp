#pragma once

#include <thread>
#include <unordered_map>
#include <functional>
#include <future>
#include <atomic>

#include "TaskPool.hpp"
#include "Defer.hpp"

#include "Helper.h"
#include "EventLoop.hpp"

namespace base_on_libevent {
class HttpSvr {
  public:
    class Request {
      public:
        Request(evhttp_request* req) {
            _req = req;

            path = evhttp_uri_get_path(evhttp_request_get_evhttp_uri(req));
            method = evhttp_request_get_command(req);

            //read body
            auto buf = evhttp_request_get_input_buffer(req);
            auto buflen = evbuffer_get_length(buf);
            auto pRaw = evbuffer_pullup(buf, buflen);
            body.assign((char*)pRaw, buflen);
        }

        void ReadHeader() {
            auto hs = evhttp_request_get_input_headers(_req);
            for (auto header = hs->tqh_first; header; header = header->next.tqe_next) {
                headers[header->key] = header->value;
            }
        }

        void ReadBodyTo(std::string& body) {
            body = this->body;
            // auto buf = evhttp_request_get_input_buffer(_req);
            // int n = 0;
            // char cbuf[128] = {0};
            // while (evbuffer_get_length(buf) > 0) {
            //     n = evbuffer_remove(buf, cbuf, sizeof(cbuf));
            //     if (n <= 0) {
            //         break;
            //     }
            //     body.append(cbuf, n);
            // }
        }

        std::string body;
        std::unordered_map<std::string, std::string> headers;
        uint32_t method = 0;
        std::string path;
        evhttp_request* _req = nullptr;
    };

    class Response {
      public:
        std::unordered_map<std::string, std::string> header;
        uint32_t status = 0;
        std::string body;
    };

    using ServerHttpFunc = std::function<void(Request&, Response&)>;

    HttpSvr(int threadnum, EventLoop* loop = nullptr) {
        _run_status = 0;
        _loop = loop;
        _taskPool = new xs::TaskPool(threadnum);
        _createEVBaseLocal = _loop == nullptr;
        if (_createEVBaseLocal) {
            auto pThread = new EventLoopThread();
            pThread->Start();
            _loop = pThread;
        }
    }

    virtual ~HttpSvr() {
        Stop();
    }

    void Stop() {
        auto status = _run_status.exchange(0);
        if (status != 1) {
            return;
        }
        if (_createEVBaseLocal && _loop != nullptr) {
            _loop->Stop();
            _loop = nullptr;
        }
    }

    bool Start(uint32_t port, ServerHttpFunc cb) {
        auto status = _run_status.exchange(1);
        if (status != 0) {
            return false;
        }

        _cbfunc = cb;

        static std::string addr = "0.0.0.0";

        auto pHttpRaw = evhttp_new(_loop->event_base());
        evhttp_set_gencb(pHttpRaw, _handle, this);

        _evhttp = std::shared_ptr<evhttp>(pHttpRaw, [](evhttp* p) {
            evhttp_free(p);
        });

        auto handle = evhttp_bind_socket_with_handle(pHttpRaw, addr.c_str(), port);
        if (handle == nullptr) {
            return false;
        }
        return true;
    }

  private:
    void ServerHttp(std::shared_ptr<Request>& req, std::shared_ptr<Response>& resp, std::function<void()> cb) {
        if (_cbfunc) {
            _cbfunc(*req, *resp);
        } else {
            resp->status = 404;
        }

        _loop->RunInLoop(cb);
    }

    void OnHandle(struct evhttp_request* req) {
        std::shared_ptr<Request> pReq = std::make_shared<Request>(req);
        std::shared_ptr<Response> pResp = std::make_shared<Response>();

        // pReq->ReadBodyTo
        // while (evbuffer_get_length(buf) > 0) {
        //     n = evbuffer_remove(buf, cbuf, sizeof(cbuf));
        //     if (n <= 0) {
        //         break;
        //     }
        //     body.append(cbuf, n);
        // }

        auto cb = [pReq, pResp]() {
            auto& req = pReq->_req;
            if (pResp->status == 0) {
                pResp->status = 200;
            } else if (pResp->status < 0) {
                pResp->status = 500;
            }

            if (pResp->body.size() > 0) {
                auto out = evhttp_request_get_output_buffer(req);
                evbuffer_add(out, pResp->body.c_str(), pResp->body.size());
            }

            if (pResp->header.size() > 0) {
                auto hs = evhttp_request_get_output_headers(req);
                for (auto& it : pResp->header) {
                    evhttp_add_header(hs, it.first.c_str(), it.second.c_str());
                }
            }
            evhttp_send_reply(req, pResp->status, NULL, NULL);
        };

        _taskPool->PushTask(std::bind(&HttpSvr::ServerHttp, this, pReq, pResp, cb));
    }

    static void _handle(struct evhttp_request* req, void* h) {
        auto p = (HttpSvr*)h;
        p->OnHandle(req);
    }

    xs::TaskPool* _taskPool;

    EventLoop* _loop = nullptr;
    std::atomic<uint8_t> _run_status;
    ServerHttpFunc _cbfunc = nullptr;
    bool _createEVBaseLocal = false;
    std::shared_ptr<evhttp> _evhttp = nullptr;
};
}