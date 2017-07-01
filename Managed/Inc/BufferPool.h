#pragma once

#include "detail/Interop/BufferPool.h"
#include "IPC/Managed/detail/NativeObject.h"


namespace IPC
{
namespace Bond
{
namespace Managed
{
    public ref class BufferPool
    {
    public:
        ref class ConstBuffer
        {
        internal:
            ConstBuffer(const detail::Interop::BufferPool::ConstBuffer& buffer);

            property const detail::Interop::BufferPool::ConstBuffer& Impl
            {
                const detail::Interop::BufferPool::ConstBuffer& get();
            }

        public:
            property System::Int32 Length
            {
                System::Int32 get();
            }

            void CopyTo(cli::array<System::Byte>^ buffer, System::Int32 offset);

        private:
            System::Int32 CalculateLength();

            IPC::Managed::detail::NativeObject<detail::Interop::BufferPool::ConstBuffer> m_impl;
            System::Lazy<System::Int32> m_length;
        };

        BufferPool(IPC::Managed::SharedMemory^ memory);

        property IPC::Managed::SharedMemory^ Memory
        {
            IPC::Managed::SharedMemory^ get();
        }

    internal:
        property detail::Interop::BufferPool& Impl
        {
            detail::Interop::BufferPool& get();
        }

    private:
        IPC::Managed::detail::NativeObject<detail::Interop::BufferPool> m_impl;
    };

} // Managed
} // Bond
} // IPC
