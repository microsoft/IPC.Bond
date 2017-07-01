#include "stdafx.h"
#include "IPC/Bond/OutputBuffer.h"
#include "IPC/detail/RandomString.h"
#include <tuple>
#include <type_traits>

using namespace IPC::Bond;
using IPC::detail::GenerateRandomString;
using IPC::SharedMemory;
using IPC::create_only;


BOOST_AUTO_TEST_SUITE(OutputBufferTests)

static_assert(!std::is_copy_constructible<DefaultOutputBuffer>::value, "OutputBuffer should not be copy constructible.");
static_assert(!std::is_copy_assignable<DefaultOutputBuffer>::value, "OutputBuffer should not be copy assignable.");
static_assert(std::is_move_constructible<DefaultOutputBuffer>::value, "OutputBuffer should be move constructible.");
static_assert(std::is_move_assignable<DefaultOutputBuffer>::value, "OutputBuffer should be move assignable.");

template <typename Function, std::size_t... I>
void ForEach(Function&& func, std::index_sequence<I...>)
{
    auto dummy = { (func(std::integral_constant<std::size_t, I>{}), 0)... };
    (void)dummy;
}

BOOST_AUTO_TEST_CASE(WriteArithmeticTypesTest)
{
    auto pool = std::make_shared<DefaultBufferPool>(std::make_shared<SharedMemory>(create_only, GenerateRandomString().c_str(), 1024 * 1024));
    DefaultOutputBuffer output{ pool, 3 };

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

    std::size_t size = 0;

    ForEach(
        [&](auto index)
        {
            const auto& value = std::get<index.value>(values);
            (void)index;

            output.Write(value);
            size += sizeof(decltype(value));
        },
        std::make_index_sequence<std::tuple_size<decltype(values)>::value>{});

    auto buffer = std::move(output).GetBuffer();

    BOOST_TEST(buffer.size() == size);
    BOOST_TEST(std::distance(buffer.begin(), buffer.end()) == 1);

    auto ptr = buffer.begin()->data();

    ForEach(
        [&](auto index)
        {
            const auto& value = std::get<index.value>(values);
            (void)index;

            BOOST_TEST(std::memcmp(&value, ptr, sizeof(value)) == 0);
            ptr += sizeof(decltype(value));
        },
        std::make_index_sequence<std::tuple_size<decltype(values)>::value>{});
}

BOOST_AUTO_TEST_CASE(WriteRawBytesTest)
{
    auto pool = std::make_shared<DefaultBufferPool>(std::make_shared<SharedMemory>(create_only, GenerateRandomString().c_str(), 1024 * 1024));
    DefaultOutputBuffer output{ pool, 2 };

    const char data[] = "Data";

    output.Write(data, sizeof(data));
    output.Write(data, sizeof(data));

    auto buffer = std::move(output).GetBuffer();

    BOOST_TEST(buffer.size() == 2 * sizeof(data));
    BOOST_TEST(std::distance(buffer.begin(), buffer.end()) == 1);

    auto ptr = buffer.begin()->data();

    BOOST_TEST(std::memcmp(ptr, data, sizeof(data)) == 0);
    BOOST_TEST(std::memcmp(ptr + sizeof(data), data, sizeof(data)) == 0);
}

