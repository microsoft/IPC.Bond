#include "stdafx.h"
#include "IPC/Bond/InputBuffer.h"
#include "IPC/detail/RandomString.h"
#include <tuple>
#include <type_traits>

using namespace IPC::Bond;
using IPC::detail::GenerateRandomString;
using IPC::SharedMemory;
using IPC::create_only;


BOOST_AUTO_TEST_SUITE(InputBufferTests)

static_assert(std::is_default_constructible<DefaultInputBuffer>::value, "InputBuffer should be default constructible.");
static_assert(std::is_copy_constructible<DefaultInputBuffer>::value, "InputBuffer should be copy constructible.");
static_assert(std::is_copy_assignable<DefaultInputBuffer>::value, "InputBuffer should be copy assignable.");

template <typename Function, std::size_t... I>
void ForEach(Function&& func, std::index_sequence<I...>)
{
    auto dummy = { (func(std::integral_constant<std::size_t, I>{}), 0)... };
    (void)dummy;
}

BOOST_AUTO_TEST_CASE(ReadTrivialTypesTest)
{
    auto pool = std::make_shared<DefaultBufferPool>(std::make_shared<SharedMemory>(create_only, GenerateRandomString().c_str(), 1024 * 1024));
    auto buffer = pool->TakeBuffer();

    auto values = std::make_tuple(
        static_cast<std::int8_t>(1),
        static_cast<std::int16_t>(2),
        static_cast<std::int32_t>(3),
        static_cast<std::int64_t>(4),
        static_cast<std::uint8_t>(5),
        static_cast<std::uint16_t>(6),
        static_cast<std::uint32_t>(7),
        static_cast<std::uint64_t>(8),
        static_cast<float>(9.10),
        static_cast<double>(11.12));

    ForEach(
        [&](auto index)
        {
            auto& value = std::get<index.value>(values);
            (void)index;

            auto blob = pool->TakeBlob();
            blob->resize(sizeof(decltype(value)), boost::container::default_init);
            std::memcpy(blob->data(), &value, sizeof(value));

            buffer->push_back(std::move(blob));
        },
        std::make_index_sequence<std::tuple_size<decltype(values)>::value>{});

    DefaultInputBuffer input{ std::move(buffer), pool->GetMemory() };
    decltype(values) results;

    BOOST_TEST(!input.IsEof());

    ForEach(
        [&](auto index)
        {
            input.Read(std::get<index.value>(results));
            (void)index;
        },
        std::make_index_sequence<std::tuple_size<decltype(values)>::value>{});

    BOOST_TEST(input.IsEof());
    BOOST_TEST((values == results));
}

BOOST_AUTO_TEST_CASE(ReadRawBytesTest)
{
    auto pool = std::make_shared<DefaultBufferPool>(std::make_shared<SharedMemory>(create_only, GenerateRandomString().c_str(), 1024 * 1024));

    auto buffer = pool->TakeBuffer();
    {
        auto blob = pool->TakeBlob();
        blob->resize(10, 1);
        buffer->push_back(std::move(blob));
    }
    {
        auto blob = pool->TakeBlob();
        blob->resize(20, 2);
        buffer->push_back(std::move(blob));
    }
    {
        auto blob = pool->TakeBlob();
        blob->resize(30, 3);
        buffer->push_back(std::move(blob));
    }

    DefaultInputBuffer input{ std::move(buffer), pool->GetMemory() };
    char result[10 + 20 + 30];
    auto ptr = result;

    input.Read(ptr, 5);
    input.Read(ptr += 5, 10);
    input.Read(ptr += 10, 5);
    BOOST_CHECK_THROW(input.Read(ptr += 5, 41), std::exception);
    input.Read(ptr, 15);
    input.Read(ptr += 15, 5);
    input.Read(ptr += 5, 20);

    BOOST_TEST(input.IsEof());
    BOOST_TEST(std::all_of(result, result + 10, [](const char& c) { return c == 1; }));
    BOOST_TEST(std::all_of(result + 10, result + 10 + 20, [](const char& c) { return c == 2; }));
    BOOST_TEST(std::all_of(result + 10 + 20, result + 10 + 20 + 30, [](const char& c) { return c == 3; }));
}

