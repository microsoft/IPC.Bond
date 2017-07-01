#include "stdafx.h"
#include "IPC/Bond/BufferPool.h"
#include "IPC/detail/RandomString.h"

using namespace IPC::Bond;
using IPC::detail::GenerateRandomString;
using IPC::SharedMemory;
using IPC::create_only;


BOOST_AUTO_TEST_SUITE(BufferPoolTests)

static_assert(std::is_copy_constructible<DefaultBufferPool>::value, "BufferPool should be copy constructible.");
static_assert(std::is_copy_assignable<DefaultBufferPool>::value, "BufferPool should be copy assignable.");

static_assert(std::is_default_constructible<DefaultBufferPool::Blob>::value, "BufferPool::Blob should be default constructible.");
static_assert(!std::is_copy_constructible<DefaultBufferPool::Blob>::value, "BufferPool::Blob should not be copy constructible.");
static_assert(!std::is_copy_assignable<DefaultBufferPool::Blob>::value, "BufferPool::Blob should not be copy assignable.");
static_assert(std::is_move_constructible<DefaultBufferPool::Blob>::value, "BufferPool::Blob should be move constructible.");
static_assert(std::is_move_assignable<DefaultBufferPool::Blob>::value, "BufferPool::Blob should be move assignable.");

static_assert(std::is_default_constructible<DefaultBufferPool::ConstBlob>::value, "BufferPool::ConstBlob should be default constructible.");
static_assert(std::is_copy_constructible<DefaultBufferPool::ConstBlob>::value, "BufferPool::ConstBlob should be copy constructible.");
static_assert(std::is_copy_assignable<DefaultBufferPool::ConstBlob>::value, "BufferPool::ConstBlob should be copy assignable.");

static_assert(std::is_default_constructible<DefaultBufferPool::Buffer>::value, "BufferPool::Buffer should be default constructible.");
static_assert(!std::is_copy_constructible<DefaultBufferPool::Buffer>::value, "BufferPool::Buffer should not be copy constructible.");
static_assert(!std::is_copy_assignable<DefaultBufferPool::Buffer>::value, "BufferPool::Buffer should not be copy assignable.");
static_assert(std::is_move_constructible<DefaultBufferPool::Buffer>::value, "BufferPool::Buffer should be move constructible.");
static_assert(std::is_move_assignable<DefaultBufferPool::Buffer>::value, "BufferPool::Buffer should be move assignable.");

static_assert(std::is_default_constructible<DefaultBufferPool::ConstBuffer>::value, "BufferPool::ConstBuffer should be default constructible.");
static_assert(std::is_copy_constructible<DefaultBufferPool::ConstBuffer>::value, "BufferPool::ConstBuffer should be copy constructible.");
static_assert(std::is_copy_assignable<DefaultBufferPool::ConstBuffer>::value, "BufferPool::ConstBuffer should be copy assignable.");

static_assert(std::is_default_constructible<DefaultBufferPool::ConstBuffer::Range>::value, "BufferPool::ConstBuffer::Range should be default constructible.");
static_assert(std::is_copy_constructible<DefaultBufferPool::ConstBuffer::Range>::value, "BufferPool::ConstBuffer::Range should be copy constructible.");
static_assert(std::is_copy_assignable<DefaultBufferPool::ConstBuffer::Range>::value, "BufferPool::ConstBuffer::Range should be copy assignable.");

BOOST_AUTO_TEST_CASE(BlobTest)
{
    auto name = GenerateRandomString();
    auto memory = std::make_shared<SharedMemory>(create_only, name.c_str(), 1024 * 1024);
    auto pool = std::make_unique<DefaultBufferPool>(memory);

    auto blob = pool->TakeBlob();
    BOOST_TEST(!!blob);
    BOOST_TEST(blob->empty());

    BOOST_TEST(&*blob == blob.operator->());

    BOOST_CHECK_THROW((blob->resize(memory->GetFreeSize() + 1)), std::exception);

    blob->resize(1024, boost::container::default_init);
    BOOST_TEST(blob->size() == 1024);

    BOOST_TEST(memory->Contains(blob->data()));
    BOOST_TEST(memory->Contains(&*blob));

    auto ptr = &*blob;
    blob = {};
    BOOST_TEST(!blob);

    blob = pool->TakeBlob();
    BOOST_TEST(!!blob);
    BOOST_TEST(blob->empty());

    BOOST_TEST(ptr == &*blob);

    pool.reset();
    BOOST_CHECK_NO_THROW(blob = {});
    BOOST_TEST(!blob);
}

BOOST_AUTO_TEST_CASE(ConstBlobTest)
{
    auto name = GenerateRandomString();
    auto memory = std::make_shared<SharedMemory>(create_only, name.c_str(), 1024 * 1024);
    auto pool = std::make_unique<DefaultBufferPool>(memory);

    BOOST_TEST(!DefaultBufferPool::ConstBlob{});

    auto blob = pool->TakeBlob();
    blob->resize(1024, boost::container::default_init);

    auto data = blob->data();
    auto size = blob->size();

    DefaultBufferPool::ConstBlob constBlob{ std::move(blob) };

    BOOST_TEST(!!constBlob);
    BOOST_TEST(constBlob.data() == data);
    BOOST_TEST(constBlob.size() == size);
    BOOST_TEST(&*constBlob.begin() == data);
    BOOST_TEST(&*constBlob.end() == data + size);

    BOOST_CHECK_THROW(constBlob.GetRange(1, size), std::exception);
    BOOST_CHECK_THROW(constBlob.GetRange(size, 1), std::exception);

    constexpr std::size_t offset = 2;
    constexpr std::size_t count = 2;

    auto range = constBlob.GetRange(offset, count);
    BOOST_TEST(!!range);
    BOOST_TEST(range.data() == data + offset);
    BOOST_TEST(range.size() == count);
    BOOST_TEST(&*range.begin() == data + offset);
    BOOST_TEST(&*range.end() == data + offset + count);
}