BOOST_AUTO_TEST_CASE(WriteBufferTest)
{
    auto pool = std::make_shared<DefaultBufferPool>(std::make_shared<SharedMemory>(create_only, GenerateRandomString().c_str(), 1024 * 1024));
    DefaultOutputBuffer output{ pool, 3 };

    const char data[] = "Data";

    output.Write(data, sizeof(data));

    DefaultBufferPool::ConstBlob b1;
    {
        auto blob = pool->TakeBlob();
        blob->resize(10);
        b1 = std::move(blob);
    }

    DefaultBufferPool::ConstBlob b2;
    {
        auto blob = pool->TakeBlob();
        blob->resize(20);
        b2 = std::move(blob);
    }

    {
        auto buffer = pool->TakeBuffer();
        buffer->push_back(b1);
        buffer->push_back(DefaultBufferPool::ConstBlob{ pool->TakeBlob() });
        buffer->push_back(b2);
        buffer->push_back(DefaultBufferPool::ConstBlob{});

        output.Write(DefaultBufferPool::ConstBuffer{ std::move(buffer) });
    }

    auto otherPool = std::make_shared<DefaultBufferPool>(std::make_shared<SharedMemory>(create_only, GenerateRandomString().c_str(), 1024 * 1024));
    {
        auto buffer = otherPool->TakeBuffer();
        auto blob = otherPool->TakeBlob();
        blob->resize(sizeof(data), boost::container::default_init);
        std::strcpy(blob->data(), data);
        buffer->push_back(std::move(blob));

        output.Write(DefaultBufferPool::ConstBuffer{ std::move(buffer) });
    }

    output.Write(data, sizeof(data));

    auto buffer = std::move(output).GetBuffer();

    BOOST_TEST(buffer.size() == sizeof(data) + 10 + 20 + 2 * sizeof(data));
    BOOST_TEST(std::distance(buffer.begin(), buffer.end()) == 4);

    auto blob = buffer.begin();
    BOOST_TEST(blob->size() == sizeof(data));
    BOOST_TEST(std::memcmp(blob->data(), data, sizeof(data)) == 0);

    ++blob;
    BOOST_TEST(blob->size() == b1.size());
    BOOST_TEST(blob->data() == b1.data());

    ++blob;
    BOOST_TEST(blob->size() == b2.size());
    BOOST_TEST(blob->data() == b2.data());

    ++blob;
    BOOST_TEST(blob->size() == 2 * sizeof(data));
    BOOST_TEST(std::memcmp(blob->data(), data, sizeof(data)) == 0);
    BOOST_TEST(std::memcmp(blob->data() + sizeof(data), data, sizeof(data)) == 0);

    BOOST_TEST((++blob == buffer.end()));
}

BOOST_AUTO_TEST_CASE(WriteBufferRangeTest)
{
    auto pool = std::make_shared<DefaultBufferPool>(std::make_shared<SharedMemory>(create_only, GenerateRandomString().c_str(), 1024 * 1024));

    DefaultBufferPool::ConstBlob constBlob;
    DefaultBufferPool::ConstBuffer constBuffer;

    const char data[] = "Data!";
    {
        auto blob = pool->TakeBlob();
        blob->resize(sizeof(data), boost::container::default_init);
        std::strcpy(blob->data(), data);

        constBlob = std::move(blob);

        auto buffer = pool->TakeBuffer();
        buffer->push_back(constBlob);
        buffer->push_back(constBlob);
        buffer->push_back(constBlob);

        constBuffer = std::move(buffer);
    }

    constexpr std::size_t leftOffset = 2, rightOffset = 4;

    {
        DefaultOutputBuffer output{ pool, 4 };
        output.Write(DefaultBufferPool::ConstBuffer::Range{ constBuffer, constBuffer.begin(), leftOffset, std::next(constBuffer.begin()), 0 });

        auto buffer = std::move(output).GetBuffer();

        BOOST_TEST(buffer.size() == constBlob.size() - leftOffset);
        BOOST_TEST(std::distance(buffer.begin(), buffer.end()) == 1);

        auto blob = buffer.begin();
        BOOST_TEST(blob->size() == constBlob.size() - leftOffset);
        BOOST_TEST(blob->data() == constBlob.data() + leftOffset);

        BOOST_TEST((++blob == buffer.end()));
    }
    {
        DefaultOutputBuffer output{ pool, 3 };
        output.Write(DefaultBufferPool::ConstBuffer::Range{ constBuffer, constBuffer.begin(), leftOffset, constBuffer.begin(), rightOffset });

        auto buffer = std::move(output).GetBuffer();

        BOOST_TEST(buffer.size() == rightOffset - leftOffset);
        BOOST_TEST(std::distance(buffer.begin(), buffer.end()) == 1);

        auto blob = buffer.begin();
        BOOST_TEST(blob->size() == rightOffset - leftOffset);
        BOOST_TEST(blob->data() == constBlob.data() + leftOffset);

        BOOST_TEST((++blob == buffer.end()));
    }
    {
        DefaultOutputBuffer output{ pool, 2 };
        output.Write(DefaultBufferPool::ConstBuffer::Range{ constBuffer, constBuffer.begin(), leftOffset, std::next(constBuffer.begin()), rightOffset });

        auto buffer = std::move(output).GetBuffer();

        BOOST_TEST(buffer.size() == constBlob.size() - leftOffset + rightOffset);
        BOOST_TEST(std::distance(buffer.begin(), buffer.end()) == 2);

        auto blob = buffer.begin();
        BOOST_TEST(blob->size() == constBlob.size() - leftOffset);
        BOOST_TEST(blob->data() == constBlob.data() + leftOffset);

        ++blob;
        BOOST_TEST(blob->size() == rightOffset);
        BOOST_TEST(blob->data() == constBlob.data());

        BOOST_TEST((++blob == buffer.end()));
    }
    {
        DefaultOutputBuffer output{ pool, 2 };
        output.Write(DefaultBufferPool::ConstBuffer::Range{ constBuffer, constBuffer.begin(), leftOffset, constBuffer.end(), 0 });

        auto buffer = std::move(output).GetBuffer();

        BOOST_TEST(buffer.size() == 3 * constBlob.size() - leftOffset);
        BOOST_TEST(std::distance(buffer.begin(), buffer.end()) == 3);

        auto blob = buffer.begin();
        BOOST_TEST(blob->size() == constBlob.size() - leftOffset);
        BOOST_TEST(blob->data() == constBlob.data() + leftOffset);

        ++blob;
        BOOST_TEST(blob->size() == constBlob.size());
        BOOST_TEST(blob->data() == constBlob.data());

        ++blob;
        BOOST_TEST(blob->size() == constBlob.size());
        BOOST_TEST(blob->data() == constBlob.data());

        BOOST_TEST((++blob == buffer.end()));
    }
}