BOOST_AUTO_TEST_CASE(ReadBondBlobTest)
{
    auto pool = std::make_shared<DefaultBufferPool>(std::make_shared<SharedMemory>(create_only, GenerateRandomString().c_str(), 1024 * 1024));

    auto buffer = pool->TakeBuffer();
    {
        auto blob = pool->TakeBlob();
        blob->resize(10, 1);
        buffer->push_back(std::move(blob));
    }
    {
        auto blob = pool->TakeBlob();
        blob->resize(10, 2);
        buffer->push_back(std::move(blob));
    }

    DefaultBufferPool::ConstBuffer constBuffer{ std::move(buffer) };

    DefaultInputBuffer input{ constBuffer, pool->GetMemory() };

    bond::blob b1, b2, b3;

    BOOST_CHECK_THROW(input.Read(b1, 20), std::exception);

    input.Read(b1, 5);
    input.Read(b2, 5);
    input.Read(b3, 10);

    BOOST_TEST(input.IsEof());
    BOOST_TEST(b1.size() == 5U);
    BOOST_TEST(std::all_of(b1.begin(), b1.end(), [](const char& c) { return c == 1; }));
    BOOST_TEST(b2.size() == 5U);
    BOOST_TEST(std::all_of(b2.begin(), b2.end(), [](const char& c) { return c == 1; }));
    BOOST_TEST(b3.size() == 10U);
    BOOST_TEST(std::all_of(b3.begin(), b3.end(), [](const char& c) { return c == 2; }));

    BOOST_TEST(BlobCast<DefaultBufferPool::ConstBlob>(b1).data() == constBuffer.begin()->data());
    BOOST_TEST(BlobCast<DefaultBufferPool::ConstBlob>(b2).data() == constBuffer.begin()->data() + 5);
    BOOST_TEST(BlobCast<DefaultBufferPool::ConstBlob>(b3).data() == std::next(constBuffer.begin())->data());
}

BOOST_AUTO_TEST_CASE(SkipTest)
{
    auto pool = std::make_shared<DefaultBufferPool>(std::make_shared<SharedMemory>(create_only, GenerateRandomString().c_str(), 1024 * 1024));

    auto buffer = pool->TakeBuffer();
    {
        auto blob = pool->TakeBlob();
        blob->resize(10, 1);
        buffer->push_back(std::move(blob));
    }
    {
        auto blob = pool->TakeBlob();
        blob->resize(10, 2);
        buffer->push_back(std::move(blob));
    }

    DefaultInputBuffer input{ std::move(buffer), pool->GetMemory() };
    char result[10];

    input.Read(result, 5);
    BOOST_CHECK_THROW(input.Skip(16), std::exception);
    input.Skip(10);
    input.Read(result + 5, 5);

    BOOST_TEST(input.IsEof());
    BOOST_TEST(std::all_of(result, result + 5, [](const char& c) { return c == 1; }));
    BOOST_TEST(std::all_of(result + 5, result + 10, [](const char& c) { return c == 2; }));

    BOOST_CHECK_NO_THROW(input.Skip(0));
    BOOST_CHECK_THROW(input.Skip(1), std::exception);
}

BOOST_AUTO_TEST_CASE(ReadEmptyDataTest)
{
    auto pool = std::make_shared<DefaultBufferPool>(std::make_shared<SharedMemory>(create_only, GenerateRandomString().c_str(), 1024 * 1024));

    auto buffer = pool->TakeBuffer();
    buffer->push_back(DefaultBufferPool::ConstBlob{});
    buffer->push_back(pool->TakeBlob());

    DefaultBufferPool::ConstBuffer constBuffer{ std::move(buffer) };

    BOOST_TEST((DefaultInputBuffer{ DefaultBufferPool::ConstBuffer{}, nullptr }.IsEof()));

    BOOST_TEST((DefaultInputBuffer{ constBuffer, pool->GetMemory() }.IsEof()));

    BOOST_TEST((DefaultInputBuffer{ DefaultBufferPool::ConstBuffer::Range{ constBuffer, constBuffer.begin(), 0, constBuffer.end(), 0 }, pool->GetMemory() }.IsEof()));

    BOOST_TEST((DefaultInputBuffer{ DefaultBufferPool::ConstBuffer::Range{}, nullptr }.IsEof()));
}

