#pragma once

#include <memory>


namespace IPC
{
namespace Bond
{
    namespace detail
    {
        template <typename BufferPool>
        class BufferPoolHolder
        {
        public:
            BufferPoolHolder(std::shared_ptr<BufferPool> inPool, std::shared_ptr<BufferPool> outPool)
                : m_inPool{ std::move(inPool) },
                  m_outPool{ std::move(outPool) }
            {}

            const std::shared_ptr<BufferPool>& GetInputPool() const
            {
                return m_inPool;
            }

            const std::shared_ptr<BufferPool>& GetOutputPool() const
            {
                return m_outPool;
            }

        private:
            std::shared_ptr<BufferPool> m_inPool;
            std::shared_ptr<BufferPool> m_outPool;
        };


        template <typename BufferPool, typename Connection>
        auto MakeBufferPoolHolder(const Connection& connection)
        {
            auto pools = std::make_shared<std::pair<BufferPool, BufferPool>>(
                std::piecewise_construct,
                std::forward_as_tuple(connection.GetInputChannel().GetMemory()),
                std::forward_as_tuple(connection.GetOutputChannel().GetMemory()));

            auto& pair = *pools;

            std::shared_ptr<BufferPool> in{ pools, &pair.first };
            std::shared_ptr<BufferPool> out{ std::move(pools), &pair.second };

            return BufferPoolHolder<BufferPool>{ std::move(in), std::move(out) };
        }

    } // detail
} // Bond
} // IPC