BOOST_AUTO_TEST_CASE(WriteBondBlobWithPrivateMemoryTest)
{
    auto pool = std::make_shared<DefaultBufferPool>(std::make_shared<SharedMemory>(create_only, GenerateRandomString().c_str(), 1024 * 1024));
    DefaultOutputBuffer output{ pool, 1 };

    const char data[] = "Data";

    output.Write(static_cast<short>(1));
    output.Write(bond::blob{ data, sizeof(data) });
    output.Write(static_cast<int>(2));

    auto buffer = std::move(output).GetBuffer();

    BOOST_TEST(buffer.size() == sizeof(data) + sizeof(short) + sizeof(int));
    BOOST_TEST(std::distance(buffer.begin(), buffer.end()) == 1);

    BOOST_TEST(buffer.size() == buffer.begin()->size());

    auto ptr = buffer.begin()->data();
    BOOST_TEST(*reinterpret_cast<const short*>(ptr) == 1);

    ptr += sizeof(short);
    BOOST_TEST(std::memcmp(ptr, data, sizeof(data)) == 0);

    ptr += sizeof(data);
    BOOST_TEST(*reinterpret_cast<const int*>(ptr) == 2);
}

BOOST_AUTO_TEST_CASE(WriteBondBlobWithSharedMemoryTest)
{
    auto pool = std::make_shared<DefaultBufferPool>(std::make_shared<SharedMemory>(create_only, GenerateRandomString().c_str(), 1024 * 1024));
    DefaultOutputBuffer output{ pool, 2 };

    output.Write(static_cast<short>(1));

    const char data[] = "Data";
    {
        auto blob = pool->TakeBlob();
        blob->resize(sizeof(data), boost::container::default_init);
        std::strcpy(blob->data(), data);

        output.Write(BlobCast(DefaultBufferPool::ConstBlob{ std::move(blob) }, pool->GetMemory()));
    }

    output.Write(static_cast<int>(2));

    auto buffer = std::move(output).GetBuffer();

    BOOST_TEST(buffer.size() == sizeof(data) + sizeof(short) + sizeof(int));
    BOOST_TEST(std::distance(buffer.begin(), buffer.end()) == 3);

    auto blob = buffer.begin();
    BOOST_TEST(blob->size() == sizeof(short));
    BOOST_TEST(*reinterpret_cast<const short*>(blob->data()) == 1);

    ++blob;
    BOOST_TEST(blob->size() == sizeof(data));
    BOOST_TEST(std::memcmp(blob->data(), data, sizeof(data)) == 0);

    ++blob;
    BOOST_TEST(blob->size() == sizeof(int));
    BOOST_TEST(*reinterpret_cast<const int*>(blob->data()) == 2);

    BOOST_TEST((++blob == buffer.end()));
}

