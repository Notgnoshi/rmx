#pragma once
#include <atomic>
#include <exception>
#include <functional>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <type_traits>
#include <utility>

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
                        std::unique_lock<MutexImplT>&& lock,
                        std::atomic<bool>& was_poisoned) noexcept :
        m_lock(std::move(lock)),
        m_ref(value_ref),
        m_was_poisoned(was_poisoned),
        m_uncaught_at_entry(std::uncaught_exceptions())
    {
    }

    MutexGuard(const MutexGuard&) = delete;
    MutexGuard& operator=(const MutexGuard&) = delete;
    MutexGuard(MutexGuard&&) = delete;
    MutexGuard& operator=(MutexGuard&&) = delete;

    //! If this guard, which represents locked data, is destructed when an exception was thrown,
    //! then that means whatever transaction that was expected to be performed while it was locked,
    //! was unfinished, leaving the locked data in an indeterminate state.
    ~MutexGuard()
    {
        if (std::uncaught_exceptions() > m_uncaught_at_entry)
        {
            // TODO: A possible enhancement is to stash the thread::id or possibly the
            // exception.what() so that it can be referenced in the poison exception.
            m_was_poisoned.get().store(true, std::memory_order_relaxed);
        }
    }

    //! Access the underlying value by reference
    //!
    //! @warning It is incorrect to store the reference returned by this operator.
    [[nodiscard]] ValueT& operator*() noexcept { return m_ref; }
    [[nodiscard]] const ValueT& operator*() const noexcept { return m_ref; }

    //! Access the underlying value by pointer
    //!
    //! @warning It is incorrect to store the pointer returned by this operator.
    [[nodiscard]] ValueT* operator->() noexcept { return &m_ref.get(); }
    [[nodiscard]] const ValueT* operator->() const noexcept { return &m_ref.get(); }

    //! Provide access to the underlying std::unique_lock to facilitate use with
    //! std::condition_variable::wait()
    //!
    //! @warning It is incorrect to access the wrapped value if the inner lock has been manually
    //! unlocked. Don't do that.
    [[nodiscard]] std::unique_lock<MutexImplT>& inner() noexcept { return m_lock; }

  private:
    std::unique_lock<MutexImplT> m_lock;
    std::reference_wrapper<ValueT> m_ref;
    std::reference_wrapper<std::atomic<bool>> m_was_poisoned;
    int m_uncaught_at_entry;
};

//! A Rust-inspired mutex that wraps some other type.
//!
//! @note `Mutex` is neither copyable nor movable.
template<typename ValueT, typename MutexImplT = std::mutex>
class Mutex
{
    static_assert(std::is_object_v<ValueT>, "rmx::Mutex must be able to take ownership of ValueT");

  public:
    //! Take ownership of an existing @p ValueT
    explicit Mutex(ValueT&& value) noexcept(std::is_nothrow_move_constructible_v<ValueT>) :
        m_value(std::move(value))
    {
    }

    //! Construct a new @p ValueT from the given args, includes default constructor
    //!
    //! @note POD types need to have a constructor or be passed directly.
    template<typename... ArgsT,
             typename std::enable_if_t<std::is_constructible_v<ValueT, ArgsT...>, bool> = true>
    explicit Mutex(ArgsT&&... args) : m_value{std::forward<ArgsT>(args)...}
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
        if (is_poisoned())
        {
            throw std::runtime_error("Mutex poisoned: exception thrown while Mutex was locked");
        }
        return MutexGuard<ValueT, MutexImplT>(m_value, std::move(lock), m_was_poisoned);
    }

    //! Lock the mutex and return an RAII guard controlling access to the underlying value
    //!
    //! @note `noexcept` iff constructing a `std::unique_lock<MutexImplT>` from the underlying
    //! mutex is `noexcept`.
    [[nodiscard]] RMX_INLINE MutexGuard<ValueT, MutexImplT> lock_unchecked() noexcept(
        std::is_nothrow_constructible_v<std::unique_lock<MutexImplT>, MutexImplT&>)
    {
        std::unique_lock<MutexImplT> lock(m_mutex);
        return MutexGuard(m_value, std::move(lock), m_was_poisoned);
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
        if (is_poisoned())
        {
            throw std::runtime_error("Mutex poisoned: exception thrown while Mutex was locked");
        }
        return std::optional<MutexGuard<ValueT, MutexImplT>>(
            std::in_place, m_value, std::move(lock), m_was_poisoned);
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
            return std::optional<MutexGuard<ValueT, MutexImplT>>(
                std::in_place, m_value, std::move(maybe_lock), m_was_poisoned);
        }
        return std::nullopt;
    }

    //! Indicates whether this Mutex has been poisoned
    [[nodiscard]] RMX_INLINE bool is_poisoned() noexcept
    {
        return m_was_poisoned.load(std::memory_order_relaxed);
    }

  private:
    MutexImplT m_mutex;
    ValueT m_value;
    std::atomic<bool> m_was_poisoned{false};
};

}  // namespace rmx
