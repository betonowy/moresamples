#pragma once

#include <exception>
#include <utility>

#include <cassert>

#define _CONCAT(a, b) a##b
#define CONCAT(a, b) _CONCAT(a, b)

namespace utl {
template <typename F> struct Defer {
    Defer(F f) : f(std::move(f)) {}
    ~Defer() { f(); }

    Defer(const Defer &) = delete;
    Defer(Defer &&) = delete;
    Defer &operator=(const Defer &) = delete;
    Defer &operator=(Defer &&) = delete;

private:
    F f;
};

#define DEFER(...) auto CONCAT(__defer__, __LINE__) = ::utl::Defer([&] { __VA_ARGS__; })
#define ERRDEFER(...) DEFER(if (std::uncaught_exceptions()) { __VA_ARGS__; })

#ifdef APP_DEBUG
inline static constexpr bool DebugMode = true;
#else
inline static constexpr bool DebugMode = false;
#endif

template <typename T> T *assertNotNull(T *ptr) {
    assert(ptr);
    return ptr;
}
} // namespace utl
