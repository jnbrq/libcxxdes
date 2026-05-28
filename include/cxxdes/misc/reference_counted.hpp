/**
 * @file reference_counted.hpp
 * @author Canberk Sönmez (canberk.sonmez.409@gmail.com)
 * @brief Utility classes for reference counted objects.
 * @date 2022-08-23
 * 
 * Copyright (c) Canberk Sönmez 2022
 * 
 */

#ifndef CXXDES_MISC_REFERENCE_COUNTED_HPP_INCLUDED
#define CXXDES_MISC_REFERENCE_COUNTED_HPP_INCLUDED

#include <cassert>
#include <type_traits>

namespace cxxdes {
namespace memory {

/**
 * @brief Intrusive reference-counting base class.
 *
 * Derived objects are deleted when `unref()` decrements the count to zero.
 * Reference counts are mutable and not atomic; this type is not thread-safe.
 * 
 * @tparam Derived CRTP parameter.
 */
template <typename Derived>
struct reference_counted_base {
protected:
    reference_counted_base() = default;

    reference_counted_base(const reference_counted_base &) = delete;
    reference_counted_base &operator=(const reference_counted_base &) = delete;

    reference_counted_base(reference_counted_base &&) = delete;
    reference_counted_base &operator=(reference_counted_base &&) = delete;
public:
    /** @brief Increments the intrusive reference count. */
    void ref() const noexcept {
        ++count_;
    }

    /**
     * @brief Decrements the reference count and deletes the object at zero.
     *
     * Debug builds assert if the count is already zero.
     */
    void unref() const {
        assert(count_ > 0 && "object is already destroyed.");
        if (--count_ == 0) {
            delete static_cast<Derived const *>(this);
        }
    }

    /** @brief Returns the current intrusive reference count. */
    std::size_t ref_count() const noexcept {
        return count_;
    }

#ifndef NDEBUG
    ~reference_counted_base() {
        assert(count_ == 0 && "object is destroyed while there are still references to it.");
    }
#endif

private:
    mutable std::size_t count_ = 0;
};

/**
 * @brief Owning pointer for objects derived from `reference_counted_base`.
 *
 * Copying increments the pointee's reference count. Destroying or reassigning a
 * non-null pointer decrements it. This pointer does not use atomic operations.
 * 
 * @tparam T Referenced object type.
 */
template <typename T>
struct ptr {
    ptr() noexcept = default;

    ptr(T *p) noexcept: ptr_{p} {
        if (ptr_) ptr_->ref();
    }

    ptr(const ptr& other) noexcept {
        // noexcept, initially nullptr
        *this = other;
    }

    ptr &operator=(const ptr &other) {
        if (ptr_ != other.ptr_) {
            if (ptr_) ptr_->unref();
            ptr_ = other.ptr_;
            if (ptr_) ptr_->ref();
        }
        return *this;
    }

    ptr(ptr && other) noexcept {
        *this = std::move(other);
    }

    ptr &operator=(ptr &&other) noexcept {
        std::swap(ptr_, other.ptr_);
        return *this;
    }

    T *operator->() noexcept {
        return ptr_;
    }

    T *operator->() const noexcept {
        return ptr_;
    }

#define REFERENCE_COUNTED_PTR_IMPLEMENT_REL_OP(OPNAME) \
    bool operator OPNAME (ptr const &other) const noexcept { \
        return ptr_ OPNAME other.ptr_; \
    }

    REFERENCE_COUNTED_PTR_IMPLEMENT_REL_OP(==)
    REFERENCE_COUNTED_PTR_IMPLEMENT_REL_OP(!=)
    REFERENCE_COUNTED_PTR_IMPLEMENT_REL_OP(>)
    REFERENCE_COUNTED_PTR_IMPLEMENT_REL_OP(>=)
    REFERENCE_COUNTED_PTR_IMPLEMENT_REL_OP(<)
    REFERENCE_COUNTED_PTR_IMPLEMENT_REL_OP(<=)

#undef REFERENCE_COUNTED_PTR_IMPLEMENT_REL_OP

    /** @brief Returns whether this pointer is non-null. */
    bool valid() const noexcept {
        return ptr_ != nullptr;
    }

    /** @brief Equivalent to `valid()`. */
    operator bool() const noexcept {
        return valid();
    }

    bool operator==(std::nullptr_t) {
        return !valid();
    }

    /** @brief Returns the raw pointer. */
    T *get() noexcept {
        return ptr_;
    }

    /** @brief Returns the raw pointer. */
    T const *get() const noexcept {
        return ptr_;
    }

    operator T*() noexcept {
        return get();
    }

    operator T const*() const noexcept {
        return get();
    }

    /** @brief Returns a pointer after `static_cast` to `U`. */
    template <typename U>
    memory::ptr<U> cast() noexcept {
        return { static_cast<U *>(ptr_) };
    }

    /** @brief Returns a const pointer after `static_cast` to `U`. */
    template <typename U>
    memory::ptr<const U> cast() const noexcept {
        return { static_cast<U const *>(ptr_) };
    }

    /** @brief Returns a pointer after `dynamic_cast` to `U`. */
    template <typename U>
    memory::ptr<U> dyncast() noexcept {
        return { dynamic_cast<U *>(ptr_) };
    }

    /** @brief Returns a const pointer after `dynamic_cast` to `U`. */
    template <typename U>
    memory::ptr<const U> dyncast() const noexcept {
        return { dynamic_cast<U const *>(ptr_) };
    }

    /** @brief Returns a const-qualified pointer to the same object. */
    memory::ptr<const T> as_const() const noexcept {
        return { (const T *) ptr_ };
    }

    ~ptr() {
        if (ptr_) ptr_->unref();
    }

private:
    T *ptr_ = nullptr;
};

} /* namespace memory */
} /* namespace cxxdes */

#include <functional>

namespace std {
    template <typename T>
    struct hash<cxxdes::memory::ptr<T>>: std::hash<const T *> {
        auto operator()(const cxxdes::memory::ptr<T> &x) const {
            return std::hash<const T *>::operator()(x.get());
        }
    };
} /* namespace std */

#endif /* CXXDES_MISC_REFERENCE_COUNTED_HPP_INCLUDED */
