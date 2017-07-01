#pragma once

#include "detail/ComponentBase.h"
#include <IPC/Server.h>
#include "DefaultTraits.h"
#include <bond/core/bond_const_enum.h>


namespace IPC
{
namespace Bond
{
    template <typename Request, typename Response, typename Traits = DefaultTraits>
    class Server : public detail::ComponentBase<IPC::Server, Request, Response, Traits>
    {
        using Base = detail::ComponentBase<IPC::Server, Request, Response, Traits>;

    public:
        template <typename Handler, typename CloseHandler>
        Server(typename Base::BufferPoolHolder pools, typename Base::Serializer serializer, std::unique_ptr<typename Base::Connection> connection, Handler&& handler, CloseHandler&& closeHandler)
            : Base{
                std::move(pools),
                serializer,
                std::move(connection),
                [serializer, handler = std::forward<Handler>(handler)](typename Traits::BufferPool::ConstBuffer&& buffer, auto&& callback) mutable
                {
                    handler(
                        serializer.template Deserialize<Request>(std::move(buffer)),
                        [serializer, callback = std::forward<decltype(callback)>(callback)](const Response& response) mutable
                        {
                            callback(serializer.Serialize(response));
                        });
                },
                std::forward<CloseHandler>(closeHandler) }
        {}
    };


    template <typename Request, typename Response, typename Traits = DefaultTraits, typename HandlerFactory, typename CloseHandler>
    auto MakeServer(
        std::unique_ptr<typename Server<Request, Response, Traits>::Connection> connection,
        HandlerFactory&& handlerFactory,
        CloseHandler&& closeHandler,
        bond::ProtocolType protocol = bond::ProtocolType::COMPACT_PROTOCOL,
        bool marshal = true,
        std::size_t minBlobSize = 0)
    {
        auto pools = detail::MakeBufferPoolHolder<typename Traits::BufferPool>(*connection);
        typename Traits::Serializer serializer{ protocol, marshal, pools.GetOutputPool(), pools.GetInputPool()->GetMemory(), minBlobSize };

        auto handler = handlerFactory(*connection, pools, serializer);

        return std::make_unique<Server<Request, Response, Traits>>(
            std::move(pools), std::move(serializer), std::move(connection), std::move(handler), std::forward<CloseHandler>(closeHandler));
    }

} // Bond
} // IPC
