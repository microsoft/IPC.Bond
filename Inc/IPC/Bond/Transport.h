#pragma once

#include "Acceptor.h"
#include "Accept.h"
#include "Connector.h"
#include "Connect.h"
#include <memory>
#include <mutex>


namespace IPC
{
namespace Bond
{
    struct DefaultTraits;


    template <typename Request, typename Response, typename Traits = DefaultTraits>
    class Transport
    {
    public:
        using Client = Client<Request, Response, Traits>;
        using Server = Server<Request, Response, Traits>;
        using ClientConnector = ClientConnector<Request, Response, Traits>;
        using ServerConnector = ServerConnector<Request, Response, Traits>;
        using ClientAcceptor = ClientAcceptor<Request, Response, Traits>;
        using ServerAcceptor = ServerAcceptor<Request, Response, Traits>;

        Transport()
            : Transport{ bond::ProtocolType::COMPACT_PROTOCOL }
        {}

        explicit Transport(
            bond::ProtocolType protocol,
            bool marshal = true,
            ChannelSettings<Traits> channelSettings = {},
            std::size_t minBlobSize = 0,
            std::size_t hostInfoMemorySize = 0,
            typename Traits::TimeoutFactory timeoutFactory = {},
            typename Traits::ErrorHandler errorHandler = {},
            typename Traits::TransactionManagerFactory transactionManagerFactory = {})
            : m_protocol{ protocol },
              m_marshal{ marshal },
              m_channelSettings{ std::move(channelSettings) },
              m_minBlobSize{ minBlobSize },
              m_hostInfoMemorySize{ hostInfoMemorySize },
              m_timeoutFactory{ std::move(timeoutFactory) },
              m_errorHandler{ std::move(errorHandler) },
              m_transactionManagerFactory{ std::move(transactionManagerFactory) }
        {}

        template <typename CloseHandler>
        auto MakeClient(std::unique_ptr<typename Client::Connection> connection, CloseHandler&& closeHandler)
        {
            return IPC::Bond::MakeClient<Request, Response, Traits>(
                std::move(connection),
                std::forward<CloseHandler>(closeHandler),
                m_protocol,
                m_marshal,
                m_minBlobSize,
                m_transactionManagerFactory(IPC::detail::Identity<typename Client::TransactionManager>{}));
        }

        template <typename HandlerFactory, typename CloseHandler>
        auto MakeServer(std::unique_ptr<typename Server::Connection> connection, HandlerFactory&& handlerFactory, CloseHandler&& closeHandler)
        {
            return IPC::Bond::MakeServer<Request, Response, Traits>(
                std::move(connection),
                std::forward<HandlerFactory>(handlerFactory),
                std::forward<CloseHandler>(closeHandler),
                m_protocol,
                m_marshal,
                m_minBlobSize);
        }

        auto MakeClientConnector()
        {
            return ClientConnector{ m_channelSettings, m_transactionManagerFactory };
        }

        auto MakeServerConnector()
        {
            return ServerConnector{ m_channelSettings, m_transactionManagerFactory };
        }

        template <typename Handler>
        auto MakeServerAcceptor(const char* name, Handler&& handler)
        {
            return ServerAcceptor{ name, std::forward<Handler>(handler), m_channelSettings, m_hostInfoMemorySize };
        }

        template <typename Handler>
        auto MakeClientAcceptor(const char* name, Handler&& handler)
        {
            return ClientAcceptor{ name, std::forward<Handler>(handler), m_channelSettings, m_hostInfoMemorySize };
        }

        template <typename... TransactionArgs>
        auto ConnectClient(
            const char* name,
            bool async,
            std::shared_ptr<ClientConnector> connector = {},
            TransactionArgs&&... transactionArgs)
        {
            if (!connector)
            {
                std::call_once(
                    m_clientConnectorOnceFlag,
                    [this] { m_clientConnector = std::make_shared<ClientConnector>(MakeClientConnector()); });

                connector = m_clientConnector;
            }

            return IPC::Bond::ConnectClient(
                name,
                connector,
                async,
                m_protocol,
                m_marshal,
                m_minBlobSize,
                m_timeoutFactory,
                m_errorHandler,
                m_transactionManagerFactory,
                std::forward<TransactionArgs>(transactionArgs)...);
        }

        template <typename HandlerFactory, typename... TransactionArgs>
        auto ConnectServer(
            const char* name,
            HandlerFactory&& handlerFactory,
            bool async,
            std::shared_ptr<ServerConnector> connector = {},
            TransactionArgs&&... transactionArgs)
        {
            if (!connector)
            {
                std::call_once(
                    m_serverConnectorOnceFlag,
                    [this] { m_serverConnector = std::make_shared<ServerConnector>(MakeServerConnector()); });

                connector = m_serverConnector;
            }

            return IPC::Bond::ConnectServer(
                name,
                connector,
                std::forward<HandlerFactory>(handlerFactory),
                async,
                m_protocol,
                m_marshal,
                m_minBlobSize,
                m_timeoutFactory,
                m_errorHandler,
                std::forward<TransactionArgs>(transactionArgs)...);
        }

        template <typename HandlerFactory>
        auto AcceptServers(const char* name, HandlerFactory&& handlerFactory)
        {
            return IPC::Bond::AcceptServers<Request, Response, Traits>(
                name,
                std::forward<HandlerFactory>(handlerFactory),
                m_protocol,
                m_marshal,
                m_channelSettings,
                m_minBlobSize,
                m_hostInfoMemorySize,
                m_errorHandler);
        }

        auto AcceptClients(const char* name)
        {
            return IPC::Bond::AcceptClients<Request, Response, Traits>(
                name,
                m_protocol,
                m_marshal,
                m_channelSettings,
                m_minBlobSize,
                m_hostInfoMemorySize,
                m_errorHandler,
                m_transactionManagerFactory);
        }

    private:
        bond::ProtocolType m_protocol;
        bool m_marshal;
        ChannelSettings<Traits> m_channelSettings;
        std::size_t m_minBlobSize;
        std::size_t m_hostInfoMemorySize;
        typename Traits::TimeoutFactory m_timeoutFactory;
        typename Traits::ErrorHandler m_errorHandler;
        typename Traits::TransactionManagerFactory m_transactionManagerFactory;
        std::shared_ptr<ClientConnector> m_clientConnector;
        std::shared_ptr<ServerConnector> m_serverConnector;
        std::once_flag m_clientConnectorOnceFlag;
        std::once_flag m_serverConnectorOnceFlag;
    };

} // Bond
} // IPC
