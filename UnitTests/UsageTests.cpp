#include "stdafx.h"
#include "IPC/Bond/Accept.h"
#include "IPC/Bond/Acceptor.h"
#include "IPC/Bond/Connect.h"
#include "IPC/Bond/Connector.h"
#include "IPC/detail/RandomString.h"
#include <cmath>
#include <memory>
#include <vector>
#include <iostream>
#include <algorithm>
#include <mutex>
#include <condition_variable>
#include <future>

#include <bond/core/bond_types.h>
#include <bond/protocol/simple_json_writer.h>


BOOST_AUTO_TEST_SUITE(UsageTests)

struct Traits : IPC::Bond::DefaultTraits
{
    using TimeoutFactory = IPC::Policies::NullTimeoutFactory;   // Using no-timeout to make these tests reliable.

    template <typename Context>
    using TransactionManager = IPC::Policies::TransactionManager<Context, TimeoutFactory>;
};

BOOST_AUTO_TEST_CASE(AcceptorConnectorTest)
{
    constexpr auto protocol = bond::ProtocolType::COMPACT_PROTOCOL;
    constexpr auto marshal = false;

    auto serverHandler = [](std::future<bond::Box<int>> x, auto&& callback)
    {
        try
        {
            bond::Box<double> y;
            y.value = std::sqrt(x.get().value);

            callback(y);
        }
        catch (const std::exception& e)
        {
            std::clog << "Failed to send response: " << e.what() << std::endl;
        }
    };

    auto closeHandler = []
    {
        std::clog << "Connection is closed." << std::endl;
    };

    auto name = IPC::detail::GenerateRandomString();

    std::mutex lock;
    std::condition_variable serverInserted;
    std::vector<std::unique_ptr<IPC::Bond::Server<bond::Box<int>, bond::Box<double>, Traits>>> servers;

    IPC::Bond::ServerAcceptor<bond::Box<int>, bond::Box<double>, Traits> acceptor{
        name.c_str(),
        [&](auto futureConnection)
        {
            try
            {
                std::lock_guard<std::mutex> guard{ lock };
                servers.push_back(IPC::Bond::MakeServer<bond::Box<int>, bond::Box<double>, Traits>(
                    futureConnection.get(),
                    [&](auto&&...) { return serverHandler; },
                    closeHandler,
                    protocol,
                    marshal));
                serverInserted.notify_one();
            }
            catch (const std::exception& e)
            {
                std::clog << "Failed to accept a server: " << e.what() << std::endl;
            }
        } };

    std::unique_ptr<IPC::Bond::Client<bond::Box<int>, bond::Box<double>, Traits>> client;

    IPC::Bond::ClientConnector<bond::Box<int>, bond::Box<double>, Traits> connector;

    try
    {
        client = IPC::Bond::MakeClient<bond::Box<int>, bond::Box<double>, Traits>(
            connector.Connect(name.c_str()).get(), closeHandler, protocol, marshal);
    }
    catch (const std::exception& e)
    {
        std::clog << "Failed to connect a client: " << e.what() << std::endl;
    }

    {
        std::unique_lock<std::mutex> guard{ lock };
        serverInserted.wait(guard, [&] { return !servers.empty(); });
    }

    BOOST_TEST(servers.size() == 1);

    bond::Box<int> x;
    x.value = 111;

    bond::Box<double> y;

    try
    {
        y = (*client)(x).get();
    }
    catch (const std::exception& e)
    {
        std::clog << "Failed to invoke a client: " << e.what() << std::endl;
    }

    BOOST_TEST(y.value == std::sqrt(x.value), boost::test_tools::tolerance(0.0001));
}

