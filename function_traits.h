#pragma once
#include <functional>

namespace xs {

template <typename T>
struct FunctinoTraits;

template <typename TRet, typename... TArgs>
struct FunctinoTraits<std::function<TRet(TArgs...)>> {
    static const size_t NArgs = sizeof...(TArgs);

    using ArgsTuple = std::tuple<TArgs...>;
    using ResultType = TRet;

    template <size_t i>
    struct Arg {
        typedef typename std::tuple_element<i, ArgsTuple>::type type;
    };
};

template <typename TClass, typename TRet, typename... TArgs>
struct FunctinoTraits<TRet (TClass::*)(TArgs...)> {
    static const size_t NArgs = sizeof...(TArgs);

    using ResultType = TRet;
    using ArgsTuple = std::tuple<TArgs...>;

    template <size_t i>
    struct Arg {
        typedef typename std::tuple_element<i, ArgsTuple>::type type;
    };
};
}