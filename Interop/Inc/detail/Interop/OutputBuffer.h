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
        class OutputBuffer
        {
        public:
            OutputBuffer(const BufferPool& pool, std::size_t minBlobSize);

            ~OutputBuffer();

            void Write(const void* value, std::uint32_t size);

            void* Allocate(std::uint32_t size);

			void WriteFloat(float value);

			void WriteDouble(double value);

            void WriteByte(std::uint8_t value);

            void WriteUInt16(std::uint16_t value);

            void WriteUInt32(std::uint32_t value);

            void WriteUInt64(std::uint64_t value);

            void WriteVarUInt16(std::uint16_t value);

            void WriteVarUInt32(std::uint32_t value);

            void WriteVarUInt64(std::uint64_t value);

            BufferPool::ConstBuffer GetBuffer() &&;

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
