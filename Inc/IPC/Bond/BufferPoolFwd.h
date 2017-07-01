#pragma once

#include <memory>


namespace IPC
{

class SharedMemory;

namespace Bond
{
    template <template <typename> typename QueueT>
    class BufferPool
    {
    public:
        class Blob;
        class ConstBlob;

        class Buffer;
        class ConstBuffer;

        explicit BufferPool(std::shared_ptr<SharedMemory> memory);

        Blob TakeBlob();

        Buffer TakeBuffer();

        const std::shared_ptr<SharedMemory>& GetMemory() const;

    private:
        struct Data;
        class ItemBase;

        class Impl;

        std::shared_ptr<Impl> m_impl;
    };


    namespace detail
    {
        template <typename T>
        class DefaultBufferPoolQueue;

        template <typename BufferPool>
        class ConstBuffer;

    } // detail


    using DefaultBufferPool = BufferPool<detail::DefaultBufferPoolQueue>;

    using DefaultConstBuffer = detail::ConstBuffer<DefaultBufferPool>;

} // Bond
} // IPC