BOOST_AUTO_TEST_CASE(ReadFromRangeTest)
{
    auto pool = std::make_shared<DefaultBufferPool>(std::make_shared<SharedMemory>(create_only, GenerateRandomString().c_str(), 1024 * 1024));

    auto buffer = pool->TakeBuffer();
    {
        auto blob = pool->TakeBlob();
        blob->resize(10, 1);
        buffer->push_back(std::move(blob));
    }
    {
        auto blob = pool->TakeBlob();
        blob->resize(20, 2);
        buffer->push_back(std::move(blob));
    }
    {
        auto blob = pool->TakeBlob();
        blob->resize(30, 3);
        buffer->push_back(std::move(blob));
    }

    DefaultBufferPool::ConstBuffer constBuffer{ std::move(buffer) };

    {
        DefaultInputBuffer input{ DefaultBufferPool::ConstBuffer::Range{ constBuffer, constBuffer.begin(), 0, constBuffer.end(), 0 }, pool->GetMemory() };
        char result[10 + 20 + 30];

        BOOST_TEST(!input.IsEof());
        input.Read(result, sizeof(result));
        BOOST_TEST(input.IsEof());
        BOOST_TEST(std::all_of(result, result + 10, [](const char& c) { return c == 1; }));
        BOOST_TEST(std::all_of(result + 10, result + 10 + 20, [](const char& c) { return c == 2; }));
        BOOST_TEST(std::all_of(result + 10 + 20, result + 10 + 20 + 30, [](const char& c) { return c == 3; }));
    }
    {
        DefaultInputBuffer input{ DefaultBufferPool::ConstBuffer::Range{ constBuffer, constBuffer.begin(), 3, constBuffer.begin(), 8 }, pool->GetMemory() };
        char result[5];

        BOOST_TEST(!input.IsEof());
        input.Read(result, sizeof(result));
        BOOST_TEST(input.IsEof());
        BOOST_TEST(std::all_of(result, result + 5, [](const char& c) { return c == 1; }));
    }
    {
        DefaultInputBuffer input{ DefaultBufferPool::ConstBuffer::Range{ constBuffer, constBuffer.begin(), 5, std::next(constBuffer.begin(), 2), 15 }, pool->GetMemory() };
        char result[5 + 20 + 15];

        BOOST_TEST(!input.IsEof());
        input.Read(result, sizeof(result));
        BOOST_TEST(input.IsEof());
        BOOST_TEST(std::all_of(result, result + 5, [](const char& c) { return c == 1; }));
        BOOST_TEST(std::all_of(result + 5, result + 5 + 20, [](const char& c) { return c == 2; }));
        BOOST_TEST(std::all_of(result + 5 + 20, result + 5 + 20 + 15, [](const char& c) { return c == 3; }));
    }
}

BOOST_AUTO_TEST_CASE(IsEofTest)
{
    auto pool = std::make_shared<DefaultBufferPool>(std::make_shared<SharedMemory>(create_only, GenerateRandomString().c_str(), 1024 * 1024));

    auto buffer = pool->TakeBuffer();
    auto blob = pool->TakeBlob();
    blob->resize(10);
    buffer->push_back(std::move(blob));

    DefaultInputBuffer input{ std::move(buffer), pool->GetMemory() };

    BOOST_TEST(DefaultInputBuffer{}.IsEof());

    BOOST_TEST(!input.IsEof());
    input.Skip(5);
    BOOST_TEST(!input.IsEof());
    BOOST_CHECK_THROW(input.Skip(10), std::exception);
    BOOST_TEST(!input.IsEof());
    input.Skip(5);
    BOOST_TEST(input.IsEof());
}

BOOST_AUTO_TEST_CASE(EqualityTest)
{
    BOOST_TEST((DefaultInputBuffer{ DefaultBufferPool::ConstBuffer{}, nullptr } == DefaultInputBuffer{ DefaultBufferPool::ConstBuffer{}, nullptr }));

    auto pool = std::make_shared<DefaultBufferPool>(std::make_shared<SharedMemory>(create_only, GenerateRandomString().c_str(), 1024 * 1024));

    auto buffer1 = pool->TakeBuffer();
    {
        auto blob = pool->TakeBlob();
        blob->resize(10);
        buffer1->push_back(std::move(blob));
    }
    auto buffer2 = pool->TakeBuffer();
    {
        auto blob = pool->TakeBlob();
        blob->resize(10);
        buffer2->push_back(std::move(blob));
    }

    assert(buffer1->size() == 1);
    assert(buffer2->size() == 1);
    assert(std::equal(buffer1->front().begin(), buffer1->front().end(), buffer2->front().begin(), buffer2->front().end()));

    DefaultInputBuffer input1{ std::move(buffer1), pool->GetMemory() };
    auto input1Copy = input1;
    DefaultInputBuffer input2{ std::move(buffer2), pool->GetMemory() };
    auto input2Copy = input2;

    BOOST_TEST(!(input1 == input2));
    BOOST_TEST(!(input1Copy == input2Copy));
    BOOST_TEST((input1 == input1Copy));
    BOOST_TEST((input2 == input2Copy));
}

BOOST_AUTO_TEST_CASE(GetCurrentBufferTest)
{
    auto pool = std::make_shared<DefaultBufferPool>(std::make_shared<SharedMemory>(create_only, GenerateRandomString().c_str(), 1024 * 1024));

    auto buffer = pool->TakeBuffer();
    auto blob = pool->TakeBlob();
    blob->resize(10);
    buffer->push_back(std::move(blob));

    DefaultInputBuffer input{ std::move(buffer), pool->GetMemory() };

    BOOST_TEST((GetCurrentBuffer(input) == input));
}