BOOST_AUTO_TEST_CASE(ReverseConnectionAcceptorConnectorTest)
{
    constexpr auto protocol = bond::ProtocolType::COMPACT_PROTOCOL;
    constexpr auto marshal = false;

    auto serverHandler = [](std::future<bond::Box<int>> x, auto&& callback)
    {
        try
        {
            bond::Box<double> y;
            y.value = std::sqrt(x.get().value);

            callback(std::move(y));
        }
        catch (const std::exception& e)
        {
            std::clog << "Failed to send response: " << e.what() << std::endl;
        }
    };

    auto closeHandler = []
    {
        std::clog << "Connection is closed." << std::endl;
    };

    auto name = IPC::detail::GenerateRandomString();

    std::mutex lock;
    std::condition_variable clientInserted;
    std::vector<std::unique_ptr<IPC::Bond::Client<bond::Box<int>, bond::Box<double>, Traits>>> clients;

    IPC::Bond::ClientAcceptor<bond::Box<int>, bond::Box<double>, Traits> acceptor{
        name.c_str(),
        [&](auto futureConnection)
        {
            try
            {
                std::lock_guard<std::mutex> guard{ lock };
                clients.push_back(IPC::Bond::MakeClient<bond::Box<int>, bond::Box<double>, Traits>(
                    futureConnection.get(), closeHandler, protocol, marshal));
                clientInserted.notify_one();
            }
            catch (const std::exception& e)
            {
                std::clog << "Failed to accept a client: " << e.what() << std::endl;
            }
        } };

    std::unique_ptr<IPC::Bond::Server<bond::Box<int>, bond::Box<double>, Traits>> server;

    IPC::Bond::ServerConnector<bond::Box<int>, bond::Box<double>, Traits> connector;

    try
    {
        server = IPC::Bond::MakeServer<bond::Box<int>, bond::Box<double>, Traits>(
            connector.Connect(name.c_str()).get(),
            [&](auto&&...) { return serverHandler; },
            closeHandler,
            protocol,
            marshal);
    }
    catch (const std::exception& e)
    {
        std::clog << "Failed to connect a server: " << e.what() << std::endl;
    }

    {
        std::unique_lock<std::mutex> guard{ lock };
        clientInserted.wait(guard, [&] { return !clients.empty(); });
    }

    BOOST_TEST(clients.size() == 1);

    bond::Box<int> x;
    x.value = 222;

    bond::Box<double> y;

    try
    {
        y = (*clients.front())(x).get();
    }
    catch (const std::exception& e)
    {
        std::clog << "Failed to invoke a client: " << e.what() << std::endl;
    }

    BOOST_TEST(y.value == std::sqrt(x.value), boost::test_tools::tolerance(0.0001));
}

BOOST_AUTO_TEST_CASE(AcceptConnectTest)
{
    auto serverHandler = [](std::future<bond::Box<int>> x, auto&& callback)
    {
        try
        {
            bond::Box<double> y;
            y.value = std::sqrt(x.get().value);

            callback(std::move(y));
        }
        catch (const std::exception& e)
        {
            std::clog << "Failed to send response: " << e.what() << std::endl;
        }
    };

    auto name = IPC::detail::GenerateRandomString();

    auto serversAccessor = IPC::Bond::AcceptServers<bond::Box<int>, bond::Box<double>, Traits>(
        name.c_str(), [&](auto&&...) { return serverHandler; });

    auto clientAccessor = IPC::Bond::ConnectClient(
        name.c_str(), std::make_shared<IPC::Bond::ClientConnector<bond::Box<int>, bond::Box<double>, Traits>>(), false);

    std::shared_ptr<IPC::Bond::Client<bond::Box<int>, bond::Box<double>, Traits>> client;

    try
    {
        client = clientAccessor();
    }
    catch (const std::exception& e)
    {
        std::clog << "Client is not available: " << e.what() << std::endl;
    }

    assert(client);

    bond::Box<int> x;
    x.value = 333;

    bond::Box<double> y;

    try
    {
        y = (*client)(x).get();
    }
    catch (const std::exception& e)
    {
        std::clog << "Failed to invoke a client: " << e.what() << std::endl;
    }

    BOOST_TEST(y.value == std::sqrt(x.value), boost::test_tools::tolerance(0.0001));
}

