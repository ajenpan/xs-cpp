#pragma once

#include <type_traits>
#include "is_detected.h"

namespace xs {
#define GENERATE_HAS_MEMBER_FUNC(func_name)                     \
    template <typename T>                                       \
    using has_##func_name##_func_t = decltype(&T::##func_name); \
    template <typename T>                                       \
    using has_##func_name##_func = is_detected<Has_##func_name##_func_t, T>
}