BOOST_AUTO_TEST_CASE(GetBufferRangeTest)
{
    BOOST_TEST(GetBufferRange(
        DefaultInputBuffer{ DefaultBufferPool::ConstBuffer{}, nullptr },
        DefaultInputBuffer{ DefaultBufferPool::ConstBuffer{}, nullptr }).IsEmpty());

    auto pool = std::make_shared<DefaultBufferPool>(std::make_shared<SharedMemory>(create_only, GenerateRandomString().c_str(), 1024 * 1024));

    auto buffer = pool->TakeBuffer();
    {
        auto blob = pool->TakeBlob();
        blob->resize(10, 1);
        buffer->push_back(std::move(blob));
    }
    {
        auto blob = pool->TakeBlob();
        blob->resize(20, 2);
        buffer->push_back(std::move(blob));
    }

    DefaultBufferPool::ConstBuffer constBuffer{ std::move(buffer) };

    DefaultInputBuffer input1{ constBuffer, pool->GetMemory() };
    DefaultInputBuffer input2{ constBuffer, pool->GetMemory() };

    input1.Skip(5);
    input2.Skip(20);
    {
        DefaultInputBuffer input3{ GetBufferRange(input1, input2), pool->GetMemory() };
        char result[15];

        BOOST_TEST(!input3.IsEof());
        input3.Read(result, sizeof(result));
        BOOST_TEST(input3.IsEof());
        BOOST_TEST(std::all_of(result, result + 5, [](const char& c) { return c == 1; }));
        BOOST_TEST(std::all_of(result + 5, result + 15, [](const char& c) { return c == 2; }));
    }
    input1.Skip(10);
    input2.Skip(5);
    {
        DefaultInputBuffer input3{ GetBufferRange(input1, input2), pool->GetMemory() };
        char result[10];

        BOOST_TEST(!input3.IsEof());
        input3.Read(result, sizeof(result));
        BOOST_TEST(input3.IsEof());
        BOOST_TEST(std::all_of(result, result + 10, [](const char& c) { return c == 2; }));
    }
    input2.Skip(5);
    assert(input2.IsEof());
    {
        DefaultInputBuffer input3{ GetBufferRange(input1, input2), pool->GetMemory() };
        char result[15];

        BOOST_TEST(!input3.IsEof());
        input3.Read(result, sizeof(result));
        BOOST_TEST(input3.IsEof());
        BOOST_TEST(std::all_of(result, result + 15, [](const char& c) { return c == 2; }));
    }
    input1.Skip(15);
    assert(input1.IsEof());
    {
        DefaultInputBuffer input3{ GetBufferRange(input1, input2), pool->GetMemory() };
        BOOST_TEST(input3.IsEof());
        BOOST_CHECK_THROW(input3.Skip(1), std::exception);
    }
}

BOOST_AUTO_TEST_CASE(CreateInputBufferTest)
{
    auto pool = std::make_shared<DefaultBufferPool>(std::make_shared<SharedMemory>(create_only, GenerateRandomString().c_str(), 1024 * 1024));

    auto buffer = pool->TakeBuffer();
    auto blob = pool->TakeBlob();
    blob->resize(10, 1);
    buffer->push_back(std::move(blob));

    DefaultBufferPool::ConstBuffer constBuffer{ std::move(buffer) };

    auto input = CreateInputBuffer(
        DefaultInputBuffer{ constBuffer, pool->GetMemory() },
        DefaultBufferPool::ConstBuffer::Range{ constBuffer, constBuffer.begin(), 3, constBuffer.begin(), 8 });

    char result[5];

    BOOST_TEST(!input.IsEof());
    input.Read(result, sizeof(result));
    BOOST_TEST(input.IsEof());
    BOOST_TEST(std::all_of(result, result + 5, [](const char& c) { return c == 1; }));
}

BOOST_AUTO_TEST_CASE(MemoryOwnershipTest)
{
    auto memory = std::make_shared<SharedMemory>(create_only, GenerateRandomString().c_str(), 1024 * 1024);

    bond::blob bondBlob;
    const char* ptr;

    BOOST_TEST(memory.unique());
    {
        DefaultBufferPool::ConstBuffer constBuffer;
        {
            auto pool = std::make_shared<DefaultBufferPool>(memory);

            auto buffer = pool->TakeBuffer();
            auto blob = pool->TakeBlob();
            blob->resize(10, 1);
            buffer->push_back(std::move(blob));

            constBuffer = std::move(buffer);
        }

        DefaultInputBuffer input{ constBuffer, memory };
        BOOST_TEST(memory.use_count() == 2);

        input.Read(bondBlob, 10);
        BOOST_TEST(memory.use_count() == 3);

        ptr = constBuffer.begin()->data();
    }

    BOOST_TEST(bondBlob.data() == ptr);
    BOOST_TEST(memory.use_count() == 2);

    memory.reset();

    BOOST_TEST(std::all_of(ptr, ptr + 10, [](const char& c) { return c == 1; }));
}

BOOST_AUTO_TEST_SUITE_END()
