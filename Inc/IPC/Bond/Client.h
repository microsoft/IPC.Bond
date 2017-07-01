#pragma once

#include "detail/ComponentBase.h"
#include <IPC/Client.h>
#include "DefaultTraits.h"
#include <bond/core/bond_const_enum.h>


namespace IPC
{
namespace Bond
{
    template <typename Request, typename Response, typename Traits = DefaultTraits>
    class Client : public detail::ComponentBase<IPC::Client, Request, Response, Traits>
    {
        using Base = detail::ComponentBase<IPC::Client, Request, Response, Traits>;

    public:
        template <typename CloseHandler>
        Client(
            typename Base::BufferPoolHolder pools,
            typename Base::Serializer serializer,
            std::unique_ptr<typename Base::Connection> connection,
            CloseHandler&& closeHandler,
            typename Base::TransactionManager transactionManager = {})
            : Base{ std::move(pools), std::move(serializer), std::move(connection), std::forward<CloseHandler>(closeHandler), std::move(transactionManager) }
        {}

        template <typename Callback, typename... TransactionArgs, typename U = Response, std::enable_if_t<!std::is_void<U>::value>* = nullptr,
            decltype(std::declval<Callback>()(std::declval<std::future<U>>()))* = nullptr>
        void operator()(const Request& request, Callback&& callback, TransactionArgs&&... transactionArgs)
        {
            Base::operator()(
                this->Serialize(request),
                [serializer = static_cast<typename Base::Serializer&>(*this), callback = std::forward<Callback>(callback)](typename Traits::BufferPool::ConstBuffer&& buffer) mutable
                {
                    callback(serializer.template Deserialize<Response>(std::move(buffer)));
                },
                std::forward<TransactionArgs>(transactionArgs)...);
        }

        template <typename U = Response, std::enable_if_t<std::is_void<U>::value>* = nullptr>
        void operator()(const Request& request)
        {
            Base::operator()(this->Serialize(request));
        }

        template <typename... TransactionArgs, typename U = Response, std::enable_if_t<!std::is_void<U>::value>* = nullptr>
        std::future<Response> operator()(const Request& request, TransactionArgs&&... transactionArgs)
        {
            std::packaged_task<Response(typename Traits::BufferPool::ConstBuffer&&)> callback{
                [serializer = static_cast<typename Base::Serializer&>(*this)](typename Traits::BufferPool::ConstBuffer&& buffer) mutable
                {
                    Response response;
                    serializer.Deserialize(std::move(buffer), response);
                    return response;
                } };

            auto result = callback.get_future();

            Base::operator()(this->Serialize(request), std::move(callback), std::forward<TransactionArgs>(transactionArgs)...);

            return result;
        }
    };


    template <typename Request, typename Response, typename Traits = DefaultTraits, typename CloseHandler>
    auto MakeClient(
        std::unique_ptr<typename Client<Request, Response, Traits>::Connection> connection,
        CloseHandler&& closeHandler,
        bond::ProtocolType protocol = bond::ProtocolType::COMPACT_PROTOCOL,
        bool marshal = true,
        std::size_t minBlobSize = 0,
        typename Client<Request, Response, Traits>::TransactionManager transactionManager = {})
    {
        auto pools = detail::MakeBufferPoolHolder<typename Traits::BufferPool>(*connection);
        typename Traits::Serializer serializer{ protocol, marshal, pools.GetOutputPool(), pools.GetInputPool()->GetMemory(), minBlobSize };

        return std::make_unique<Client<Request, Response, Traits>>(
            std::move(pools), std::move(serializer), std::move(connection), std::forward<CloseHandler>(closeHandler), std::move(transactionManager));
    }

} // Bond
} // IPC
