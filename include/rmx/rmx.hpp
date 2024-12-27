#pragma once
#include <exception>
#include <functional>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <type_traits>
#include <utility>

namespace rmx {

//! An RAII-style guard wrapping a reference to some type protected by a mutex
//!
//! Acquire a `MutexGuard` by locking a `Mutex`.
template<typename ValueT, typename MutexImplT>
class MutexGuard
{
  public:
    explicit MutexGuard(ValueT& value_ref,
                        std::unique_lock<MutexImplT>&& lock,
                        bool& was_poisoned) noexcept :
        m_lock(std::move(lock)), m_ref(value_ref), m_was_poisoned(was_poisoned)
    {
    }

    explicit MutexGuard(MutexGuard&&) noexcept = default;
    MutexGuard& operator=(MutexGuard&&) noexcept = default;

    //! A MutexGuard represents a locked value, which means it's incorrect to copy it around.
    MutexGuard(const MutexGuard&) = delete;
    MutexGuard& operator=(const MutexGuard&) = delete;

    //! If this guard, which represents locked data, is destructed when an exception was thrown,
    //! then that means whatever transaction that was expected to be performed while it was locked,
    //! was unfinished, leaving the locked data in an indeterminate state.
    virtual ~MutexGuard()
    {
        if (std::uncaught_exceptions() != 0)
        {
            // TODO: A possible enhancement is to stash the thread::id or possibly the
            // exception.what() so that it can be referenced in the poison exception.
            m_was_poisoned.get() = true;
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
    std::reference_wrapper<bool> m_was_poisoned;
};

//! A Rust-inspired mutex that wraps some other type.
template<typename ValueT, typename MutexImplT = std::mutex>
class Mutex
{
  public:
    //! Take ownership of an existing @p ValueT
    explicit Mutex(ValueT&& value) noexcept : m_value(std::move(value)) {}

    //! Construct a new @p ValueT from the given args, includes default constructor
    //!
    //! @note POD types need to have a constructor or be passed directly.
    template<typename... ArgsT,
             typename std::enable_if_t<std::is_constructible_v<ValueT, ArgsT...>, bool> = true>
    explicit Mutex(ArgsT&&... args) : m_value{std::forward<ArgsT>(args)...}
    {
    }

    //! Lock the mutex and return an RAII guard controlling access to the underlying value
    //!
    //! @throws std::runtime_error if the Mutex was locked while an exception was thrown. If this
    //! happens, it means that whatever transaction the lock was protecting was left unfinished,
    //! leaving the locked data in an indeterminate state.
    [[nodiscard]] MutexGuard<ValueT, MutexImplT> lock() noexcept(false)
    {
        throw_if_poisoned();
        return lock_unchecked();
    }

    //! Lock the mutex and return an RAII guard controlling access to the underlying value
    //!
    //! @note Depending on the particular @p MutexImplT implementation, locking the mutex might
    //! throw an exception. For most mutexes, this won't throw.
    [[nodiscard]] MutexGuard<ValueT, MutexImplT> lock_unchecked() noexcept
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
        throw_if_poisoned();
        return try_lock_unchecked();
    }

    //! Attempt to lock the mutex and return an RAII guard controlling access to the underlying
    //! value
    //!
    //! @note Depending on the particular @p MutexImplT implementation, locking the mutex might
    //! throw an exception. For most mutexes, this won't throw.
    [[nodiscard]] std::optional<MutexGuard<ValueT, MutexImplT>> try_lock_unchecked() noexcept
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
    [[nodiscard]] bool is_poisoned() noexcept { return m_was_poisoned; }

  private:
    MutexImplT m_mutex;
    ValueT m_value;
    bool m_was_poisoned = false;

    void throw_if_poisoned() noexcept(false)
    {
        if (is_poisoned())
        {
            throw std::runtime_error("Mutex poisoned: exception thrown while Mutex was locked");
        }
    }
};

}  // namespace rmx
