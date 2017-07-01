#include "OutputStream.h"
#include <vcclr.h>


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


    OutputStream::OutputStream(BufferPool^ pool, System::UInt32 minBlobSize)
    try
        : m_impl{ pool->Impl, static_cast<std::size_t>(minBlobSize) }
    {}
    catch (const std::exception& /*e*/)
    {
        detail::ThrowManagedException(std::current_exception());
    }

    System::Int64 OutputStream::Position::get()
    {
#ifdef NDEBUG
		ThrowNotImplemented();
#else
		return 0;
#endif // NDEBUG
    }

    void OutputStream::Position::set(System::Int64 /*value*/)
    {
        ThrowNotImplemented();
    }

	void OutputStream::WriteFloat(System::Single value)
	{
		m_impl->WriteFloat(value);
	}

	void OutputStream::WriteDouble(System::Double value)
	{
		m_impl->WriteDouble(value);
	}

    void OutputStream::WriteUInt8(System::Byte value)
    {
        m_impl->WriteByte(value);
    }

    void OutputStream::WriteUInt16(System::UInt16 value)
    {
        m_impl->WriteUInt16(value);
    }

    void OutputStream::WriteUInt32(System::UInt32 value)
    {
        m_impl->WriteUInt32(value);
    }

    void OutputStream::WriteUInt64(System::UInt64 value)
    {
        m_impl->WriteUInt64(value);
    }

    void OutputStream::WriteVarUInt16(System::UInt16 value)
    {
        m_impl->WriteVarUInt16(value);
    }

    void OutputStream::WriteVarUInt32(System::UInt32 value)
    {
        m_impl->WriteVarUInt32(value);
    }

    void OutputStream::WriteVarUInt64(System::UInt64 value)
    {
        m_impl->WriteVarUInt64(value);
    }

	void OutputStream::WriteString(System::Text::Encoding^ encoding, System::String^ value, System::Int32 count)
	{
		auto ptr = m_impl->Allocate(count);
		pin_ptr<const wchar_t> str = PtrToStringChars(value);
		encoding->GetBytes(const_cast<wchar_t*>(str), value->Length, static_cast<unsigned char*>(ptr), count);
	}

	void OutputStream::WriteBytes(System::ArraySegment<System::Byte> data)
	{
		if (data.Array != nullptr && data.Count > 0)
		{
			pin_ptr<System::Byte> ptr = &data.Array[data.Offset];
			m_impl->Write(ptr, data.Count);
		}
	}

	BufferPool::ConstBuffer^ OutputStream::GetBuffer()
	{
		auto buffer = gcnew BufferPool::ConstBuffer{ std::move(*m_impl).GetBuffer() };
		delete this;
		return buffer;
	}

} // Managed
} // Bond
} // IPC
