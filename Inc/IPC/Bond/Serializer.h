#pragma once

#include "OutputBuffer.h"
#include "InputBuffer.h"
#include "BufferPool.h"
#include <bond/core/bond.h>
#include <memory>
#include <future>


namespace IPC
{
namespace Bond
{
    struct DefaultProtocols
        : bond::Protocols<
            bond::CompactBinaryReader<DefaultInputBuffer>,
            bond::SimpleBinaryReader<DefaultInputBuffer>,
            bond::FastBinaryReader<DefaultInputBuffer>,
            bond::SimpleJsonReader<DefaultInputBuffer>> {};


    template <template <typename> typename Writer, typename Protocols = DefaultProtocols, typename BufferPool, typename T>
    typename BufferPool::ConstBuffer Serialize(std::shared_ptr<BufferPool> pool, const T& value, std::size_t minBlobSize = 0)
    {
        OutputBuffer<BufferPool> output{ std::move(pool), minBlobSize };
        Writer<decltype(output)> writer{ output };
        bond::Serialize<Protocols>(value, writer);
        return std::move(output).GetBuffer();
    }

    template <typename Protocols = DefaultProtocols, typename BufferPool, typename T>
    typename BufferPool::ConstBuffer Serialize(bond::ProtocolType protocol, std::shared_ptr<BufferPool> pool, const T& value, std::size_t minBlobSize = 0)
    {
        OutputBuffer<BufferPool> output{ std::move(pool), minBlobSize };
        bond::Apply<bond::Serializer, Protocols>(value, output, static_cast<std::uint16_t>(protocol));
        return std::move(output).GetBuffer();
    }

    template <template <typename> typename Reader, typename Protocols = DefaultProtocols, typename ConstBuffer, typename T>
    void Deserialize(ConstBuffer&& buffer, T& value, std::shared_ptr<SharedMemory> memory)
    {
        InputBuffer<std::decay_t<ConstBuffer>> input{ std::forward<ConstBuffer>(buffer), std::move(memory) };
        Reader<decltype(input)> reader{ std::move(input) };
        bond::Deserialize<Protocols>(reader, value);
    }

    template <typename Protocols = DefaultProtocols, typename ConstBuffer, typename T>
    void Deserialize(bond::ProtocolType protocol, ConstBuffer&& buffer, T& value, std::shared_ptr<SharedMemory> memory)
    {
        InputBuffer<std::decay_t<ConstBuffer>> input{ std::forward<ConstBuffer>(buffer), std::move(memory) };
        bond::Apply<T, Protocols>(bond::To<T, Protocols>{ value }, input, static_cast<std::uint16_t>(protocol));
    }

    template <template <typename> typename Writer, typename Protocols = DefaultProtocols, typename BufferPool, typename T>
    typename BufferPool::ConstBuffer Marshal(std::shared_ptr<BufferPool> pool, const T& value, std::size_t minBlobSize = 0)
    {
        OutputBuffer<BufferPool> output{ std::move(pool), minBlobSize };
        Writer<decltype(output)> writer{ output };
        bond::Marshal<Protocols>(value, writer);
        return std::move(output).GetBuffer();
    }

    template <typename Protocols = DefaultProtocols, typename BufferPool, typename T>
    typename BufferPool::ConstBuffer Marshal(bond::ProtocolType protocol, std::shared_ptr<BufferPool> pool, const T& value, std::size_t minBlobSize = 0)
    {
        OutputBuffer<BufferPool> output{ std::move(pool), minBlobSize };
        bond::Apply<bond::Marshaler, Protocols>(value, output, static_cast<std::uint16_t>(protocol));
        return std::move(output).GetBuffer();
    }

    template <typename Protocols = DefaultProtocols, typename ConstBuffer, typename T>
    void Unmarshal(ConstBuffer&& buffer, T& value, std::shared_ptr<SharedMemory> memory)
    {
        bond::Unmarshal<Protocols>(InputBuffer<std::decay_t<ConstBuffer>>{ std::forward<ConstBuffer>(buffer), std::move(memory) }, value);
    }


    template <typename BufferPool, typename ProtocolsT = DefaultProtocols>
    class Serializer    // TODO: Add compile-time protocol support.
    {
    public:
        using Protocols = ProtocolsT;

        Serializer(bond::ProtocolType protocol, bool marshal, std::shared_ptr<BufferPool> outputPool, std::shared_ptr<SharedMemory> inputMemory, std::size_t minBlobSize = 0)
            : m_outputPool{ std::move(outputPool) },
              m_inputMemory{ std::move(inputMemory) },
              m_protocol{ protocol },
              m_marshal{ marshal },
              m_minBlobSize{ minBlobSize }
        {}

        bond::ProtocolType GetProtocolType() const
        {
            return m_protocol;
        }

        bool IsMarshaled() const
        {
            return m_marshal;
        }

        template <typename T>
        typename BufferPool::ConstBuffer Serialize(const T& value)
        {
            return m_marshal
                ? Bond::Marshal<Protocols>(m_protocol, m_outputPool, value, m_minBlobSize)
                : Bond::Serialize<Protocols>(m_protocol, m_outputPool, value, m_minBlobSize);
        }

        template <typename T>
        void Deserialize(typename BufferPool::ConstBuffer&& buffer, T& value)
        {
            m_marshal
                ? Bond::Unmarshal<Protocols>(std::move(buffer), value, m_inputMemory)
                : Bond::Deserialize<Protocols>(m_protocol, std::move(buffer), value, m_inputMemory);
        }

        template <typename T>
        std::future<T> Deserialize(typename BufferPool::ConstBuffer buffer)
        {
            std::packaged_task<T()> task{
                [&]
                {
                    T value;
                    Deserialize(std::move(buffer), value);
                    return value;
                } };

            task();

            return task.get_future();
        }

        const std::shared_ptr<BufferPool>& GetOutputBufferPool() const
        {
            return m_outputPool;
        }

        const std::shared_ptr<SharedMemory>& GetInputMemory() const
        {
            return m_inputMemory;
        }

    private:
        std::shared_ptr<BufferPool> m_outputPool;
        std::shared_ptr<SharedMemory> m_inputMemory;
        bond::ProtocolType m_protocol;
        bool m_marshal;
        std::size_t m_minBlobSize;
    };


    using DefaultSerializer = Serializer<DefaultBufferPool>;

} // Bond
} // IPC
