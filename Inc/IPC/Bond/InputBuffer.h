#pragma once

#include "BlobCast.h"
#include "BufferPool.h"

#include <bond/core/traits.h>
#include <bond/stream/input_buffer.h>
#include <bond/protocol/encoding.h>


namespace IPC
{
namespace Bond
{
    template <typename ConstBuffer>
    class InputBuffer
    {
    public:
        InputBuffer() = default;

        InputBuffer(ConstBuffer buffer, std::shared_ptr<SharedMemory> memory)
            : InputBuffer{ buffer ? InputBuffer{ std::move(memory), buffer, buffer.begin(), buffer.end() } : InputBuffer{} }
        {}

        InputBuffer(const typename ConstBuffer::Range& range, std::shared_ptr<SharedMemory> memory)
            : InputBuffer{ std::move(memory), range.m_buffer, range.m_firstBlob, range.m_lastBlob, range.m_firstOffset, range.m_lastOffset }
        {}

        template <typename T>
        void Read(T& value)
        {
            // Assuming primitives do not span over multiple blobs.
            ReadSingle(sizeof(T), [&] { std::memcpy(&value, m_ptr, sizeof(T)); });
        }

        void Read(void* buffer, std::uint32_t size)
        {
            ReadMultiple(
                size,
                [&](std::size_t available)
                {
                    std::memcpy(buffer, m_ptr, available);
                    buffer = static_cast<char*>(buffer) + available;
                });
        }

        void Read(bond::blob& bondBlob, std::uint32_t size)
        {
            // Do not merge if spans over multiple blobs.
            const auto& blob = *m_blob;
            ReadSingle(size, [&] { bondBlob.assign(BlobCast(blob, m_memory), static_cast<std::uint32_t>(m_ptr - blob.data()), size); });
        }

        const void* Allocate(std::uint32_t size)
        {
            const void* ptr;
            ReadSingle(size, [&] { ptr = m_ptr; });
            return ptr;
        }

        template <typename T>
        void ReadVariableUnsigned(T& value)
        {
            if (m_ptr + sizeof(T) * 8 / 7 < m_ptrEnd)
            {
                auto ptr = m_ptr;
                bond::input_buffer::VariableUnsignedUnchecked<T, 0>::Read(ptr, value);
                EndRead(ptr);
            }
            else
            {
                bond::GenericReadVariableUnsigned(*this, value);
            }
        }

        void Skip(std::uint32_t size)
        {
            ReadMultiple(size, [](std::size_t) {});
        }

        bool IsEof() const
        {
            assert(m_isEof == (m_blob == m_blobEnd));
            return m_isEof;
        }

        bool operator==(const InputBuffer& other) const
        {
            return m_ptr == other.m_ptr && m_blob == other.m_blob && m_blobEnd == other.m_blobEnd && m_lastOffset == other.m_lastOffset;
        }

        const std::shared_ptr<SharedMemory>& GetMemory() const
        {
            return m_memory;
        }

    private:
        using BlobIterator = decltype(std::declval<ConstBuffer>().begin());

        InputBuffer(
            std::shared_ptr<SharedMemory> memory,
            ConstBuffer buffer,
            BlobIterator begin,
            BlobIterator end,
            std::size_t firstOffset = 0,
            std::size_t lastOffset = 0)
            : m_blob{ begin },
              m_blobEnd{ lastOffset != 0 ? std::next(end) : end },
              m_lastOffset{ lastOffset },
              m_memory{ std::move(memory) },
              m_buffer{ std::move(buffer) }
        {
            if (UpdateData())
            {
                m_ptr += firstOffset;
            }
        }

        template <typename Function>
        void ReadMultiple(std::size_t size, Function&& func)
        {
            auto ptr = m_ptr;
            auto ptrEnd = m_ptrEnd;
            auto blob = m_blob;

            try
            {
                while (size != 0)
                {
                    const auto n = std::min<std::size_t>(m_ptrEnd - m_ptr, size);

                    ReadSingle(n, [&] { func(n); });

                    size -= n;
                }
            }
            catch (...)
            {
                m_ptr = ptr;
                m_ptrEnd = ptrEnd;
                m_blob = blob;
                m_isEof = (m_blob == m_blobEnd);

                throw;
            }
        }

        void BeginRead(const char* ptr) const
        {
            if (ptr > m_ptrEnd || IsEof())
            {
                throw std::out_of_range{ "Out of buffer range." };
            }
        }

        void EndRead(const char* ptr)
        {
            if (ptr != m_ptrEnd)
            {
                m_ptr = ptr;
            }
            else
            {
                MoveNext();
            }
        }

        __declspec(noinline) void MoveNext()
        {
            m_isEof = (++m_blob == m_blobEnd);
            UpdateData();
        }

        bool SkipEmptyBlobs()
        {
            for (; m_blob != m_blobEnd; ++m_blob)
            {
                const auto& blob = *m_blob;

                if (blob && blob.size() != 0)
                {
                    return false;
                }
            }

            m_isEof = true;
            return true;
        }

        template <typename Function>
        void ReadSingle(std::size_t size, Function&& func)
        {
            const auto ptr = m_ptr + size;

            BeginRead(ptr);

            std::forward<Function>(func)();

            EndRead(ptr);
        }

        bool UpdateData()
        {
            if (!SkipEmptyBlobs())
            {
                const auto& blob = *m_blob;

                m_ptr = blob.data();
                m_ptrEnd = blob.data() + (m_lastOffset != 0 && std::next(m_blob) == m_blobEnd ? m_lastOffset : blob.size());

                return true;
            }

            m_ptr = m_ptrEnd = nullptr;

            return false;
        }

        friend auto GetCurrentBuffer(const InputBuffer& buffer)
        {
            return buffer;
        }

        friend auto GetBufferRange(const InputBuffer& begin, const InputBuffer& end)
        {
            assert(begin.m_buffer == end.m_buffer);
            assert(end.IsEof() || !begin.IsEof());

            return typename ConstBuffer::Range{
                begin.m_buffer,
                begin.m_blob,
                begin.IsEof() ? 0 : std::size_t(begin.m_ptr - begin.m_blob->data()),
                end.m_blob,
                end.IsEof() ? 0 : std::size_t(end.m_ptr - end.m_blob->data()) };
        }

        friend auto CreateInputBuffer(const InputBuffer& other, const typename ConstBuffer::Range& range)
        {
            return InputBuffer{ range, other.m_memory };
        }


        const char* m_ptr{ nullptr };
        const char* m_ptrEnd{ nullptr };
        BlobIterator m_blob{};
        BlobIterator m_blobEnd{};
        bool m_isEof{ m_blob == m_blobEnd };
        std::size_t m_lastOffset{ 0 };
        std::shared_ptr<SharedMemory> m_memory; // Must be declared before m_buffer.
        ConstBuffer m_buffer{};
    };


    using DefaultInputBuffer = InputBuffer<DefaultBufferPool::ConstBuffer>;

} // Bond
} // IPC

namespace bond
{
    BOND_DEFINE_BUFFER_MAGIC(IPC::Bond::DefaultInputBuffer, 0x5349 /*SB*/);

} // namespace bond