BOOST_AUTO_TEST_CASE(WriteEmptyDataTest)
{
    auto pool = std::make_shared<DefaultBufferPool>(std::make_shared<SharedMemory>(create_only, GenerateRandomString().c_str(), 1024 * 1024));

    {
        DefaultOutputBuffer output{ pool };

        auto buffer = std::move(output).GetBuffer();

        BOOST_TEST(!!buffer);
        BOOST_TEST(buffer.size() == 0);
        BOOST_TEST(std::distance(buffer.begin(), buffer.end()) == 0);
    }

    DefaultOutputBuffer output{ pool };

    output.Write(bond::blob{});
    output.Write(DefaultBufferPool::ConstBuffer{});
    output.Write(DefaultBufferPool::ConstBuffer::Range{});

    {
        auto buffer = pool->TakeBuffer();
        DefaultBufferPool::ConstBuffer constBuffer{ std::move(buffer) };

        output.Write(constBuffer);
        output.Write(DefaultBufferPool::ConstBuffer::Range{ constBuffer, constBuffer.begin(), 0, constBuffer.end(), 0 });
    }
    {
        auto buffer = pool->TakeBuffer();
        buffer->push_back(DefaultBufferPool::ConstBlob{});
        DefaultBufferPool::ConstBuffer constBuffer{ std::move(buffer) };

        output.Write(constBuffer);
        output.Write(DefaultBufferPool::ConstBuffer::Range{ constBuffer, constBuffer.begin(), 0, constBuffer.end(), 0 });
    }

    auto buffer = std::move(output).GetBuffer();

    BOOST_TEST(!!buffer);
    BOOST_TEST(buffer.size() == 0);
    BOOST_TEST(std::distance(buffer.begin(), buffer.end()) == 0);
}

BOOST_AUTO_TEST_CASE(FlushTest)
{
    auto pool = std::make_shared<DefaultBufferPool>(std::make_shared<SharedMemory>(create_only, GenerateRandomString().c_str(), 1024 * 1024));
    DefaultOutputBuffer output{ pool };

    const char data[] = "Data";
    const auto& buffer = output.GetBuffer();
    
    output.Write(data, sizeof(data));
    BOOST_TEST(buffer->empty());

    output.Flush();
    BOOST_TEST(buffer->size() == 1);
    BOOST_TEST(std::memcmp(buffer->front().data(), data, sizeof(data)) == 0);

    output.Write(data, sizeof(data));
    BOOST_TEST(buffer->size() == 1);

    output.Flush();
    BOOST_TEST(buffer->size() == 2);
    BOOST_TEST(std::memcmp(buffer->back().data(), data, sizeof(data)) == 0);
}

BOOST_AUTO_TEST_CASE(GetBufferTest)
{
    auto pool = std::make_shared<DefaultBufferPool>(std::make_shared<SharedMemory>(create_only, GenerateRandomString().c_str(), 1024 * 1024));
    DefaultOutputBuffer output{ pool };
    const auto& constOutput = output;

    BOOST_TEST(output.GetBufferPool() == pool);
    
    const char data[] = "Data";

    BOOST_CHECK_NO_THROW(constOutput.GetBuffer());
    BOOST_CHECK_NO_THROW(output.GetBuffer());
    BOOST_TEST(&output.GetBuffer() == &constOutput.GetBuffer());
    BOOST_TEST(output.GetBuffer()->empty());

    output.Write(data, sizeof(data));

    BOOST_CHECK_THROW(constOutput.GetBuffer(), std::exception);
    BOOST_CHECK_NO_THROW(output.GetBuffer());
    BOOST_CHECK_NO_THROW(constOutput.GetBuffer());

    output.Write(data, sizeof(data));

    BOOST_CHECK_THROW(constOutput.GetBuffer(), std::exception);

    auto buffer = std::move(output).GetBuffer();

    BOOST_TEST(buffer.size() == 2 * sizeof(data));
    BOOST_TEST(std::distance(buffer.begin(), buffer.end()) == 2);

    auto blob = buffer.begin();
    BOOST_TEST(std::memcmp(blob->data(), data, sizeof(data)) == 0);

    ++blob;
    BOOST_TEST(std::memcmp(blob->data(), data, sizeof(data)) == 0);

    BOOST_TEST((++blob == buffer.end()));
}

BOOST_AUTO_TEST_CASE(CreateOutputBufferTest)
{
    auto pool = std::make_shared<DefaultBufferPool>(std::make_shared<SharedMemory>(create_only, GenerateRandomString().c_str(), 1024 * 1024));
    DefaultOutputBuffer output{ pool };

    output.Write(1);

    {
        const auto& buffer = output.GetBuffer();
        BOOST_TEST(!!buffer);
        BOOST_TEST(buffer->size() == 1);
    }

    auto output2 = CreateOutputBuffer(output);

    {
        const auto& buffer = output2.GetBuffer();
        BOOST_TEST(!!buffer);
        BOOST_TEST(buffer->empty());
    }
}

BOOST_AUTO_TEST_SUITE_END()