BOOST_AUTO_TEST_CASE(ReverseConnectionAcceptConnectTest)
{
    auto serverHandler = [](std::future<bond::Box<int>> x, auto&& callback)
    {
        try
        {
            bond::Box<double> y;
            y.value = std::sqrt(x.get().value);

            callback(std::move(y));
        }
        catch (const std::exception& e)
        {
            std::clog << "Failed to send response: " << e.what() << std::endl;
        }
    };

    auto name = IPC::detail::GenerateRandomString();

    auto clientsAccessor = IPC::Bond::AcceptClients<bond::Box<int>, bond::Box<double>, Traits>(name.c_str());

    auto serverAccessor = IPC::Bond::ConnectServer(
        name.c_str(),
        std::make_shared<IPC::Bond::ServerConnector<bond::Box<int>, bond::Box<double>, Traits>>(),
        [&](auto&&...) { return serverHandler; },
        false);

    std::shared_ptr<IPC::Bond::Server<bond::Box<int>, bond::Box<double>, Traits>> server;

    try
    {
        server = serverAccessor();
    }
    catch (const std::exception& e)
    {
        std::clog << "Server is not available: " << e.what() << std::endl;
    }

    assert(server);

    bond::Box<int> x;
    x.value = 444;

    bond::Box<double> y;

    try
    {
        y = (*clientsAccessor()->front())(x).get();
    }
    catch (const std::exception& e)
    {
        std::clog << "Failed to invoke a client: " << e.what() << std::endl;
    }

    BOOST_TEST(y.value == std::sqrt(x.value), boost::test_tools::tolerance(0.0001));
}

BOOST_AUTO_TEST_CASE(DynamicDataTest)
{
    using Struct = bond::Box<bond::blob>;

    auto serverHandler = [](IPC::Bond::DefaultBufferPool& pool, std::future<Struct> futureRequest, auto&& callback)
    {
        try
        {
            auto request = futureRequest.get();

            auto blob = pool.TakeBlob();
            blob->resize(2 * request.value.size(), boost::container::default_init);
            std::memcpy(blob->data(), request.value.data(), request.value.size());
            std::memcpy(blob->data() + request.value.size(), request.value.data(), request.value.size());

            Struct response;
            response.value = IPC::Bond::BlobCast(IPC::Bond::DefaultBufferPool::ConstBlob{ std::move(blob) }, pool.GetMemory());

            callback(std::move(response));
        }
        catch (const std::exception& e)
        {
            std::clog << "Failed to send response: " << e.what() << std::endl;
        }
    };

    auto name = IPC::detail::GenerateRandomString();

    auto serversAccessor = IPC::Bond::AcceptServers<Struct, Struct, Traits>(
        name.c_str(),
        [&](const auto& /*connection*/, const auto& pools, const auto& /*serializer*/)
        {
            return [&serverHandler, pool = pools.GetOutputPool()](std::future<Struct> request, auto&& callback)
            {
                return serverHandler(*pool, std::move(request), std::forward<decltype(callback)>(callback));
            };
        });

    auto clientAccessor = IPC::Bond::ConnectClient(
        name.c_str(),
        std::make_shared<IPC::Bond::ClientConnector<Struct, Struct, Traits>>(),
        false);

    std::shared_ptr<IPC::Bond::Client<Struct, Struct, Traits>> client;

    try
    {
        client = clientAccessor();
    }
    catch (const std::exception& e)
    {
        std::clog << "Client is not available: " << e.what() << std::endl;
    }

    assert(client);

    const char str[] = "Hello World";
    Struct result;

    try
    {
        auto pool = client->GetOutputPool();
        auto blob = pool->TakeBlob();
        blob->resize(sizeof(str), boost::container::default_init);
        std::strcpy(blob->data(), str);

        Struct request;
        request.value = IPC::Bond::BlobCast(IPC::Bond::DefaultBufferPool::ConstBlob{ std::move(blob) }, pool->GetMemory());

        result = (*client)(std::move(request)).get();
    }
    catch (const std::exception& e)
    {
        std::clog << "Failed to invoke a client: " << e.what() << std::endl;
    }

    BOOST_TEST(result.value.size() == 2 * sizeof(str));
    BOOST_TEST(std::strcmp(result.value.content(), str) == 0);
    BOOST_TEST(std::strcmp(result.value.content() + sizeof(str), str) == 0);
}

BOOST_AUTO_TEST_SUITE_END()
