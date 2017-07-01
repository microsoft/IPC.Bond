#pragma once
#pragma managed(push, off)

#include "BufferPool.h"
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
        class InputBuffer
        {
        public:
            InputBuffer();

            InputBuffer(const BufferPool::ConstBuffer& buffer, const std::shared_ptr<IPC::SharedMemory>& memory);

            InputBuffer(const InputBuffer& other);

            ~InputBuffer();

            void Read(void* buffer, std::uint32_t size);

            const void* Allocate(std::uint32_t size);

            float ReadFloat();

            double ReadDouble();

            std::uint8_t ReadByte();

            std::uint16_t ReadUInt16();

            std::uint32_t ReadUInt32();

            std::uint64_t ReadUInt64();

            std::uint16_t ReadVarUInt16();

            std::uint32_t ReadVarUInt32();

            std::uint64_t ReadVarUInt64();

            void Skip(std::uint32_t size);

        private:
            class Impl;

            std::unique_ptr<Impl> m_impl;
        };

    } // Interop
    } // detail

} // Managed
} // Bond
} // IPC

#pragma managed(pop)
