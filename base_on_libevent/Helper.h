#pragma once

#include <atomic>
#include <memory>
#include <string>
#include <ctime>
#include <vector>

#if _WIN32
#pragma comment(lib, "ws2_32.lib") // Socket 编程需用的动态链接库
#pragma comment(lib, "mswsock.lib")
#pragma comment(lib, "Kernel32.lib") // IOCP 需要用到的动态链接库
#else
#include <arpa/inet.h>
#endif

namespace base_on_libevent {

class Helper {
  public:
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

    static void StringSplit(const std::string& str, const std::string& delims, std::vector<std::string>& ret) {
        if (str.empty() || str.size() < delims.size()) {
            return;
        }

        // Use STL methods
        size_t start = 0, pos = 0;

        do {
            pos = str.find_first_of(delims, start);
            if (pos == start) {
                ret.emplace_back(std::string());
                start = pos + delims.size();
            } else if (pos == std::string::npos) {
                // Copy the rest of the string
                ret.emplace_back(std::string(str.data() + start, str.size() - start));
                break;
            } else {
                // Copy up to delimiter
                ret.emplace_back(std::string(str.data() + start, pos - start));
                start = pos + delims.size();
            }

        } while (pos != std::string::npos);
    }
};
} // namespace base_on_libevent
