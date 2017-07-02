#pragma once

#include "BufferPool.h"
#include "detail/Interop/InputBuffer.h"
#include "IPC/Managed/detail/NativeObject.h"


namespace IPC
{
namespace Bond
{
namespace Managed
{
    public ref class InputStream : public ::Bond::IO::IInputStream, public ::Bond::IO::ICloneable<InputStream^>
    {
    public:
        InputStream(BufferPool::ConstBuffer^ buffer, IPC::Managed::SharedMemory^ inputMemory);

        virtual property System::Int64 Length
        {
            System::Int64 get();
        }

        virtual property System::Int64 Position
        {
            System::Int64 get();

            void set(System::Int64 value);
        }

        virtual System::Single ReadFloat();

        virtual System::Double ReadDouble();

        virtual System::Byte ReadUInt8();

        virtual System::UInt16 ReadUInt16();

        virtual System::UInt32 ReadUInt32();

        virtual System::UInt64 ReadUInt64();

        virtual System::UInt16 ReadVarUInt16();

        virtual System::UInt32 ReadVarUInt32();

        virtual System::UInt64 ReadVarUInt64();

        virtual System::String^ ReadString(System::Text::Encoding^ encoding, System::Int32 count);

        virtual System::ArraySegment<System::Byte> ReadBytes(System::Int32 count);

        virtual void SkipBytes(System::Int32 count);

        virtual InputStream^ Clone();

    private:
        InputStream(InputStream% other);

        IPC::Managed::detail::NativeObject<detail::Interop::InputBuffer> m_impl;
    };

} // Managed
} // Bond
} // IPC
