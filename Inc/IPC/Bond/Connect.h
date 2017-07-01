#pragma once

#include <IPC/detail/Connect.h>
#include <IPC/Policies/TimeoutFactory.h>
#include <IPC/Policies/ErrorHandler.h>
#include "Client.h"
#include "Server.h"
#include <bond/core/bond_const_enum.h>
#include <memory>
#include <chrono>


namespace IPC
{
namespace Bond
{
    template <
        typename PacketConnector,
        typename TimeoutFactory = typename PacketConnector::Traits::TimeoutFactory,
        typename ErrorHandler = typename PacketConnector::Traits::ErrorHandler,
        typename... TransactionArgs>
    auto ConnectClient(
        const char* acceptorName,
        std::shared_ptr<PacketConnector> connector,
        bool async,
        bond::ProtocolType protocol = bond::ProtocolType::COMPACT_PROTOCOL,
        bool marshal = true,
        std::size_t minBlobSize = 0,
        TimeoutFactory&& timeoutFactory = { std::chrono::seconds{ 1 } },
        ErrorHandler&& errorHandler = {},
        typename PacketConnector::Traits::TransactionManagerFactory transactionManagerFactory = {},
        TransactionArgs&&... transactionArgs)
    {
        return IPC::detail::Connect(
            acceptorName,
            std::move(connector),
            async,
            std::forward<TimeoutFactory>(timeoutFactory),
            std::forward<ErrorHandler>(errorHandler),
            [protocol, marshal, minBlobSize, transactionManagerFactory = std::move(transactionManagerFactory)](auto&& connection, auto&& callback)
            {
                using Client = Client<typename PacketConnector::Request, typename PacketConnector::Response, typename PacketConnector::Traits>;

                return MakeClient<typename PacketConnector::Request, typename PacketConnector::Response, typename PacketConnector::Traits>(
                    std::move(connection),
                    std::forward<decltype(callback)>(callback),
                    protocol,
                    marshal,
                    minBlobSize,
                    transactionManagerFactory(IPC::detail::Identity<typename Client::TransactionManager>{}));
            },
            std::forward<TransactionArgs>(transactionArgs)...);
    }

    template <
        typename PacketConnector,
        typename HandlerFactory,
        typename TimeoutFactory = typename PacketConnector::Traits::TimeoutFactory,
        typename ErrorHandler = typename PacketConnector::Traits::ErrorHandler,
        typename... TransactionArgs>
    auto ConnectServer(
        const char* acceptorName,
        std::shared_ptr<PacketConnector> connector,
        HandlerFactory&& handlerFactory,
        bool async,
        bond::ProtocolType protocol = bond::ProtocolType::COMPACT_PROTOCOL,
        bool marshal = true,
        std::size_t minBlobSize = 0,
        TimeoutFactory&& timeoutFactory = { std::chrono::seconds{ 1 } },
        ErrorHandler&& errorHandler = {},
        TransactionArgs&&... transactionArgs)
    {
        return IPC::detail::Connect(
            acceptorName,
            std::move(connector),
            async,
            std::forward<TimeoutFactory>(timeoutFactory),
            std::forward<ErrorHandler>(errorHandler),
            [protocol, marshal, minBlobSize, handlerFactory = std::forward<HandlerFactory>(handlerFactory)](auto&& connection, auto&& callback) mutable
            {
                return MakeServer<typename PacketConnector::Request, typename PacketConnector::Response, typename PacketConnector::Traits>(
                    std::move(connection), handlerFactory, std::forward<decltype(callback)>(callback), protocol, marshal, minBlobSize);
            },
            std::forward<TransactionArgs>(transactionArgs)...);
    }

} // Bond
} // IPC
