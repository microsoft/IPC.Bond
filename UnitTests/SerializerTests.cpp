#include "stdafx.h"
#include "IPC/Bond/Serializer.h"
#include "IPC/detail/RandomString.h"
#include <bond/core/bond.h>
#include <bond/core/bond_types.h>
#include <bond/core/tuple.h>
#include <bond/protocol/simple_binary.h>
#include <bond/protocol/compact_binary.h>
#include <bond/protocol/fast_binary.h>
#include <bond/protocol/simple_json_reader.h>
#include <bond/protocol/simple_json_writer.h>
#include <string>
#include <vector>
#include <map>


using ValueStruct = std::tuple<
    std::int8_t,
    std::int16_t,
    std::int32_t,
    std::int64_t,
    std::uint8_t,
    std::uint16_t,
    std::uint32_t,
    std::uint64_t,
    bond::blob,
    bond::ProtocolType,
    bond::blob,
    std::string>;

using Struct = std::tuple<
    ValueStruct,
    std::vector<ValueStruct>,
    std::map<bond::ProtocolType, ValueStruct>>;

using BondedStruct = bond::Box<bond::bonded<Struct>>;

namespace bond
{
    // TODO: Fix bond to properly work with std::tuple.

    inline ValueStruct make_element(std::vector<ValueStruct>& container)
    {
        return{};
    }

    inline ValueStruct make_value(std::map<ProtocolType, ValueStruct>& map)
    {
        return{};
    }

} // namespace bond


using namespace IPC::Bond;
using IPC::detail::GenerateRandomString;
using IPC::SharedMemory;
using IPC::create_only;


BOOST_AUTO_TEST_SUITE(SerializerTests)

static_assert(std::is_copy_constructible<DefaultSerializer>::value, "Serializer should be copy constructible.");
static_assert(std::is_copy_assignable<DefaultSerializer>::value, "Serializer should be copy assignable.");


template <typename Function, typename Protocol, typename MakeBondedFunc>
void RunProtocolTest(Function&& func, Protocol protocol, MakeBondedFunc&& makeBondedFunc)
{
    auto pool = std::make_shared<DefaultBufferPool>(std::make_shared<SharedMemory>(create_only, GenerateRandomString().c_str(), 1024 * 1024));

    auto obj = MakeStruct(*pool);
    BondedStruct bondedObj;
    bondedObj.value = makeBondedFunc(pool, obj, protocol);

    BondedStruct bondedResult;

    std::forward<Function>(func)(pool, bondedObj, bondedResult, protocol);

    Struct result;
    bondedResult.value.Deserialize<DefaultProtocols>(result);

    BOOST_TEST((obj == result));
}

template <typename Function, typename Protocol>
void RunProtocolTest(Function&& func, Protocol protocol, std::nullptr_t)
{
    RunProtocolTest(
        std::forward<Function>(func),
        protocol,
        [](auto&& /*pool*/, const Struct& obj, auto /*protocol*/)
        {
            return bond::bonded<Struct>{ obj };
        });
}

template <typename Function>
void ForEachRuntimeProtocol(Function&& func, std::initializer_list<bond::ProtocolType> protocols)
{
    for (const auto& protocol : protocols)
    {
        RunProtocolTest(func, protocol, nullptr);
    }
}

template <typename Function>
void ForEachRuntimeProtocol(Function&& func)
{
    ForEachRuntimeProtocol(
        std::forward<Function>(func),
        {
            bond::ProtocolType::COMPACT_PROTOCOL,
            bond::ProtocolType::FAST_PROTOCOL,
            bond::ProtocolType::SIMPLE_PROTOCOL,
            bond::ProtocolType::SIMPLE_JSON_PROTOCOL
        });
}

template <template <typename...> typename R>
struct ProtocolInfo
{
    template <typename T>
    using Reader = R<T>;

    template <typename T>
    using Writer = typename bond::get_protocol_writer<R<T>, T>::type;
};

template <template <typename...> typename... Readers, typename Function>
void ForEachReader(Function&& func)
{
    std::initializer_list<int>{ ((void)func(ProtocolInfo<Readers>{}), 0)... };
}

