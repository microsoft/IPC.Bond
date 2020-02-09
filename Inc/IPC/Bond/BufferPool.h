#pragma once

#include "BufferPoolFwd.h"
#include "IPC/SharedMemory.h"
#include "IPC/detail/LockFree/Queue.h"
#include <boost/interprocess/containers/vector.hpp>


namespace IPC
{
namespace Bond
{
    template <template <typename> typename QueueT>
    class BufferPool<QueueT>::Impl
    {
    public:
        using Queue = QueueT<SharedMemory::SharedPtr<Data>>;

        explicit Impl(std::shared_ptr<SharedMemory> memory)
            : m_memory{ std::move(memory) },
              m_queue{ m_memory->MakeShared<Queue>(anonymous_instance, m_memory->GetAllocator<char>()) }
        {}

        SharedMemory::SharedPtr<Data> Take()
        {
            if (auto data = m_queue->Pop())
            {
                return std::move(*data);
            }

            return m_memory->MakeShared<Data>(anonymous_instance, m_memory->GetAllocator<char>());
        }

        const auto& GetQueue() const
        {
            return m_queue;
        }

        const auto& GetMemory() const
        {
            return m_memory;
        }

    private:
        std::shared_ptr<SharedMemory> m_memory;
        SharedMemory::SharedPtr<Queue> m_queue;
    };


    template <template <typename> typename QueueT>
    struct BufferPool<QueueT>::Data
    {
        explicit Data(const SharedMemory::Allocator<char>& allocator)
            : m_blob{ allocator },
              m_buffer{ allocator }
        {}

        boost::interprocess::vector<char, SharedMemory::Allocator<char>> m_blob;
        boost::interprocess::vector<ConstBlob, SharedMemory::Allocator<ConstBlob>> m_buffer;
    };


    template <template <typename> typename QueueT>
    class BufferPool<QueueT>::ItemBase
    {
    public:
        ~ItemBase()
        {
            if (m_data.unique())
            {
                m_data->m_blob.clear();
                m_data->m_buffer.clear();
                m_queue->Push(std::move(m_data));
            }
        }

        explicit operator bool() const
        {
            return m_data && m_queue;
        }

    protected:
        ItemBase() = default;

        ItemBase(SharedMemory::SharedPtr<Data> data, SharedMemory::SharedPtr<typename Impl::Queue> queue)
            : m_data{ std::move(data) },
              m_queue{ std::move(queue) }
        {}

        const auto& GetBlob() const
        {
            return m_data->m_blob;
        }

        auto& GetBlob()
        {
            return m_data->m_blob;
        }

        const auto& GetBuffer() const
        {
            return m_data->m_buffer;
        }

        auto& GetBuffer()
        {
            return m_data->m_buffer;
        }

        bool operator==(const ItemBase& other) const
        {
            return m_data == other.m_data && m_queue == other.m_queue;
        }

    private:
        SharedMemory::SharedPtr<Data> m_data;
        SharedMemory::SharedPtr<typename Impl::Queue> m_queue;
    };


    template <template <typename> typename QueueT>
    class BufferPool<QueueT>::Blob : public ItemBase
    {
        friend BufferPool;

        using ItemBase::ItemBase;

    public:
        Blob() = default;

        Blob(const Blob& other) = delete;
        Blob& operator=(const Blob& other) = delete;

        Blob(Blob&& other) = default;
        Blob& operator=(Blob&& other) = default;

        decltype(auto) operator*() const
        {
            return this->GetBlob();
        }

        decltype(auto) operator*()
        {
            return this->GetBlob();
        }

        auto operator->() const
        {
            return &this->GetBlob();
        }

        auto operator->()
        {
            return &this->GetBlob();
        }
    };


    template <template <typename> typename QueueT>
    class BufferPool<QueueT>::ConstBlob : public ItemBase
    {
        using Iterator = decltype(std::declval<Data>().m_blob.cbegin());

    public:
        ConstBlob() = default;

        ConstBlob(const Blob& blob)
            : ConstBlob{ blob, blob->begin(), blob->end() }
        {}

        auto begin() const
        {
            return m_begin;
        }

        auto end() const
        {
            return m_end;
        }

        std::size_t size() const
        {
            return std::distance(m_begin, m_end);
        }

        const char* data() const
        {
            return std::addressof(*m_begin);
        }

