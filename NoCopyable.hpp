#pragma once

namespace xs {
class NoCopyable {
  public:
    NoCopyable() {}

    ~NoCopyable() {}

    NoCopyable(const NoCopyable&) = delete;

    NoCopyable& operator=(const NoCopyable&) = delete;

    NoCopyable(const NoCopyable&&) = delete;

    NoCopyable& operator==(const NoCopyable&&) = delete;
};
}