template <template <typename...> typename Reader, template <typename...> typename... Readers, typename Function, typename MakeBondedFunc = std::nullptr_t>
void ForEachCompiletimeProtocol(Function&& func, MakeBondedFunc&& makeBondedFunc = nullptr)
{
    ForEachReader<Reader, Readers...>(
        [func = std::forward<Function>(func), makeBondedFunc = std::forward<MakeBondedFunc>(makeBondedFunc)](auto protocol)
        {
            RunProtocolTest(func, protocol, makeBondedFunc);
        });
}

template <typename Function, typename MakeBondedFunc = std::nullptr_t>
void ForEachCompiletimeProtocol(Function&& func, MakeBondedFunc&& makeBondedFunc = nullptr)
{
    ForEachCompiletimeProtocol<
        bond::CompactBinaryReader,
        bond::SimpleBinaryReader,
        bond::FastBinaryReader,
        bond::SimpleJsonReader>(std::forward<Function>(func), std::forward<MakeBondedFunc>(makeBondedFunc));
}

Struct MakeStruct(DefaultBufferPool& pool)
{
    auto data = "Random data for blob.";

    auto blob = pool.TakeBlob();
    blob->resize(std::strlen(data) + 1, boost::container::default_init);
    std::strcpy(blob->data(), data);

    bond::blob bondBlob = BlobCast(DefaultBufferPool::ConstBlob{ std::move(blob) }, pool.GetMemory());

    ValueStruct value{
        1, 2, 3, 4, 5, 6, 7, 8,
        bondBlob,
        bond::ProtocolType::COMPACT_PROTOCOL,
        { bondBlob.data(), bondBlob.size() },
        data };

    return Struct(
        value,
        { 10, value },
        {
            { bond::ProtocolType::COMPACT_PROTOCOL, value },
            { bond::ProtocolType::SIMPLE_PROTOCOL, value },
            { bond::ProtocolType::SIMPLE_JSON_PROTOCOL, value }
        });
}

BOOST_AUTO_TEST_CASE(SerializationUsingRuntimeProtocolTest)
{
    ForEachRuntimeProtocol(
        [&](auto&& pool, BondedStruct& bondedObj, BondedStruct& bondedResult, bond::ProtocolType protocol)
        {
            Deserialize(protocol, Serialize(protocol, pool, bondedObj), bondedResult, pool->GetMemory());
        });

    ForEachRuntimeProtocol(
        [&](auto&& pool, BondedStruct& bondedObj, BondedStruct& bondedResult, bond::ProtocolType protocol)
        {
            DefaultSerializer serializer{ protocol, false, pool, pool->GetMemory() };
            BOOST_TEST(!serializer.IsMarshaled());
            BOOST_TEST(serializer.GetProtocolType() == protocol);

            serializer.Deserialize(serializer.Serialize(bondedObj), bondedResult);
        });
}

BOOST_AUTO_TEST_CASE(SerializationUsingCompiletimeProtocolTest)
{
    ForEachCompiletimeProtocol(
        [&](auto&& pool, BondedStruct& bondedObj, BondedStruct& bondedResult, auto protocol)
        {
            using Protocol = decltype(protocol);
            (void)protocol;

            Deserialize<typename Protocol::template Reader>(
                Serialize<typename Protocol::template Writer>(pool, bondedObj), bondedResult, pool->GetMemory());
        });
}

BOOST_AUTO_TEST_CASE(MarshalingUsingRuntimeProtocolTest)
{
    auto protocols =
    {
        bond::ProtocolType::COMPACT_PROTOCOL,
        bond::ProtocolType::FAST_PROTOCOL,
        bond::ProtocolType::SIMPLE_PROTOCOL
    };

    ForEachRuntimeProtocol(
        [&](auto&& pool, BondedStruct& bondedObj, BondedStruct& bondedResult, bond::ProtocolType protocol)
        {
            Unmarshal(Marshal(protocol, pool, bondedObj), bondedResult, pool->GetMemory());
        },
        protocols);

    ForEachRuntimeProtocol(
        [&](auto&& pool, BondedStruct& bondedObj, BondedStruct& bondedResult, bond::ProtocolType protocol)
        {
            DefaultSerializer serializer{ protocol, true, pool, pool->GetMemory() };
            BOOST_TEST(serializer.IsMarshaled());
            BOOST_TEST(serializer.GetProtocolType() == protocol);

            serializer.Deserialize(serializer.Serialize(bondedObj), bondedResult);
        },
        protocols);
}

