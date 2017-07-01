#include "stdafx.h"
#include "IPC/Bond/BlobCast.h"
#include "IPC/SharedMemory.h"
#include "IPC/detail/RandomString.h"
#include <memory>

using namespace IPC::Bond;
using IPC::detail::GenerateRandomString;
using IPC::SharedMemory;
using IPC::create_only;
using IPC::anonymous_instance;


BOOST_AUTO_TEST_SUITE(BlobCastTests)

struct BlobMock : public std::shared_ptr<std::pair<const char*, std::size_t>>
{
    BlobMock() = default;

    BlobMock(const char* data, std::size_t size)
        : shared_ptr{ std::make_shared<std::pair<const char*, std::size_t>>(std::make_pair(data, size)) }
    {}

    const char* data() const
    {
        return get()->first;
    }

    std::size_t size() const
    {
        return get()->second;
    }

    BlobMock GetRange(std::size_t offset, std::size_t count) const
    {
        if (offset + count > size())
        {
            throw std::out_of_range{ "Count is out of range." };
        }

        return{ data() + offset, count };
    }
};


BOOST_AUTO_TEST_CASE(CastToBondBlobTest)
{
    auto memory = std::make_shared<SharedMemory>(create_only, GenerateRandomString().c_str(), 1024);

    auto& data = memory->Construct<char[5]>(anonymous_instance);
    std::strcpy(data, "Data");

    BlobMock mock{ data, sizeof(data) };
    BOOST_TEST(mock.unique());

    bond::blob blob = BlobCast(mock, memory);

    BOOST_TEST(blob.data() == data);
    BOOST_TEST(blob.size() == sizeof(data));

    BOOST_TEST(mock.use_count() == 2);
    blob.clear();
    BOOST_TEST(mock.unique());
}

BOOST_AUTO_TEST_CASE(CastToBondBlobMemoryOwnershipTest)
{
    auto memory = std::make_shared<SharedMemory>(create_only, GenerateRandomString().c_str(), 1024);

    auto& data = memory->Construct<char[5]>(anonymous_instance);
    std::strcpy(data, "Data");

    bond::blob blob;
    {
        BlobMock mock{ data, sizeof(data) };
        BOOST_TEST(mock.unique());
        BOOST_TEST(memory.unique());

        blob = BlobCast(mock, memory);
    }

    BOOST_TEST(blob.data() == data);
    BOOST_TEST(blob.size() == sizeof(data));
    BOOST_TEST(memory.use_count() == 2);

    memory.reset();

    BOOST_TEST(std::strcmp(data, "Data") == 0);
}

BOOST_AUTO_TEST_CASE(CastFromCorrectBondBlobTest)
{
    auto memory = std::make_shared<SharedMemory>(create_only, GenerateRandomString().c_str(), 1024);

    auto& data = memory->Construct<char[5]>(anonymous_instance);
    std::strcpy(data, "Data");

    BlobMock mock = BlobCast<BlobMock>(BlobCast(BlobMock{ data, sizeof(data) }, memory));

    BOOST_TEST(mock.data() == data);
    BOOST_TEST(mock.size() == sizeof(data));
}

BOOST_AUTO_TEST_CASE(CastFromCorrectBondBlobWithOffsetTest)
{
    auto memory = std::make_shared<SharedMemory>(create_only, GenerateRandomString().c_str(), 1024);

    auto& data = memory->Construct<char[6]>(anonymous_instance);
    std::strcpy(data, "Data!");

    constexpr std::size_t offset = 2;

    BlobMock mock = BlobCast<BlobMock>(BlobCast(BlobMock{ data, sizeof(data) }, memory).range(offset));

    BOOST_TEST(mock.data() == data + offset);
    BOOST_TEST(mock.size() == sizeof(data) - offset);
}

BOOST_AUTO_TEST_CASE(CastFromIncorrectBondBlobTest)
{
    const char data[] = "Data";

    BlobMock mock = BlobCast<BlobMock>(bond::blob{ data, sizeof(data) });

    BOOST_TEST(!mock);
}

BOOST_AUTO_TEST_SUITE_END()
