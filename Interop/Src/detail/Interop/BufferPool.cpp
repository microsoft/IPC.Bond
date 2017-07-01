#include "stdafx.h"
#include "detail/Interop/BufferPool.h"
#include <IPC/Bond/BufferPool.h>


namespace IPC
{
namespace Bond
{
namespace Managed
{
    namespace detail
    {
    namespace Interop
    {
        BufferPool::ConstBuffer::ConstBuffer(const IPC::Bond::DefaultConstBuffer& impl)
            : shared_ptr{ std::make_shared<IPC::Bond::DefaultConstBuffer>(impl) }
        {}

        BufferPool::ConstBuffer::~ConstBuffer() = default;

        std::size_t BufferPool::ConstBuffer::GetSize() const
        {
            std::size_t size = 0;

            for (const auto& blob : **this)
            {
                size += blob.size();
            }

            return size;
        }

        std::size_t BufferPool::ConstBuffer::CopyTo(void* buffer, std::size_t size) const
        {
            auto ptr = static_cast<char*>(buffer);

            for (const auto& blob : **this)
            {
                if (auto sz = (std::min)(size, blob.size()))
                {
                    std::memcpy(ptr, blob.data(), sz);
                    ptr += sz;
                    size -= sz;
                }
                else
                {
                    break;
                }
            }

            return ptr - static_cast<char*>(buffer);
        }


        BufferPool::BufferPool(const std::shared_ptr<IPC::SharedMemory>& memory)
            : shared_ptr{ std::make_shared<IPC::Bond::DefaultBufferPool>(memory) }
        {}

        BufferPool::~BufferPool() = default;

        const std::shared_ptr<IPC::SharedMemory>& BufferPool::GetMemory() const
        {
            return get()->GetMemory();
        }

    } // Interop
    } // detail

} // Managed
} // Bond
} // IPC
