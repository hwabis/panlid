#include "panlid/allocation/TripleBuffer.hh"
#include <catch2/catch_test_macros.hpp>
#include <chrono>
#include <latch>
#include <memory>
#include <optional>
#include <thread>
#include <vector>

using panlid::allocation::TripleBuffer;

namespace {

struct TestObject {
    int Id;
};

using TestBuffer = TripleBuffer<std::shared_ptr<TestObject>>;

constexpr std::chrono::milliseconds kWriteDelay{10};

std::shared_ptr<TestObject> MakeObject(int id) {
    return std::make_shared<TestObject>(TestObject{.Id = id});
}

int InitialiseBuffer(TestBuffer& tripleBuffer) {
    auto write = tripleBuffer.GetForWrite();
    write.SetObject(MakeObject(write.GetIndex()));
    return write.GetIndex();
}

void CreateWithIdsMatchingIndices(TestBuffer& tripleBuffer) {
    std::vector<int> initialisedBuffers;

    initialisedBuffers.push_back(InitialiseBuffer(tripleBuffer));

    {
        auto read = tripleBuffer.GetForRead();
        initialisedBuffers.push_back(InitialiseBuffer(tripleBuffer));
        initialisedBuffers.push_back(InitialiseBuffer(tripleBuffer));
    }

    REQUIRE(initialisedBuffers == std::vector<int>{0, 1, 2});

    {
        auto read = tripleBuffer.GetForRead();
    }
    {
        auto read = tripleBuffer.GetForRead();
    }
}

void WriteWhileReading(TestBuffer& tripleBuffer, int readId, std::optional<int>& lastWrite) {
    for (int i = 0; i < 3; ++i) {
        auto write = tripleBuffer.GetForWrite();
        REQUIRE(write.GetObject()->Id != lastWrite);
        REQUIRE(write.GetObject()->Id != readId);
        lastWrite = write.GetObject()->Id;
    }
}

std::shared_ptr<TestObject> ReadWhileWriteArrives(TestBuffer& tripleBuffer,
                                                  const std::shared_ptr<TestObject>& object) {
    std::latch readerStarted{1};
    std::shared_ptr<TestObject> observed;

    {
        std::jthread reader{[&] {
            readerStarted.count_down();

            if (auto buffer = tripleBuffer.GetForRead()) {
                observed = buffer.GetObject();
            }
        }};

        std::jthread writer{[&] {
            readerStarted.wait();
            std::this_thread::sleep_for(kWriteDelay);

            auto write = tripleBuffer.GetForWrite();
            write.SetObject(object);
        }};
    }

    return observed;
}

} // namespace

TEST_CASE("WriteOnly", "[TripleBuffer]") {
    TestBuffer tripleBuffer;

    for (int i = 0; i < 1000; ++i) {
        auto write = tripleBuffer.GetForWrite();
    }
}

TEST_CASE("ReadOnly", "[TripleBuffer]") {
    TestBuffer tripleBuffer;

    auto buffer = tripleBuffer.GetForRead();
    REQUIRE_FALSE(buffer);
}

TEST_CASE("SameBufferIsNotWrittenTwiceInRowNoContestation", "[TripleBuffer]") {
    TestBuffer tripleBuffer;
    CreateWithIdsMatchingIndices(tripleBuffer);

    std::optional<int> lastWrite;

    for (int i = 0; i < 3; ++i) {
        {
            auto write = tripleBuffer.GetForWrite();
            REQUIRE(write.GetObject()->Id != lastWrite);
            lastWrite = write.GetObject()->Id;
        }

        auto buffer = tripleBuffer.GetForRead();
        REQUIRE(buffer);
        REQUIRE(buffer.GetObject()->Id == lastWrite);
    }
}

TEST_CASE("SameBufferIsNotWrittenTwiceInRowContestation", "[TripleBuffer]") {
    TestBuffer tripleBuffer;
    CreateWithIdsMatchingIndices(tripleBuffer);

    {
        auto write = tripleBuffer.GetForWrite();
    }

    std::optional<int> lastRead;
    std::optional<int> lastWrite;

    for (int i = 0; i < 3; ++i) {
        auto read = tripleBuffer.GetForRead();
        REQUIRE(read);
        REQUIRE(read.GetObject()->Id != lastRead);
        lastRead = read.GetObject()->Id;

        WriteWhileReading(tripleBuffer, read.GetObject()->Id, lastWrite);
    }
}

TEST_CASE("WriteThenRead", "[TripleBuffer]") {
    TestBuffer tripleBuffer;

    for (int i = 0; i < 1000; ++i) {
        const auto object = MakeObject(i);

        {
            auto write = tripleBuffer.GetForWrite();
            write.SetObject(object);
        }

        auto buffer = tripleBuffer.GetForRead();
        REQUIRE(buffer);
        REQUIRE(buffer.GetObject() == object);
    }

    auto buffer = tripleBuffer.GetForRead();
    REQUIRE_FALSE(buffer);
}

TEST_CASE("ReadSaturated", "[TripleBuffer]") {
    TestBuffer tripleBuffer;

    for (int i = 0; i < 10; ++i) {
        const auto object = MakeObject(i);
        REQUIRE(ReadWhileWriteArrives(tripleBuffer, object) == object);
    }
}
