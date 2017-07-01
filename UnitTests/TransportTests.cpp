#include "stdafx.h"
#include "IPC/Bond/Transport.h"
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


BOOST_AUTO_TEST_SUITE(TransportTests)

struct Traits : IPC::Bond::DefaultTraits
{
    using TimeoutFactory = IPC::Policies::NullTimeoutFactory;   // Using no-timeout to make these tests reliable.

    template <typename Context>
    using TransactionManager = IPC::Policies::TransactionManager<Context, TimeoutFactory>;
};

BOOST_AUTO_TEST_CASE(AcceptorConnectorTest)
{
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

    using Transport = IPC::Bond::Transport<bond::Box<int>, bond::Box<double>, Traits>;

    Transport transport;

    auto name = IPC::detail::GenerateRandomString();

    std::mutex lock;
    std::condition_variable serverInserted;
    std::vector<std::unique_ptr<Transport::Server>> servers;

    auto acceptor = transport.MakeServerAcceptor(
        name.c_str(),
        [&](auto futureConnection)
        {
            try
            {
                std::lock_guard<std::mutex> guard{ lock };
                servers.push_back(transport.MakeServer(futureConnection.get(), [&](auto&&...) { return serverHandler; }, closeHandler));
                serverInserted.notify_one();
            }
            catch (const std::exception& e)
            {
                std::clog << "Failed to accept a server: " << e.what() << std::endl;
            }
        });

    std::unique_ptr<Transport::Client> client;

    auto connector = transport.MakeClientConnector();

    try
    {
        client = transport.MakeClient(connector.Connect(name.c_str()).get(), closeHandler);
    }
    catch (const std::exception& e)
    {
        std::clog << "Failed to connect a client: " << e.what() << std::endl;
    }

    BOOST_TEST_REQUIRE(!!client);

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

    using Transport = IPC::Bond::Transport<bond::Box<int>, bond::Box<double>, Traits>;

    Transport transport;

    auto name = IPC::detail::GenerateRandomString();

    std::mutex lock;
    std::condition_variable clientInserted;
    std::vector<std::unique_ptr<Transport::Client>> clients;

    auto acceptor = transport.MakeClientAcceptor(
        name.c_str(),
        [&](auto futureConnection)
        {
            try
            {
                std::lock_guard<std::mutex> guard{ lock };
                clients.push_back(transport.MakeClient(futureConnection.get(), closeHandler));
                clientInserted.notify_one();
            }
            catch (const std::exception& e)
            {
                std::clog << "Failed to accept a client: " << e.what() << std::endl;
            }
        });

    std::unique_ptr<Transport::Server> server;

    auto connector = transport.MakeServerConnector();

    try
    {
        server = transport.MakeServer(connector.Connect(name.c_str()).get(), [&](auto&&...) { return serverHandler; }, closeHandler);
    }
    catch (const std::exception& e)
    {
        std::clog << "Failed to connect a server: " << e.what() << std::endl;
    }

    BOOST_TEST_REQUIRE(!!server);

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

    using Transport = IPC::Bond::Transport<bond::Box<int>, bond::Box<double>, Traits>;

    Transport transport;

    auto name = IPC::detail::GenerateRandomString();

    auto serversAccessor = transport.AcceptServers(name.c_str(), [&](auto&&...) { return serverHandler; });

    auto clientAccessor = transport.ConnectClient(name.c_str(), false);

    std::shared_ptr<Transport::Client> client;

    try
    {
        client = clientAccessor();
    }
    catch (const std::exception& e)
    {
        std::clog << "Client is not available: " << e.what() << std::endl;
    }

    BOOST_TEST_REQUIRE(!!client);

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

    using Transport = IPC::Bond::Transport<bond::Box<int>, bond::Box<double>, Traits>;

    Transport transport;

    auto name = IPC::detail::GenerateRandomString();

    auto clientsAccessor = transport.AcceptClients(name.c_str());

    auto serverAccessor = transport.ConnectServer(name.c_str(), [&](auto&&...) { return serverHandler; }, false);

    std::shared_ptr<Transport::Server> server;

    try
    {
        server = serverAccessor();
    }
    catch (const std::exception& e)
    {
        std::clog << "Server is not available: " << e.what() << std::endl;
    }

    BOOST_TEST_REQUIRE(!!server);

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

    using Transport = IPC::Bond::Transport<Struct, Struct, Traits>;

    Transport transport;

    auto name = IPC::detail::GenerateRandomString();

    auto serversAccessor = transport.AcceptServers(
        name.c_str(),
        [&](const auto& /*connection*/, const auto& pools, const auto& /*serializer*/)
        {
            return [&serverHandler, pool = pools.GetOutputPool()](std::future<Struct> request, auto&& callback)
            {
                return serverHandler(*pool, std::move(request), std::forward<decltype(callback)>(callback));
            };
        });

    auto clientAccessor = transport.ConnectClient(name.c_str(), false);

    std::shared_ptr<Transport::Client> client;

    try
    {
        client = clientAccessor();
    }
    catch (const std::exception& e)
    {
        std::clog << "Client is not available: " << e.what() << std::endl;
    }

    BOOST_TEST_REQUIRE(!!client);

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
