#pragma once

#include <functional>

namespace xs {

class Defer {
  public:
    Defer(const std::function<void()>&& f)
        : _f(f) {
    }

    ~Defer() {
        if (_f && !_cancel) {
            _f();
            _f = nullptr;
        }
    }

    void Cancel() {
        _cancel = true;
    }

    bool _cancel = false;
    std::function<void()> _f;
};

}