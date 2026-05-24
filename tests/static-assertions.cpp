#include <rmx/rmx.hpp>

#include <mutex>
#include <type_traits>
#include <vector>

namespace {

template<typename ValueT>
using guard_for = rmx::MutexGuard<ValueT, std::mutex>;

// Mutex<T>: not copyable, not movable, destructible.

static_assert(!std::is_copy_constructible_v<rmx::Mutex<int>>);
static_assert(!std::is_copy_assignable_v<rmx::Mutex<int>>);
static_assert(!std::is_move_constructible_v<rmx::Mutex<int>>);
static_assert(!std::is_move_assignable_v<rmx::Mutex<int>>);
static_assert(std::is_destructible_v<rmx::Mutex<int>>);

static_assert(!std::is_copy_constructible_v<rmx::Mutex<std::vector<int>>>);
static_assert(!std::is_move_constructible_v<rmx::Mutex<std::vector<int>>>);

static_assert(!std::is_copy_constructible_v<rmx::Mutex<int, std::recursive_mutex>>);
static_assert(!std::is_move_constructible_v<rmx::Mutex<int, std::recursive_mutex>>);

// MutexGuard: not copyable, not movable, destructible.

static_assert(!std::is_copy_constructible_v<guard_for<int>>);
static_assert(!std::is_copy_assignable_v<guard_for<int>>);
static_assert(!std::is_move_constructible_v<guard_for<int>>);
static_assert(!std::is_move_assignable_v<guard_for<int>>);
static_assert(std::is_destructible_v<guard_for<int>>);

}  // namespace
