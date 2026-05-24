#pragma once
#include <chrono>
#include <mutex>
#include <optional>
#include <type_traits>
#include <utility>

#ifdef RMX_ENABLE_POISONING
    #include <atomic>
    #include <exception>
    #include <stdexcept>
#endif

#if defined(__GNUC__) || defined(__clang__)
    #define RMX_INLINE [[gnu::always_inline]] inline
#elif defined(_MSC_VER)
    #define RMX_INLINE __forceinline
#else
    #define RMX_INLINE inline
#endif

namespace rmx {

//! An RAII-style guard wrapping a reference to some type protected by a mutex
//!
//! Acquire a `MutexGuard` by locking a `Mutex`.
//!
//! @note `MutexGuard` represents a locked `Mutex`, so it is neither movable nor copyable.
template<typename ValueT, typename MutexImplT>
class MutexGuard
{
  public:
    explicit MutexGuard(ValueT& value_ref,
                        std::unique_lock<MutexImplT>&& lock
#ifdef RMX_ENABLE_POISONING
                        ,
                        std::atomic<bool>& was_poisoned
#endif
                        ) noexcept :
        m_lock(std::move(lock)),
        m_ref(&value_ref)
#ifdef RMX_ENABLE_POISONING
        ,
        m_was_poisoned(&was_poisoned),
        m_uncaught_at_entry(std::uncaught_exceptions())
#endif
    {
    }

    MutexGuard(const MutexGuard&) = delete;
    MutexGuard& operator=(const MutexGuard&) = delete;
    MutexGuard(MutexGuard&&) = delete;
    MutexGuard& operator=(MutexGuard&&) = delete;

#ifdef RMX_ENABLE_POISONING
    //! If this guard, which represents locked data, is destructed when an exception was thrown,
    //! then that means whatever transaction that was expected to be performed while it was locked,
    //! was unfinished, leaving the locked data in an indeterminate state.
    ~MutexGuard()
    {
        if (std::uncaught_exceptions() > m_uncaught_at_entry)
        {
            // TODO: A possible enhancement is to stash the thread::id or possibly the
            // exception.what() so that it can be referenced in the poison exception.
            m_was_poisoned->store(true, std::memory_order_relaxed);
        }
    }
#else
    ~MutexGuard() = default;
#endif

    //! Access the underlying value by reference
    //!
    //! @warning It is incorrect to store the reference returned by this operator.
    [[nodiscard]] ValueT& operator*() noexcept { return *m_ref; }
    [[nodiscard]] const ValueT& operator*() const noexcept { return *m_ref; }

    //! Access the underlying value by pointer
    //!
    //! @warning It is incorrect to store the pointer returned by this operator.
    [[nodiscard]] ValueT* operator->() noexcept { return m_ref; }
    [[nodiscard]] const ValueT* operator->() const noexcept { return m_ref; }

    //! Wait on a condition variable using this guard's lock and the given predicate.
    //!
    //! @p CvT can be `std::condition_variable` or `std::condition_variable_any`. The predicate is
    //! invoked with the lock held; it should return true to exit the wait.
    template<typename CvT, typename PredT>
    void wait(CvT& cv, PredT pred)
    {
        cv.wait(m_lock, std::move(pred));
    }

    //! Wait on a condition variable for up to @p timeout, returning false if the timeout fired
    //! before the predicate became true.
    template<typename CvT, typename RepT, typename PeriodT, typename PredT>
    bool wait_for(CvT& cv, const std::chrono::duration<RepT, PeriodT>& timeout, PredT pred)
    {
        return cv.wait_for(m_lock, timeout, std::move(pred));
    }

    //! Wait on a condition variable until @p deadline, returning false if the deadline passed
    //! before the predicate became true.
    template<typename CvT, typename ClockT, typename DurationT, typename PredT>
    bool wait_until(CvT& cv, const std::chrono::time_point<ClockT, DurationT>& deadline, PredT pred)
    {
        return cv.wait_until(m_lock, deadline, std::move(pred));
    }

  private:
    std::unique_lock<MutexImplT> m_lock;
    ValueT* m_ref;
#ifdef RMX_ENABLE_POISONING
    std::atomic<bool>* m_was_poisoned;
    int m_uncaught_at_entry;
#endif
};

//! A Rust-inspired mutex that wraps some other type.
//!
//! @note `Mutex` is neither copyable nor movable.
template<typename ValueT, typename MutexImplT = std::mutex>
class Mutex
{
    static_assert(std::is_object_v<ValueT>, "rmx::Mutex must be able to take ownership of ValueT");

  public:
    //! Construct a new @p ValueT from the given args
    //!
    //! Passing an rvalue reference will take ownership of an existing value.
    template<typename... ArgsT,
             typename std::enable_if_t<std::is_constructible_v<ValueT, ArgsT...>, bool> = true>
    explicit Mutex(ArgsT&&... args) noexcept(std::is_nothrow_constructible_v<ValueT, ArgsT...>) :
        m_value(std::forward<ArgsT>(args)...)
    {
    }

