#pragma once

#include <functional>

namespace xs {

class Defer {
  public:
    Defer(const std::function<void()>&& f)
        : _f(f) {
    }

    ~Defer() {
        if (_f) {
            _f();
            _f = nullptr;
        }
    }
    std::function<void()> _f;
};

}