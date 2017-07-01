#include "stdafx.h"
#include "IPC/Bond/Server.h"
#include "IPC/Bond/Client.h"
#include "IPC/Bond/Acceptor.h"
#include "IPC/Bond/Connector.h"
#include "IPC/detail/RandomString.h"
#include <bond/core/tuple.h>

using namespace IPC::Bond;
using IPC::detail::GenerateRandomString;
using IPC::SharedMemory;
using IPC::create_only;


BOOST_AUTO_TEST_SUITE(ClientServerTests)

using Request = std::tuple<int>;
using Response = std::tuple<int, int>;

class SerializerMock
{
public:
    template <typename T>
    DefaultBufferPool::ConstBuffer Serialize(const T& value);

    template <>
    DefaultBufferPool::ConstBuffer Serialize(const Request& /*value*/)
    {
        ++m_counters->m_requestSerialized;
        return{};
    }

    template <>
    DefaultBufferPool::ConstBuffer Serialize(const Response& /*value*/)
    {
        ++m_counters->m_responseSerialized;
        return{};
    }

    template <typename T>
    void Deserialize(DefaultBufferPool::ConstBuffer&& buffer, T& value);

    template <>
    void Deserialize(DefaultBufferPool::ConstBuffer&& /*buffer*/, Response& /*value*/)
    {
        ++m_counters->m_responseDeserialized;
    }

    template <typename T>
    std::future<T> Deserialize(DefaultBufferPool::ConstBuffer buffer);

    template <>
    std::future<Request> Deserialize(DefaultBufferPool::ConstBuffer /*buffer*/)
    {
        ++m_counters->m_requestDeserialized;
        return Deserialize<Request>();
    }

    template <>
    std::future<Response> Deserialize(DefaultBufferPool::ConstBuffer /*buffer*/)
    {
        ++m_counters->m_responseDeserialized;
        return Deserialize<Response>();
    }

    bool CheckClientUsage() const
    {
        return m_counters->m_requestSerialized == 1
            && m_counters->m_requestDeserialized == 0
            && m_counters->m_responseSerialized == 0
            && m_counters->m_responseDeserialized == 1;
    }

    bool CheckServerUsage() const
    {
        return m_counters->m_requestSerialized == 0
            && m_counters->m_requestDeserialized == 1
            && m_counters->m_responseSerialized == 1
            && m_counters->m_responseDeserialized == 0;
    }

    void ResetCounters()
    {
        m_counters->Reset();
    }

private:
    template <typename T>
    std::future<T> Deserialize()
    {
        std::promise<T> promise;
        promise.set_value({});
        return promise.get_future();
    }

    struct Counters
    {
        void Reset()
        {
            *this = {};
        }

        std::size_t m_requestSerialized{ 0 };
        std::size_t m_requestDeserialized{ 0 };
        std::size_t m_responseSerialized{ 0 };
        std::size_t m_responseDeserialized{ 0 };
    };

    std::shared_ptr<Counters> m_counters{ std::make_shared<Counters>() };
};

struct MockTraits : DefaultTraits
{
    using Serializer = SerializerMock;
};

using Server = Server<Request, Response, MockTraits>;
using Client = Client<Request, Response, MockTraits>;
using Acceptor = ServerAcceptor<Request, Response, MockTraits>;
using Connector = ClientConnector<Request, Response, MockTraits>;

BOOST_AUTO_TEST_CASE(SerializerTest)
{
    auto name = GenerateRandomString();

    std::unique_ptr<Server> server;

    Acceptor acceptor{
        name.c_str(),
        [&](auto&& futureConnection)
        {
            server = std::make_unique<Server>(
                detail::BufferPoolHolder<DefaultBufferPool>{ nullptr, nullptr },
                SerializerMock{},
                futureConnection.get(),
                [](auto futureRequest, auto&& callback)
                {
                    auto request = futureRequest.get();
                    callback(std::tuple_cat(request, request));
                },
                [] {});
        } };

    Client client{
        detail::BufferPoolHolder<DefaultBufferPool>{ nullptr, nullptr },
        SerializerMock{},
        Connector{}.Connect(name.c_str()).get(),
        [] {},
        {} };

    std::promise<Response> result;
    client(Request{}, [&](std::future<Response>&& response) { result.set_value(response.get()); });
    BOOST_TEST((result.get_future().get() == Response{}));
    BOOST_TEST(client.CheckClientUsage());
    BOOST_TEST(server->CheckServerUsage());

    client.ResetCounters();
    server->ResetCounters();
    BOOST_TEST((client(Request{}).get() == Response{}));
    BOOST_TEST(client.CheckClientUsage());
    BOOST_TEST(server->CheckServerUsage());

    client.ResetCounters();
    server->ResetCounters();
    BOOST_TEST((client(Request{}, std::chrono::milliseconds{ 1 }).get() == Response{}));
    BOOST_TEST(client.CheckClientUsage());
    BOOST_TEST(server->CheckServerUsage());
}

BOOST_AUTO_TEST_CASE(BufferPoolTest)
{
    auto memory = std::make_shared<SharedMemory>(create_only, GenerateRandomString().c_str(), 1024 * 1024);
    auto inPool = std::make_shared<DefaultBufferPool>(memory);
    auto outPool = std::make_shared<DefaultBufferPool>(memory);

    auto name = GenerateRandomString();

    std::unique_ptr<Server> server;

    Acceptor acceptor{
        name.c_str(),
        [&](auto&& futureConnection)
        {
            server = std::make_unique<Server>(
                detail::BufferPoolHolder<DefaultBufferPool>{ inPool, outPool },
                SerializerMock{},
                futureConnection.get(),
                [](auto&&...) {},
                [] {});
        } };

    Client client{
        detail::BufferPoolHolder<DefaultBufferPool>{ inPool, outPool },
        SerializerMock{},
        Connector{}.Connect(name.c_str()).get(),
        [] {} };

    BOOST_TEST(client.GetInputPool() == inPool);
    BOOST_TEST(client.GetOutputPool() == outPool);
    BOOST_TEST(server->GetInputPool() == inPool);
    BOOST_TEST(server->GetOutputPool() == outPool);
}

BOOST_AUTO_TEST_SUITE_END()