        ConstBlob GetRange(std::size_t offset, std::size_t count) const
        {
            if (offset + count > size())
            {
                throw std::out_of_range{ "Out of buffer range." };
            }

            auto begin = std::next(m_begin, offset);

            return{ *this, begin, std::next(begin, count) };
        }

    private:
        ConstBlob(ItemBase blob, Iterator begin, Iterator end)
            : ItemBase{ std::move(blob) },
              m_begin{ begin },
              m_end{ end }
        {}

        Iterator m_begin{};
        Iterator m_end{};
    };


    template <template <typename> typename QueueT>
    class BufferPool<QueueT>::Buffer : public ItemBase
    {
        friend BufferPool;

        using ItemBase::ItemBase;

    public:
        Buffer() = default;

        Buffer(const Buffer& other) = delete;
        Buffer& operator=(const Buffer& other) = delete;

        Buffer(Buffer&& other) = default;
        Buffer& operator=(Buffer&& other) = default;

        decltype(auto) operator*() const
        {
            return this->GetBuffer();
        }

        decltype(auto) operator*()
        {
            return this->GetBuffer();
        }

        auto operator->() const
        {
            return &this->GetBuffer();
        }

        auto operator->()
        {
            return &this->GetBuffer();
        }
    };


    template <template <typename> typename QueueT>
    class BufferPool<QueueT>::ConstBuffer : public ItemBase
    {
        using BlobIterator = decltype(std::declval<Data>().m_buffer.cbegin());

    public:
        struct Range
        {
            Range() = default;

            Range(ConstBuffer buffer, BlobIterator firstBlob, std::size_t firstOffset, BlobIterator lastBlob, std::size_t lastOffset)
                : m_firstBlob{ std::move(firstBlob) },
                  m_firstOffset{ firstOffset },
                  m_lastBlob{ std::move(lastBlob) },
                  m_lastOffset{ lastOffset },
                  m_buffer{ std::move(buffer) }
            {}

            bool IsEmpty() const
            {
                return m_firstBlob == m_lastBlob && m_firstOffset == m_lastOffset;
            }

            BlobIterator m_firstBlob;
            std::size_t m_firstOffset{ 0 };
            BlobIterator m_lastBlob;
            std::size_t m_lastOffset{ 0 };
            ConstBuffer m_buffer;
        };


        ConstBuffer() = default;

        ConstBuffer(Buffer buffer)
            : ItemBase{ std::move(buffer) }
        {}

        auto begin() const
        {
            return this->GetBuffer().begin();
        }

        auto end() const
        {
            return this->GetBuffer().end();
        }

        std::size_t size() const
        {
            std::size_t size = 0;

            for (const auto& blob : this->GetBuffer())
            {
                size += blob.size();
            }

            return size;
        }

        bool operator==(const ConstBuffer& other) const
        {
            return ItemBase::operator==(other);
        }
    };


    template <template <typename> typename QueueT>
    BufferPool<QueueT>::BufferPool(std::shared_ptr<SharedMemory> memory)
        : m_impl{ std::make_shared<Impl>(std::move(memory)) }
    {}

    template <template <typename> typename QueueT>
    auto BufferPool<QueueT>::TakeBlob() -> Blob
    {
        return{ m_impl->Take(), m_impl->GetQueue() };
    }

    template <template <typename> typename QueueT>
    auto BufferPool<QueueT>::TakeBuffer() -> Buffer
    {
        return{ m_impl->Take(), m_impl->GetQueue() };
    }

    template <template <typename> typename QueueT>
    const std::shared_ptr<SharedMemory>& BufferPool<QueueT>::GetMemory() const
    {
        return m_impl->GetMemory();
    }


    namespace detail
    {
        template <typename T>
        class DefaultBufferPoolQueue : public IPC::detail::LockFree::Queue<T, SharedMemory::Allocator<char>>
        {
        public:
            using IPC::detail::LockFree::Queue<T, SharedMemory::Allocator<char>>::Queue;
        };

        template <typename BufferPool>
        class ConstBuffer : public BufferPool::ConstBuffer
        {
        public:
            using BufferPool::ConstBuffer::ConstBuffer;

            ConstBuffer() = default;

            ConstBuffer(const typename BufferPool::ConstBuffer& buffer)
                : BufferPool::ConstBuffer{ buffer }
            {}
        };

    } // detail

} // Bond
} // IPC
