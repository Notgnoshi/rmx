#pragma once
#include <functional>
#include <mutex>
#include <optional>
#include <type_traits>

namespace rmx {

//! An RAII-style guard wrapping a reference to some type protected by a mutex
//!
//! Acquire a `MutexGuard` by locking a `Mutex`.
template<typename ValueT, typename MutexImplT>
class MutexGuard
{
  public:
    explicit MutexGuard(ValueT& value_ref, std::unique_lock<MutexImplT>&& lock) noexcept :
        m_lock(std::move(lock)), m_ref(value_ref)
    {
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
    [[nodiscard]] MutexGuard<ValueT, MutexImplT> lock()
    {
        std::unique_lock<MutexImplT> lock(m_mutex);
        return MutexGuard(m_value, std::move(lock));
    }

    //! Attempt to lock the mutex and return an RAII guard controlling access to the underlying
    //! value
    [[nodiscard]] std::optional<MutexGuard<ValueT, MutexImplT>> try_lock()
    {
        std::unique_lock<MutexImplT> maybe_lock(m_mutex, std::try_to_lock);
        if (maybe_lock)
        {
            auto guard = MutexGuard(m_value, std::move(maybe_lock));
            return std::optional(std::move(guard));
        }
        return std::nullopt;
    }

  private:
    MutexImplT m_mutex;
    ValueT m_value;
};

}  // namespace rmx