BOOST_AUTO_TEST_CASE(BufferTest)
{
    auto name = GenerateRandomString();
    auto memory = std::make_shared<SharedMemory>(create_only, name.c_str(), 1024 * 1024);
    auto pool = std::make_unique<DefaultBufferPool>(memory);

    auto buffer = pool->TakeBuffer();
    BOOST_TEST(!!buffer);
    BOOST_TEST(buffer->empty());

    BOOST_TEST(&*buffer == buffer.operator->());

    BOOST_CHECK_THROW((buffer->resize(memory->GetFreeSize() + 1)), std::exception);

    DefaultBufferPool::ConstBlob b1{ pool->TakeBlob() }, b2{ pool->TakeBlob() };

    buffer->push_back(b1);
    buffer->push_back(b2);
    BOOST_TEST(buffer->size() == 2);

    BOOST_TEST(memory->Contains(buffer->data()));
    BOOST_TEST(memory->Contains(&*buffer));

    auto ptr = &*buffer;
    buffer = {};
    BOOST_TEST(!buffer);

    buffer = pool->TakeBuffer();
    BOOST_TEST(!!buffer);
    BOOST_TEST(buffer->empty());

    BOOST_TEST(ptr == &*buffer);

    pool.reset();
    BOOST_CHECK_NO_THROW(buffer = {});
    BOOST_TEST(!buffer);
}

BOOST_AUTO_TEST_CASE(ConstBufferTest)
{
    auto name = GenerateRandomString();
    auto memory = std::make_shared<SharedMemory>(create_only, name.c_str(), 1024 * 1024);
    auto pool = std::make_unique<DefaultBufferPool>(memory);

    BOOST_TEST(!DefaultBufferPool::ConstBuffer{});

    auto buffer = pool->TakeBuffer();
    {
        auto blob = pool->TakeBlob();
        blob->resize(100, boost::container::default_init);
        buffer->push_back(std::move(blob));
    }
    {
        auto blob = pool->TakeBlob();
        blob->resize(200, boost::container::default_init);
        buffer->push_back(std::move(blob));
    }

    auto data = buffer->data();
    auto size = buffer->size();

    DefaultBufferPool::ConstBuffer constBuffer{ std::move(buffer) };

    BOOST_TEST(!!constBuffer);
    BOOST_TEST(&*constBuffer.begin() == data);
    BOOST_TEST(&*constBuffer.end() == data + size);

    BOOST_TEST(constBuffer.size() == 300);

    BOOST_CHECK_NO_THROW((DefaultBufferPool::ConstBuffer::Range{ constBuffer, constBuffer.begin(), 0, constBuffer.end(), 0 }));

    BOOST_TEST((constBuffer == constBuffer));
    BOOST_TEST((DefaultBufferPool::ConstBuffer{} == DefaultBufferPool::ConstBuffer{}));
    BOOST_TEST(!(constBuffer == DefaultBufferPool::ConstBuffer{}));
}

BOOST_AUTO_TEST_CASE(ConstBufferRangeTest)
{
    auto name = GenerateRandomString();
    auto memory = std::make_shared<SharedMemory>(create_only, name.c_str(), 1024 * 1024);
    auto pool = std::make_unique<DefaultBufferPool>(memory);

    BOOST_TEST(DefaultBufferPool::ConstBuffer::Range{}.IsEmpty());

    {
        auto buffer = pool->TakeBuffer();
        DefaultBufferPool::ConstBuffer constBuffer{ std::move(buffer) };

        BOOST_TEST((DefaultBufferPool::ConstBuffer::Range{ constBuffer, constBuffer.begin(), 0, constBuffer.end(), 0 }.IsEmpty()));
        BOOST_TEST((DefaultBufferPool::ConstBuffer::Range{ constBuffer, constBuffer.begin(), 10, constBuffer.end(), 10 }.IsEmpty()));
    }
    {
        auto buffer = pool->TakeBuffer();
        buffer->push_back(pool->TakeBlob());
        DefaultBufferPool::ConstBuffer constBuffer{ std::move(buffer) };

        BOOST_TEST(!(DefaultBufferPool::ConstBuffer::Range{ constBuffer, constBuffer.begin(), 0, constBuffer.end(), 0 }.IsEmpty()));
        BOOST_TEST(!(DefaultBufferPool::ConstBuffer::Range{ constBuffer, constBuffer.begin(), 10, constBuffer.end(), 10 }.IsEmpty()));
        BOOST_TEST((DefaultBufferPool::ConstBuffer::Range{ constBuffer, constBuffer.begin(), 0, constBuffer.begin(), 0 }.IsEmpty()));
        BOOST_TEST((DefaultBufferPool::ConstBuffer::Range{ constBuffer, constBuffer.begin(), 10, constBuffer.begin(), 10 }.IsEmpty()));
    }
}

BOOST_AUTO_TEST_SUITE_END()
