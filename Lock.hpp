#pragma once
#include <mutex>
namespace xs {

typedef std::mutex Lock;
typedef std::lock_guard<std::mutex> AutoLock;

}