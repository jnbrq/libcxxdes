#ifndef CXXDES_MISC_MEMORY_HPP_INCLUDED
#define CXXDES_MISC_MEMORY_HPP_INCLUDED

#include <new>

#if __has_include(<memory_resource>)
#   include <memory_resource>
#elif __has_include(<experimental/memory_resource>)

#   include <experimental/memory_resource>

namespace std {
    namespace pmr {
        using namespace std::experimental::pmr;
    }
}

#else
#error "Cannot find the memory_resource header!"
#endif

namespace cxxdes {
namespace memory {

using memory_resource = std::pmr::memory_resource;

// Round sz up to next multiple of al.
inline constexpr size_t aligned_allocation_size(size_t sz, size_t al) noexcept
{
    // taken from libc++
    return (sz + al - 1) & ~(al - 1);
}

struct custom_allocatable {
private:
    struct metadata {
        memory_resource *memres = nullptr;
        std::size_t sz = 0;
        std::size_t al = 0;
    };

public:
    static void *operator new(
        std::size_t sz,
        std::align_val_t al,
        memory_resource *memres = std::pmr::get_default_resource()) {
        std::size_t const meta_sz = aligned_allocation_size(sizeof(metadata), (std::size_t) al);
        void *p = memres->allocate(meta_sz + sz, (std::size_t) al);
        *((metadata *) p) = metadata{ memres, sz, (std::size_t) al };
        return ((std::byte *) p + meta_sz);
    }

    static void *operator new(
        std::size_t sz,
        memory_resource *memres = std::pmr::get_default_resource()) {
        return custom_allocatable::operator new(sz, std::align_val_t{alignof(max_align_t)}, memres);
    }

    static void operator delete(void *p, std::align_val_t al) {
        std::size_t const meta_sz = aligned_allocation_size(sizeof(metadata), (std::size_t) al);
        p = (std::byte *)p - meta_sz;
        metadata meta = *(metadata *)(p);
        meta.memres->deallocate(p, meta.sz, meta.al);
    }

    static void operator delete(void *p) {
        custom_allocatable::operator delete(p, std::align_val_t{alignof(max_align_t)});
    }
};

} /* namespace memory */
} /* namespace cxxdes */

#endif /* CXXDES_MISC_MEMORY_HPP_INCLUDED */
