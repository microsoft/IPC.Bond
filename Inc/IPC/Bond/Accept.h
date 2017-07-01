#pragma once

#include <IPC/detail/Accept.h>
#include <IPC/Policies/ErrorHandler.h>
#include <IPC/ComponentCollection.h>
#include "Acceptor.h"
#include "Client.h"
#include "Server.h"
#include "DefaultTraits.h"
#include <bond/core/bond_const_enum.h>
#include <memory>


namespace IPC
{
namespace Bond
{
    template <typename Request, typename Response, typename Traits = DefaultTraits, typename HandlerFactory, typename ErrorHandler = typename Traits::ErrorHandler>
    auto AcceptServers(
        const char* name,
        HandlerFactory&& handlerFactory,
        bond::ProtocolType protocol = bond::ProtocolType::COMPACT_PROTOCOL,
        bool marshal = true,
        ChannelSettings<Traits> channelSettings = {},
        std::size_t minBlobSize = 0,
        std::size_t hostInfoMemorySize = 0,
        ErrorHandler&& errorHandler = {})
    {
        return IPC::detail::Accept<ServerAcceptor<Request, Response, Traits>>(
            name,
            std::make_shared<ServerCollection<Server<Request, Response, Traits>>>(),
            [protocol, marshal, minBlobSize, handlerFactory = std::forward<HandlerFactory>(handlerFactory)](auto&& connection, auto&& closeHandler) mutable
            {
                return MakeServer<Request, Response, Traits>(
                    std::move(connection), handlerFactory, std::forward<decltype(closeHandler)>(closeHandler), protocol, marshal, minBlobSize);
            },
            std::forward<ErrorHandler>(errorHandler),
            std::move(channelSettings),
            hostInfoMemorySize);
    }


    template <typename Request, typename Response, typename Traits = DefaultTraits, typename ErrorHandler = typename Traits::ErrorHandler>
    auto AcceptClients(
        const char* name,
        bond::ProtocolType protocol = bond::ProtocolType::COMPACT_PROTOCOL,
        bool marshal = true,
        ChannelSettings<Traits> channelSettings = {},
        std::size_t minBlobSize = 0,
        std::size_t hostInfoMemorySize = 0,
        ErrorHandler&& errorHandler = {},
        typename Traits::TransactionManagerFactory transactionManagerFactory = {})
    {
        using Client = Client<Request, Response, Traits>;

        return IPC::detail::Accept<ClientAcceptor<Request, Response, Traits>>(
            name,
            std::make_shared<ClientCollection<Client>>(),
            [protocol, marshal, minBlobSize, transactionManagerFactory = std::move(transactionManagerFactory)](auto&& connection, auto&& closeHandler)
            {
                return MakeClient<Request, Response, Traits>(
                    std::move(connection),
                    std::forward<decltype(closeHandler)>(closeHandler),
                    protocol,
                    marshal,
                    minBlobSize,
                    transactionManagerFactory(IPC::detail::Identity<typename Client::TransactionManager>{}));
            },
            std::forward<ErrorHandler>(errorHandler),
            std::move(channelSettings),
            hostInfoMemorySize);
    }

} // Bond
} // IPC
