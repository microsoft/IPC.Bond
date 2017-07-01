#include "stdafx.h"
#include "IPC/Bond/Accept.h"
#include "IPC/Bond/Connect.h"
#include "IPC/Bond/Connector.h"
#include "IPC/detail/RandomString.h"
#include <bond/core/bond.h>
#include <bond/protocol/simple_json_writer.h>
#include <bond/core/tuple.h>

using namespace IPC::Bond;
using IPC::detail::GenerateRandomString;
using IPC::SharedMemory;
using IPC::create_only;


BOOST_AUTO_TEST_SUITE(ConnectAcceptTests)

using Request = std::tuple<int>;
using Response = std::tuple<int, int>;

class SerializerMock : public DefaultSerializer
{
public:
    SerializerMock(
        bond::ProtocolType protocol,
        bool marshal,
        std::shared_ptr<DefaultBufferPool> outputPool,
        std::shared_ptr<SharedMemory> inputMemory = {},
        std::size_t minBlobSize = 0)
        : DefaultSerializer{ protocol, marshal, outputPool, std::move(inputMemory), minBlobSize },
          m_outputPool{ std::move(outputPool) }
    {}

    const auto& GetSerializerPool() const
    {
        return m_outputPool;
    }

private:
    std::shared_ptr<DefaultBufferPool> m_outputPool;
};

struct MockTraits : DefaultTraits
{
    using Serializer = SerializerMock;
};

using ClientConnector = ClientConnector<Request, Response, MockTraits>;
using ServerConnector = ServerConnector<Request, Response, MockTraits>;

BOOST_AUTO_TEST_CASE(SerializerTest)
{
    auto name = GenerateRandomString();

    auto serversAccessor = AcceptServers<Request, Response, MockTraits>(
        name.c_str(),
        [](auto&&...) { return [](auto&&...) {}; });

    auto clientAccessor = ConnectClient(name.c_str(), std::make_shared<ClientConnector>(), false);

    while (true)
    {
        auto servers = serversAccessor();
        if (servers->empty())
        {
            std::this_thread::yield();
            continue;
        }

        const auto& server = servers->front();

        BOOST_TEST(server->GetSerializerPool() == server->GetOutputPool());

        break;
    }

    auto client = clientAccessor();

    BOOST_TEST(client->GetSerializerPool() == client->GetOutputPool());
}

BOOST_AUTO_TEST_CASE(ReverseConnectionSerializerTest)
{
    auto name = GenerateRandomString();

    auto clientsAccessor = AcceptClients<Request, Response, MockTraits>(name.c_str());

    auto serverAccessor = ConnectServer(
        name.c_str(),
        std::make_shared<ServerConnector>(),
        [](auto&&...) { return [](auto&&...) {}; },
        false);

    while (true)
    {
        auto clients = clientsAccessor();
        if (clients->empty())
        {
            std::this_thread::yield();
            continue;
        }

        const auto& client = clients->front();

        BOOST_TEST(client->GetSerializerPool() == client->GetOutputPool());

        break;
    }

    auto server = serverAccessor();

    BOOST_TEST(server->GetSerializerPool() == server->GetOutputPool());
}

BOOST_AUTO_TEST_CASE(BufferPoolTest)
{
    auto name = GenerateRandomString();

    auto serversAccessor = AcceptServers<Request, Response, MockTraits>(
        name.c_str(),
        [](auto&&...) { return [](auto&&...) {}; });

    auto clientAccessor = ConnectClient(name.c_str(), std::make_shared<ClientConnector>(), false);

    while (true)
    {
        auto servers = serversAccessor();
        if (servers->empty())
        {
            std::this_thread::yield();
            continue;
        }

        const auto& server = servers->front();

        BOOST_TEST(server->GetConnection().GetInputChannel().GetMemory() == server->GetInputPool()->GetMemory());
        BOOST_TEST(server->GetConnection().GetOutputChannel().GetMemory() == server->GetOutputPool()->GetMemory());

        break;
    }

    auto client = clientAccessor();

    BOOST_TEST(client->GetConnection().GetInputChannel().GetMemory() == client->GetInputPool()->GetMemory());
    BOOST_TEST(client->GetConnection().GetOutputChannel().GetMemory() == client->GetOutputPool()->GetMemory());
}

BOOST_AUTO_TEST_CASE(ReverseConnectionBufferPoolTest)
{
    auto name = GenerateRandomString();

    auto clientsAccessor = AcceptClients<Request, Response, MockTraits>(name.c_str());

    auto serverAccessor = ConnectServer(
        name.c_str(),
        std::make_shared<ServerConnector>(),
        [](auto&&...) { return [](auto&&...) {}; },
        false);

    while (true)
    {
        auto clients = clientsAccessor();
        if (clients->empty())
        {
            std::this_thread::yield();
            continue;
        }

        const auto& client = clients->front();

        BOOST_TEST(client->GetConnection().GetInputChannel().GetMemory() == client->GetInputPool()->GetMemory());
        BOOST_TEST(client->GetConnection().GetOutputChannel().GetMemory() == client->GetOutputPool()->GetMemory());

        break;
    }

    auto server = serverAccessor();

    BOOST_TEST(server->GetConnection().GetInputChannel().GetMemory() == server->GetInputPool()->GetMemory());
    BOOST_TEST(server->GetConnection().GetOutputChannel().GetMemory() == server->GetOutputPool()->GetMemory());
}

BOOST_AUTO_TEST_SUITE_END()
