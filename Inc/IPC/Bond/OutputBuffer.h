#pragma once

#include "BlobCast.h"
#include "BufferPool.h"

#include <bond/stream/output_buffer.h>
#include <bond/protocol/encoding.h>


namespace IPC
{
namespace Bond
{
    template <typename BufferPool>
    class OutputBuffer
    {
    public:
        explicit OutputBuffer(std::shared_ptr<BufferPool> pool, std::size_t minBlobSize = 0)
            : m_buffer{ pool->TakeBuffer() },
              m_pool{ std::move(pool) },
              m_minBlobSize{ minBlobSize != 0 ? minBlobSize : 4096 }
        {
            TakeBlob();
        }

        template <typename T>
        void Write(const T& value)
        {
            if (m_ptr + sizeof(T) < m_ptrEnd)
            {
                std::memcpy(m_ptr, &value, sizeof(T));
                m_ptr += sizeof(T);
            }
            else
            {
                Write(&value, sizeof(T));
            }
        }

        void Write(const void* value, std::uint32_t size)
        {
            std::memcpy(Allocate(size), value, size);
        }

        void* Allocate(std::uint32_t size)
        {
            if (m_ptr + size >= m_ptrEnd)
            {
                auto& blob = *m_blob;
                auto offset = m_ptr - blob.data();

                blob.resize(offset + std::max<std::size_t>(size, m_minBlobSize), boost::container::default_init);

                m_ptr = blob.data() + offset;
                m_ptrEnd = blob.data() + blob.size();
            }

            auto ptr = m_ptr;
            m_ptr += size;
            return ptr;
        }

        void Write(const bond::blob& blob)
        {
            Write(BlobCast<typename BufferPool::ConstBlob>(blob), blob);
        }

        void Write(const typename BufferPool::ConstBuffer& buffer)
        {
            if (buffer)
            {
                for (const auto& blob : buffer)
                {
                    Write(blob);
                }
            }
        }

        void Write(const typename BufferPool::ConstBuffer::Range& range)
        {
            if (!range.IsEmpty())
            {
                const auto& first = range.m_firstBlob;

                auto length = std::distance(
                    first,
                    range.m_lastOffset != 0 ? std::next(range.m_lastBlob) : range.m_lastBlob);

                assert(length != 0);

                if (length == 1)
                {
                    Write(first->GetRange(
                        range.m_firstOffset,
                        (range.m_lastOffset != 0 ? range.m_lastOffset : first->size()) - range.m_firstOffset));
                }
                else
                {
                    auto last = std::next(first, length - 1);

                    Write(range.m_firstOffset != 0
                        ? first->GetRange(range.m_firstOffset, first->size() - range.m_firstOffset)
                        : *first);

                    for (auto it = std::next(first); it != last; ++it)
                    {
                        Write(*it);
                    }

                    Write(range.m_lastOffset != 0
                        ? last->GetRange(0, range.m_lastOffset)
                        : *last);
                }
            }
        }

        template<typename T>
        void WriteVariableUnsigned(T value)
        {
            if (m_ptr + sizeof(T) * 8 / 7 < m_ptrEnd)
            {
                m_ptr += bond::output_buffer::VariableUnsignedUnchecked<T, 1>::Write(m_ptr, value);
            }
            else
            {
                bond::GenericWriteVariableUnsigned(*this, value);
            }
        }

        void Flush()
        {
            if (Merge())
            {
                TakeBlob();
            }
        }

        const typename BufferPool::Buffer& GetBuffer() const &
        {
            if (m_ptr != m_blob->data())
            {
                throw std::runtime_error{ "Buffer is not flushed." };
            }

            return m_buffer;
        }

        const typename BufferPool::Buffer& GetBuffer() &
        {
            Flush();
            return m_buffer;
        }

        typename BufferPool::ConstBuffer GetBuffer() &&
        {
            Merge();
            return std::move(m_buffer);
        }

        const std::shared_ptr<BufferPool>& GetBufferPool() const
        {
            return m_pool;
        }

    private:
        void TakeBlob()
        {
            m_blob = m_pool->TakeBlob();
            m_blob->resize(std::max<std::size_t>(m_blob->capacity(), m_minBlobSize), boost::container::default_init);

            m_ptr = m_blob->data();
            m_ptrEnd = m_blob->data() + m_blob->size();
        }

        bool Merge()
        {
            auto offset = m_ptr - m_blob->data();

            if (offset != 0)
            {
                m_blob->resize(offset);
                m_buffer->push_back(std::move(m_blob));

                return true;
            }

            return false;
        }

        template <typename OtherBlob>
        void Write(const typename BufferPool::ConstBlob& blob, const OtherBlob& otherBlob)
        {
            if (blob && m_pool->GetMemory()->Contains(blob.data()))
            {
                assert(m_pool->GetMemory()->Contains(blob.data() + blob.size() - 1));

                Flush();
                m_buffer->push_back(blob);
            }
            else
            {
                Write(otherBlob.data(), static_cast<uint32_t>(otherBlob.size()));
            }
        }

        void Write(const typename BufferPool::ConstBlob& blob)
        {
            Write(blob, blob);
        }

        friend auto CreateOutputBuffer(const OutputBuffer& other)
        {
            return OutputBuffer{ other.m_pool };
        }

        char* m_ptr{ nullptr };
        char* m_ptrEnd{ nullptr };
        typename BufferPool::Blob m_blob;
        typename BufferPool::Buffer m_buffer;
        std::shared_ptr<BufferPool> m_pool;
        std::size_t m_minBlobSize;
    };


    using DefaultOutputBuffer = OutputBuffer<DefaultBufferPool>;

} // Bond
} // IPC
