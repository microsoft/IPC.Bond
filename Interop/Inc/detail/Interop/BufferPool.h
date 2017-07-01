#pragma once
#pragma managed(push, off)

#include <IPC/Managed/detail/Interop/SharedMemory.h>
#include <IPC/Bond/BufferPoolFwd.h>
#include <memory>


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
        class BufferPool : public std::shared_ptr<IPC::Bond::DefaultBufferPool>
        {
        public:
            class ConstBuffer : public std::shared_ptr<IPC::Bond::DefaultConstBuffer>
            {
            public:
                ConstBuffer(const IPC::Bond::DefaultConstBuffer& impl);

                ~ConstBuffer();

                std::size_t GetSize() const;

                std::size_t CopyTo(void* buffer, std::size_t size) const;
            };

            explicit BufferPool(const std::shared_ptr<IPC::SharedMemory>& memory);

            ~BufferPool();

            const std::shared_ptr<IPC::SharedMemory>& GetMemory() const;
        };

    } // Interop
    } // detail

} // Managed
} // Bond
} // IPC

#pragma managed(pop)