    //! Aggregate-initialize a new @p ValueT from the given args
    //!
    //! Only selected for aggregate types when no matching constructor exists.
    template<typename... ArgsT,
             typename std::enable_if_t<!std::is_constructible_v<ValueT, ArgsT...> &&
                                           std::is_aggregate_v<ValueT>,
                                       bool> = true>
    explicit Mutex(ArgsT&&... args) noexcept(noexcept(ValueT{std::forward<ArgsT>(args)...})) :
        m_value{std::forward<ArgsT>(args)...}
    {
    }

    ~Mutex() = default;

    Mutex(const Mutex&) = delete;
    Mutex& operator=(const Mutex&) = delete;
    Mutex(Mutex&&) = delete;
    Mutex& operator=(Mutex&&) = delete;

    //! Lock the mutex and return an RAII guard controlling access to the underlying value
    //!
    //! @throws std::runtime_error if the Mutex was locked while an exception was thrown. If this
    //! happens, it means that whatever transaction the lock was protecting was left unfinished,
    //! leaving the locked data in an indeterminate state.
    [[nodiscard]] MutexGuard<ValueT, MutexImplT> lock() noexcept(false)
    {
        std::unique_lock<MutexImplT> lock(m_mutex);  // TOCTOU: check for poisoning after lock
#ifdef RMX_ENABLE_POISONING
        if (is_poisoned())
        {
            throw std::runtime_error("Mutex poisoned: exception thrown while Mutex was locked");
        }
        return MutexGuard<ValueT, MutexImplT>(m_value, std::move(lock), m_was_poisoned);
#else
        return MutexGuard<ValueT, MutexImplT>(m_value, std::move(lock));
#endif
    }

    //! Lock the mutex and return an RAII guard controlling access to the underlying value
    //!
    //! @note `noexcept` iff constructing a `std::unique_lock<MutexImplT>` from the underlying
    //! mutex is `noexcept`.
    [[nodiscard]] RMX_INLINE MutexGuard<ValueT, MutexImplT> lock_unchecked() noexcept(
        std::is_nothrow_constructible_v<std::unique_lock<MutexImplT>, MutexImplT&>)
    {
        std::unique_lock<MutexImplT> lock(m_mutex);
#ifdef RMX_ENABLE_POISONING
        return MutexGuard(m_value, std::move(lock), m_was_poisoned);
#else
        return MutexGuard(m_value, std::move(lock));
#endif
    }

    //! Attempt to lock the mutex and return an RAII guard controlling access to the underlying
    //! value
    //!
    //! @throws std::runtime_error if the Mutex was locked while an exception was thrown. If this
    //! happens, it means that whatever transaction the lock was protecting was left unfinished,
    //! leaving the locked data in an indeterminate state.
    [[nodiscard]] std::optional<MutexGuard<ValueT, MutexImplT>> try_lock() noexcept(false)
    {
        std::unique_lock<MutexImplT> lock(m_mutex, std::try_to_lock);
        if (!lock)
        {
            return std::nullopt;
        }
#ifdef RMX_ENABLE_POISONING
        if (is_poisoned())
        {
            throw std::runtime_error("Mutex poisoned: exception thrown while Mutex was locked");
        }
        return std::optional<MutexGuard<ValueT, MutexImplT>>(
            std::in_place, m_value, std::move(lock), m_was_poisoned);
#else
        return std::optional<MutexGuard<ValueT, MutexImplT>>(
            std::in_place, m_value, std::move(lock));
#endif
    }

    //! Attempt to lock the mutex and return an RAII guard controlling access to the underlying
    //! value
    //!
    //! @note `noexcept` iff constructing a `std::unique_lock<MutexImplT>` from the underlying
    //! mutex with `std::try_to_lock` is `noexcept`.
    [[nodiscard]] RMX_INLINE std::optional<MutexGuard<ValueT, MutexImplT>>
    try_lock_unchecked() noexcept(std::is_nothrow_constructible_v<std::unique_lock<MutexImplT>,
                                                                  MutexImplT&,
                                                                  std::try_to_lock_t>)
    {
        std::unique_lock<MutexImplT> maybe_lock(m_mutex, std::try_to_lock);
        if (maybe_lock)
        {
#ifdef RMX_ENABLE_POISONING
            return std::optional<MutexGuard<ValueT, MutexImplT>>(
                std::in_place, m_value, std::move(maybe_lock), m_was_poisoned);
#else
            return std::optional<MutexGuard<ValueT, MutexImplT>>(
                std::in_place, m_value, std::move(maybe_lock));
#endif
        }
        return std::nullopt;
    }

#ifdef RMX_ENABLE_POISONING
    //! Indicates whether this Mutex has been poisoned
    [[nodiscard]] RMX_INLINE bool is_poisoned() noexcept
    {
        return m_was_poisoned.load(std::memory_order_relaxed);
    }
#endif

  private:
    MutexImplT m_mutex;
    ValueT m_value;
#ifdef RMX_ENABLE_POISONING
    std::atomic<bool> m_was_poisoned{false};
#endif
};

// Deduction guide: `rmx::Mutex(value)` deduces `Mutex<std::decay_t<decltype(value)>>`.
template<typename ValueT>
Mutex(ValueT&&) -> Mutex<std::decay_t<ValueT>>;

}  // namespace rmx