BOOST_AUTO_TEST_CASE(MarshalingUsingCompiletimeProtocolTest)
{
    ForEachCompiletimeProtocol<
        bond::CompactBinaryReader,
        bond::SimpleBinaryReader,
        bond::FastBinaryReader>(
        [&](auto&& pool, BondedStruct& bondedObj, BondedStruct& bondedResult, auto protocol)
        {
            using Protocol = decltype(protocol);
            (void)protocol;

            Unmarshal(Marshal<typename Protocol::template Writer>(pool, bondedObj), bondedResult, pool->GetMemory());
        });
}

BOOST_AUTO_TEST_CASE(TranscodingTest)
{
    ForEachCompiletimeProtocol<
        bond::CompactBinaryReader,
        bond::SimpleBinaryReader,
        bond::FastBinaryReader>(
        [&](auto&& pool, BondedStruct& bondedObj, BondedStruct& bondedResult, auto protocol)
        {
            using Protocol = decltype(protocol);
            (void)protocol;

            Deserialize<typename Protocol::template Reader>(
                Serialize<typename Protocol::template Writer>(pool, bondedObj), bondedResult, pool->GetMemory());
        },
        [](auto&& pool, const Struct& obj, auto protocol)
        {
            using Protocol = decltype(protocol);
            (void)protocol;

            return bond::bonded<Struct>{
                typename Protocol::template Reader<DefaultInputBuffer>{
                    DefaultInputBuffer{ Serialize<typename Protocol::template Writer>(pool, obj), pool->GetMemory() } } };
        });
}

BOOST_AUTO_TEST_CASE(FailedDeserializationTest)
{
    auto pool = std::make_shared<DefaultBufferPool>(std::make_shared<SharedMemory>(create_only, GenerateRandomString().c_str(), 1024 * 1024));

    auto obj = MakeStruct(*pool);

    DefaultSerializer serializer{ bond::ProtocolType::COMPACT_PROTOCOL, false, pool, pool->GetMemory() };

    auto buffer = serializer.Serialize(obj);

    BOOST_TEST((serializer.Deserialize<Struct>(buffer).get() == obj));

    DefaultSerializer wrongSerializer{ serializer.GetProtocolType(), !serializer.IsMarshaled(), pool, pool->GetMemory() };

    BOOST_CHECK_THROW(wrongSerializer.Deserialize<Struct>(buffer).get(), std::exception);
}

BOOST_AUTO_TEST_CASE(MemoryOwnershipTest)
{
    auto memory = std::make_shared<SharedMemory>(create_only, GenerateRandomString().c_str(), 1024 * 1024);

    auto data = "Blob content";
    bond::Box<bond::blob> obj;
    obj.value = { data, static_cast<std::uint32_t>(std::strlen(data)) + 1 };

    BOOST_TEST(memory.unique());

    decltype(obj) serializedObj, marshaledObj;
    {
        auto pool = std::make_shared<DefaultBufferPool>(memory);
        {
            DefaultSerializer serializer{ bond::ProtocolType::COMPACT_PROTOCOL, false, pool, memory };
            BOOST_TEST(serializer.GetOutputBufferPool() == pool);
            BOOST_TEST(serializer.GetInputMemory() == memory);
            serializer.Deserialize(serializer.Serialize(obj), serializedObj);
        }
        {
            DefaultSerializer serializer{ bond::ProtocolType::COMPACT_PROTOCOL, true, pool, memory };
            BOOST_TEST(serializer.GetOutputBufferPool() == pool);
            BOOST_TEST(serializer.GetInputMemory() == memory);
            serializer.Deserialize(serializer.Serialize(obj), marshaledObj);
        }
    }

    BOOST_TEST(memory.use_count() == 3);

    BOOST_TEST((obj == serializedObj));
    BOOST_TEST(memory->Contains(serializedObj.value.content()));
    BOOST_TEST(memory->Contains(serializedObj.value.content() + serializedObj.value.size() - 1));
    serializedObj.value = {};
    BOOST_TEST(memory.use_count() == 2);

    BOOST_TEST((obj == marshaledObj));
    BOOST_TEST(memory->Contains(marshaledObj.value.content()));
    BOOST_TEST(memory->Contains(marshaledObj.value.content() + marshaledObj.value.size() - 1));
    marshaledObj.value = {};
    BOOST_TEST(memory.unique());
}

BOOST_AUTO_TEST_SUITE_END()
