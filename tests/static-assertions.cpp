#include <rmx/rmx.hpp>

#include <mutex>
#include <shared_mutex>
#include <type_traits>
#include <vector>

namespace {

template<typename ValueT>
using guard_for = rmx::MutexGuard<ValueT, std::mutex>;

struct Aggregate
{
    int a;
    int b;
};

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

// noexcept behavior.
//
// Mutex(ValueT&&) is noexcept iff ValueT's move ctor is noexcept. int qualifies.
static_assert(noexcept(rmx::Mutex<int>(0)));

static_assert(!noexcept(std::declval<rmx::Mutex<int, std::mutex>&>().lock_unchecked()));
static_assert(!noexcept(std::declval<rmx::Mutex<int, std::recursive_mutex>&>().lock_unchecked()));
static_assert(!noexcept(std::declval<rmx::Mutex<int, std::timed_mutex>&>().lock_unchecked()));
static_assert(!noexcept(std::declval<rmx::Mutex<int, std::shared_mutex>&>().lock_unchecked()));

static_assert(!noexcept(std::declval<rmx::Mutex<int, std::mutex>&>().try_lock_unchecked()));
static_assert(
    !noexcept(std::declval<rmx::Mutex<int, std::recursive_mutex>&>().try_lock_unchecked()));
static_assert(!noexcept(std::declval<rmx::Mutex<int, std::timed_mutex>&>().try_lock_unchecked()));
static_assert(!noexcept(std::declval<rmx::Mutex<int, std::shared_mutex>&>().try_lock_unchecked()));

// is_poisoned() is callable on a const Mutex&.
static_assert(noexcept(std::declval<const rmx::Mutex<int>&>().is_poisoned()));

// Poisoned inherits std::runtime_error so callers can catch via the standard hierarchy.
static_assert(std::is_base_of_v<std::runtime_error, rmx::Poisoned>);

// Construction.
//
// The forwarding ctor uses paren-init for constructible types and brace-init only as a fallback
// for aggregates.
static_assert(std::is_constructible_v<rmx::Mutex<int>>);
static_assert(std::is_constructible_v<rmx::Mutex<int>, int>);
static_assert(std::is_constructible_v<rmx::Mutex<std::vector<int>>, int>);
static_assert(std::is_constructible_v<rmx::Mutex<std::vector<int>>, std::size_t, int>);
static_assert(std::is_constructible_v<rmx::Mutex<Aggregate>>);
static_assert(std::is_constructible_v<rmx::Mutex<Aggregate>, int, int>);

}  // namespace
