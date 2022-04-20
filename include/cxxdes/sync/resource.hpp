/**
 * @file resource.hpp
 * @author Canberk Sönmez (canberk.sonmez.409@gmail.com)
 * @brief Resource synchronization primitive.
 * @date 2022-04-20
 * 
 * Copyright (c) Canberk Sönmez 2022
 * 
 */

#ifndef CXXDES_SYNC_RESOURCE_HPP_INCLUDED
#define CXXDES_SYNC_RESOURCE_HPP_INCLUDED

#include <cxxdes/core/process.hpp>
#include <cxxdes/sync/semaphore.hpp>

namespace cxxdes {
namespace sync {

namespace detail {
namespace ns_resource {

using core::process;

struct resource_handle {
    resource_handle(semaphore<> *s): s_{s} {
    }

    process<> release() {
        co_await s_->up();
    }
private:
    semaphore<> *s_;
};

struct resource {
    resource(std::size_t count): s_{count, count} {
    }

    process<resource_handle> acquire() {
        co_await s_.down();
        co_return resource_handle{&s_};
    }
private:
    semaphore<> s_;
};

} /* namespace resource */
} /* namespace detail */

using detail::ns_resource::resource_handle;
using detail::ns_resource::resource;

} /* namespace sync */
} /* namespace cxxdes */

#endif /* CXXDES_SYNC_RESOURCE_HPP_INCLUDED */
