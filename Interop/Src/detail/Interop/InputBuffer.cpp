#include "stdafx.h"
#include "detail/Interop/InputBuffer.h"
#include <IPC/Bond/InputBuffer.h>
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
        class InputBuffer::Impl : public IPC::Bond::DefaultInputBuffer
        {
        public:
            using DefaultInputBuffer::DefaultInputBuffer;
        };

        InputBuffer::InputBuffer()
            : m_impl{ std::make_unique<Impl>() }
        {}

        InputBuffer::InputBuffer(const BufferPool::ConstBuffer& buffer, const std::shared_ptr<IPC::SharedMemory>& memory)
            : m_impl{ std::make_unique<Impl>(*buffer, memory) }
        {}

        InputBuffer::InputBuffer(const InputBuffer& other)
            : m_impl{ std::make_unique<Impl>(*other.m_impl) }
        {}

        InputBuffer::~InputBuffer() = default;

        void InputBuffer::Read(void* buffer, std::uint32_t size)
        {
            InvokeThrow([&] { m_impl->Read(buffer, size); });
        }

        const void* InputBuffer::Allocate(std::uint32_t size)
        {
            return InvokeThrow([&] { return m_impl->Allocate(size); });
        }

		float InputBuffer::ReadFloat()
		{
			float value;
			InvokeThrow([&] { m_impl->Read(value); });
			return value;
		}

		double InputBuffer::ReadDouble()
		{
			double value;
			InvokeThrow([&] { m_impl->Read(value); });
			return value;
		}

        std::uint8_t InputBuffer::ReadByte()
        {
            std::uint8_t value;
            InvokeThrow([&] { m_impl->Read(value); });
            return value;
        }

        std::uint16_t InputBuffer::ReadUInt16()
        {
            std::uint16_t value;
            InvokeThrow([&] { m_impl->Read(value); });
            return value;
        }

        std::uint32_t InputBuffer::ReadUInt32()
        {
            std::uint32_t value;
            InvokeThrow([&] { m_impl->Read(value); });
            return value;
        }

        std::uint64_t InputBuffer::ReadUInt64()
        {
            std::uint64_t value;
            InvokeThrow([&] { m_impl->Read(value); });
            return value;
        }

        std::uint16_t InputBuffer::ReadVarUInt16()
        {
            std::uint16_t value;
            InvokeThrow([&] { m_impl->ReadVariableUnsigned(value); });
            return value;
        }

        std::uint32_t InputBuffer::ReadVarUInt32()
        {
            std::uint32_t value;
            InvokeThrow([&] { m_impl->ReadVariableUnsigned(value); });
            return value;
        }

        std::uint64_t InputBuffer::ReadVarUInt64()
        {
            std::uint64_t value;
            InvokeThrow([&] { m_impl->ReadVariableUnsigned(value); });
            return value;
        }

        void InputBuffer::Skip(std::uint32_t size)
        {
            InvokeThrow([&] { m_impl->Skip(size); });
        }

    } // Interop
    } // detail

} // Managed
} // Bond
} // IPC
