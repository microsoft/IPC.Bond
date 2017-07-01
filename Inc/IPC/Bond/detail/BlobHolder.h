#pragma once

#include <bond/core/blob.h>
#include <cassert>


namespace IPC
{
    class SharedMemory;


namespace Bond
{
    namespace detail
    {
        template <typename Blob>
        struct BlobHolder
        {
            explicit BlobHolder(Blob blob, std::shared_ptr<SharedMemory> memory = {})
                : m_memory{ std::move(memory) },
                  m_blob{ std::move(blob) }
            {}

            void operator()(const char* buffer)
            {
                if (buffer != nullptr)
                {
                    assert(m_blob);
                    assert(buffer == m_blob.data());

                    m_blob = {};
                }
            }

            std::shared_ptr<SharedMemory> m_memory; // Must be declared before m_blob.
            Blob m_blob;
        };

        struct BlobHook : boost::shared_ptr<const char[]>
        {
            using shared_ptr::shared_ptr;
        };

    } // detail
} // Bond
} // IPC


namespace bond
{
    template <>
    inline IPC::Bond::detail::BlobHook blob_cast(const blob& from)
    {
        return from._buffer;
    }

} // bond
