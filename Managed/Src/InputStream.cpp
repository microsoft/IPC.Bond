#include "InputStream.h"


namespace IPC
{
namespace Bond
{
namespace Managed
{
    namespace detail
    {
        using IPC::Managed::detail::ThrowManagedException;

    } // detail

    namespace
    {
        [[noreturn]] void ThrowNotImplemented()
        {
            throw gcnew System::NotImplementedException{};
        }

    } // anonymous


    InputStream::InputStream(BufferPool::ConstBuffer^ buffer, IPC::Managed::SharedMemory^ inputMemory)
    try
        : m_impl{ buffer->Impl, inputMemory->Impl }
    {}
    catch (const std::exception& /*e*/)
    {
        detail::ThrowManagedException(std::current_exception());
    }

    InputStream::InputStream(InputStream% other)
    try
        : m_impl{ *other.m_impl }
    {}
    catch (const std::exception& /*e*/)
    {
        detail::ThrowManagedException(std::current_exception());
    }

    System::Int64 InputStream::Length::get()
    {
        ThrowNotImplemented();
    }

    System::Int64 InputStream::Position::get()
    {
        // TODO: Needs to be implemented. (?)
        ThrowNotImplemented();
    }

    void InputStream::Position::set(System::Int64 value)
    {
        // TODO: Needs to be implemented. (?)
        ThrowNotImplemented();
    }

    System::Single InputStream::ReadFloat()
    {
        return m_impl->ReadFloat();
    }

    System::Double InputStream::ReadDouble()
    {
        return m_impl->ReadDouble();
    }

    System::Byte InputStream::ReadUInt8()
    {
        return m_impl->ReadByte();
    }

    System::UInt16 InputStream::ReadUInt16()
    {
        return m_impl->ReadUInt16();
    }

    System::UInt32 InputStream::ReadUInt32()
    {
        return m_impl->ReadUInt32();
    }

    System::UInt64 InputStream::ReadUInt64()
    {
        return m_impl->ReadUInt64();
    }

    System::UInt16 InputStream::ReadVarUInt16()
    {
        return m_impl->ReadVarUInt16();
    }

    System::UInt32 InputStream::ReadVarUInt32()
    {
        return m_impl->ReadVarUInt32();
    }

    System::UInt64 InputStream::ReadVarUInt64()
    {
        return m_impl->ReadVarUInt64();
    }

    System::String^ InputStream::ReadString(System::Text::Encoding^ encoding, System::Int32 count)
    {
        auto ptr = m_impl->Allocate(count);
        return encoding->GetString(static_cast<unsigned char*>(const_cast<void*>(ptr)), count);
    }

    System::ArraySegment<System::Byte> InputStream::ReadBytes(System::Int32 count)
    {
        auto buffer = gcnew cli::array<System::Byte>(count);
        pin_ptr<System::Byte> ptr = &buffer[0];
        m_impl->Read(ptr, count);
        return System::ArraySegment<System::Byte>(buffer);
    }

    void InputStream::SkipBytes(System::Int32 count)
    {
        m_impl->Skip(count);
    }

    InputStream^ InputStream::Clone()
    {
        return gcnew InputStream{ *this };
    }

} // Managed
} // Bond
} // IPC
