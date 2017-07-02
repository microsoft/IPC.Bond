#pragma once

#include "BufferPool.h"
#include "detail/Interop/OutputBuffer.h"
#include "IPC/Managed/detail/NativeObject.h"


namespace IPC
{
namespace Bond
{
namespace Managed
{
    public ref class OutputStream : ::Bond::IO::IOutputStream
    {
    public:
        OutputStream(BufferPool^ pool, [System::Runtime::InteropServices::Optional] System::UInt32 minBlobSize);

        virtual property System::Int64 Position
        {
            System::Int64 get();

            void set(System::Int64 value);
        }

        virtual void WriteFloat(System::Single value);

        virtual void WriteDouble(System::Double value);

        virtual void WriteUInt8(System::Byte value);

        virtual void WriteUInt16(System::UInt16 value);

        virtual void WriteUInt32(System::UInt32 value);

        virtual void WriteUInt64(System::UInt64 value);

        virtual void WriteVarUInt16(System::UInt16 value);

        virtual void WriteVarUInt32(System::UInt32 value);

        virtual void WriteVarUInt64(System::UInt64 value);

        virtual void WriteString(System::Text::Encoding^ encoding, System::String^ value, System::Int32 count);

        virtual void WriteBytes(System::ArraySegment<System::Byte> data);

        BufferPool::ConstBuffer^ GetBuffer();

    private:
        IPC::Managed::detail::NativeObject<detail::Interop::OutputBuffer> m_impl;
    };

} // Managed
} // Bond
} // IPC
