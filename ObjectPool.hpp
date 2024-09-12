#pragma once

#include <stack>
#include <memory>
#include <functional>

namespace xs {

template <typename T>
class ObjectPoolT {
  public:
    using ClearFunc = std::function<void(T&)>;
    ObjectPoolT() {}
    ObjectPoolT(ClearFunc clear) : _cleaner(clear) {}

    std::shared_ptr<T> Alloc() {
        T* res = nullptr;

        if (_cache.empty()) {
            res = new T;
        } else {
            res = _cache.top();
            _cache.pop();
        }

        static auto deleter = [&](T* res) {
            if (_cache.size() >= _cache_max_cnt) {
                delete res;
                return;
            }

            if (_cleaner) {
                _cleaner(*res);
            }
            _cache.push(res);
        };

        return std::shared_ptr<T>(res, deleter);
    }

    size_t Size() const {
        return _cache.size();
    }

    void SetCacheMaxCnt(size_t t) {
        _cache_max_cnt = t;
    }

  private:
    size_t _cache_max_cnt = 100;
    std::stack<T*> _cache;
    ClearFunc _cleaner = nullptr;
};
} // namespace xs