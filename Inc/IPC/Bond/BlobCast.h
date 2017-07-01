#pragma once

#include "detail/BlobHolder.h"
#include <type_traits>

#ifndef NDEBUG
#include <IPC/SharedMemory.h>
#endif


namespace IPC
{
namespace Bond
{
    template <typename Blob>
    bond::blob BlobCast(Blob&& from, std::shared_ptr<SharedMemory> memory)
    {
        auto data = from.data();
        auto size = from.size();

        assert(memory && memory->Contains(data) && memory->Contains(data + size - 1));

        return{
            boost::shared_ptr<const char[]>{ data, detail::BlobHolder<std::decay_t<Blob>>{ std::forward<Blob>(from), std::move(memory) } },
            static_cast<std::uint32_t>(size) };
    }

    template <typename Blob>
    Blob BlobCast(const bond::blob& from)
    {
        if (auto deleter = boost::get_deleter<detail::BlobHolder<Blob>>(bond::blob_cast<detail::BlobHook>(from)))
        {
            const auto& blob = deleter->m_blob;
            assert(blob);

            return blob.GetRange(std::distance(blob.data(), from.content()), from.size());
        }

        return{};
    }

} // Bond
} // IPC
