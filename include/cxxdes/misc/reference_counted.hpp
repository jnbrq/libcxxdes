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

#include <type_traits>

namespace cxxdes {
namespace memory {

/**
 * @brief Makes a type reference counted (not thread safe).
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
    void ref() const noexcept {
        ++count_;
    }

    void unref() {
        assert(count_ > 0 && "object is already destroyed.");
        if (--count_ == 0) {
            delete static_cast<Derived *>(this);
        }
    }

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

template <typename T>
concept reference_counted = std::is_base_of_v<reference_counted_base<T>, T>;

/**
 * @brief Pointer type to be used with reference counted objects.
 * 
 * @tparam T 
 */
template <reference_counted T>
struct ptr {
    ptr() noexcept = default;

    ptr(T *p) noexcept: ptr_{p} {
        if (ptr_) ptr_->ref();
    }

    ptr(const ptr& other) noexcept {
        *this = other;
    }

    ptr &operator=(const ptr &other) noexcept {
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

    ptr &operator=(ptr &&other) {
        std::swap(ptr_, other.ptr_);
        return *this;
    }

    T *operator->() noexcept {
        return ptr_;
    }

    T const *operator->() const noexcept {
        return ptr_;
    }

    operator bool() const noexcept {
        return ptr_ != nullptr;
    }

    ~ptr() {
        if (ptr_) ptr_->unref();
    }

private:
    T *ptr_ = nullptr;
};

} /* namespace memory */
} /* namespace cxxdes */

#endif /* CXXDES_MISC_REFERENCE_COUNTED_HPP_INCLUDED */