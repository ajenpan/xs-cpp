#pragma once

namespace xs {
#if __cplusplus < 201703L
template <typename...>
using void_t = void;
#else
using void_t = std::void_t;
#endif

template <typename, template <typename...> typename Op, typename... T>
struct is_detected_impl : std::false_type {};

template <template <typename...> typename Op, typename... T>
struct is_detected_impl<void_t<Op<T...>>, Op, T...> : std::true_type {};

template <template <typename...> typename Op, typename... T>
using is_detected = is_detected_impl<void, Op, T...>;

}