#include "stdafx.h"
#include "detail/Interop/OutputBuffer.h"
#include <IPC/Bond/OutputBuffer.h>
#include <IPC/Managed/detail/Throw.h>


namespace IPC
{
namespace Bond
{
namespace Managed
{
    namespace detail
    {
        using IPC::Managed::detail::InvokeThrow;

    namespace Interop
    {
        class OutputBuffer::Impl : public IPC::Bond::DefaultOutputBuffer
        {
        public:
            using DefaultOutputBuffer::DefaultOutputBuffer;
        };

        OutputBuffer::OutputBuffer(const BufferPool& pool, std::size_t minBlobSize)
            : m_impl{ std::make_unique<Impl>(pool, minBlobSize) }
        {}

        OutputBuffer::~OutputBuffer() = default;

        void OutputBuffer::Write(const void* value, std::uint32_t size)
        {
            InvokeThrow([&] { m_impl->Write(value, size); });
        }

        void* OutputBuffer::Allocate(std::uint32_t size)
        {
            return InvokeThrow([&] { return m_impl->Allocate(size); });
        }

        void OutputBuffer::WriteFloat(float value)
        {
            InvokeThrow([&] { m_impl->Write(value); });
        }

        void OutputBuffer::WriteDouble(double value)
        {
            InvokeThrow([&] { m_impl->Write(value); });
        }

        void OutputBuffer::WriteByte(std::uint8_t value)
        {
            InvokeThrow([&] { m_impl->Write(value); });
        }

        void OutputBuffer::WriteUInt16(std::uint16_t value)
        {
            InvokeThrow([&] { m_impl->Write(value); });
        }

        void OutputBuffer::WriteUInt32(std::uint32_t value)
        {
            InvokeThrow([&] { m_impl->Write(value); });
        }

        void OutputBuffer::WriteUInt64(std::uint64_t value)
        {
            InvokeThrow([&] { m_impl->Write(value); });
        }

        void OutputBuffer::WriteVarUInt16(std::uint16_t value)
        {
            InvokeThrow([&] { m_impl->WriteVariableUnsigned(value); });
        }

        void OutputBuffer::WriteVarUInt32(std::uint32_t value)
        {
            InvokeThrow([&] { m_impl->WriteVariableUnsigned(value); });
        }

        void OutputBuffer::WriteVarUInt64(std::uint64_t value)
        {
            InvokeThrow([&] { m_impl->WriteVariableUnsigned(value); });
        }

        BufferPool::ConstBuffer OutputBuffer::GetBuffer() &&
        {
            return { InvokeThrow([&] { return std::move(*m_impl).GetBuffer(); }) };
        }

    } // Interop
    } // detail

} // Managed
} // Bond
} // IPC
