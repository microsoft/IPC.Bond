#include "BufferPool.h"


namespace IPC
{
namespace Bond
{
namespace Managed
{
    BufferPool::ConstBuffer::ConstBuffer(const detail::Interop::BufferPool::ConstBuffer& buffer)
        : m_impl{ buffer },
          m_length{
            gcnew System::Func<System::Int32>(this, &BufferPool::ConstBuffer::CalculateLength),
            System::Threading::LazyThreadSafetyMode::PublicationOnly }
    {}

    const detail::Interop::BufferPool::ConstBuffer& BufferPool::ConstBuffer::Impl::get()
    {
        return *m_impl;
    }

    System::Int32 BufferPool::ConstBuffer::CalculateLength()
    {
        return static_cast<int>(m_impl->GetSize());
    }

    System::Int32 BufferPool::ConstBuffer::Length::get()
    {
        return m_length.Value;
    }

    void BufferPool::ConstBuffer::CopyTo(cli::array<System::Byte>^ buffer, System::Int32 offset)
    {
        if (buffer == nullptr)
        {
            throw gcnew System::ArgumentNullException{ "buffer" };
        }

        auto length = Length;

        if (length > 0)
        {
            if (offset + length > buffer->Length)
            {
                throw gcnew System::ArgumentException{ "Insufficient buffer size.", "buffer" };
            }

            pin_ptr<System::Byte> ptr = &buffer[offset];
            m_impl->CopyTo(ptr, length);
        }
    }

    BufferPool::BufferPool(IPC::Managed::SharedMemory^ memory)
        : m_impl{ memory->Impl }
    {}

    IPC::Managed::SharedMemory^ BufferPool::Memory::get()
    {
        return gcnew IPC::Managed::SharedMemory{ m_impl->GetMemory() };
    }

    detail::Interop::BufferPool& BufferPool::Impl::get()
    {
        return *m_impl;
    }

} // Managed
} // Bond
} // IPC
