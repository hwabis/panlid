#include "panlid/allocation/TripleBuffer.hh"
#include <catch2/catch_test_macros.hpp>
#include <chrono>
#include <latch>
#include <memory>
#include <optional>
#include <thread>
#include <vector>

using namespace panlid::allocation;
using namespace std::chrono_literals;

namespace {

struct TestObject {
    explicit TestObject(int id) : Id(id) {}
    int Id;
};

using TestBuffer = TripleBuffer<std::shared_ptr<TestObject>>;

void createWithIdsMatchingIndices(TestBuffer& tripleBuffer) {
    std::vector<int> initialisedBuffers;

    auto initialiseBuffer = [&] {
        auto write = tripleBuffer.GetForWrite();
        write.SetObject(std::make_shared<TestObject>(write.GetIndex()));
        initialisedBuffers.push_back(write.GetIndex());
    };

    initialiseBuffer();

    {
        auto read = tripleBuffer.GetForRead();
        initialiseBuffer();
        initialiseBuffer();
    }

    REQUIRE(initialisedBuffers == std::vector<int>{0, 1, 2});

    // Read remaining buffers to reset things to a sane state.
    {
        auto read = tripleBuffer.GetForRead();
    }
    {
        auto read = tripleBuffer.GetForRead();
    }
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
    createWithIdsMatchingIndices(tripleBuffer);

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
    createWithIdsMatchingIndices(tripleBuffer);

    // Test with first write in use during second.
    {
        auto write = tripleBuffer.GetForWrite();
    }

    std::optional<int> lastRead;
    std::optional<int> lastWrite;

    for (int i = 0; i < 3; ++i) {
        auto read = tripleBuffer.GetForRead();
        REQUIRE(read);
        REQUIRE(read.GetObject()->Id != lastRead);
        lastRead = read.GetObject()->Id; // missing in C#

        for (int j = 0; j < 3; ++j) {
            auto write = tripleBuffer.GetForWrite();
            REQUIRE(write.GetObject()->Id != lastWrite);
            REQUIRE(write.GetObject()->Id != read.GetObject()->Id);
            lastWrite = write.GetObject()->Id;
        }
    }
}

TEST_CASE("WriteThenRead", "[TripleBuffer]") {
    TestBuffer tripleBuffer;

    for (int i = 0; i < 1000; ++i) {
        auto obj = std::make_shared<TestObject>(i);

        {
            auto write = tripleBuffer.GetForWrite();
            write.SetObject(obj);
        }

        auto buffer = tripleBuffer.GetForRead();
        REQUIRE(buffer);
        REQUIRE(buffer.GetObject() == obj);
    }

    auto buffer = tripleBuffer.GetForRead();
    REQUIRE_FALSE(buffer);
}

TEST_CASE("ReadSaturated", "[TripleBuffer][Flaky]") {
    TestBuffer tripleBuffer;

    for (int i = 0; i < 10; ++i) {
        auto obj = std::make_shared<TestObject>(i);
        std::latch readerStarted{1};
        std::shared_ptr<TestObject> received;

        std::thread reader([&] {
            readerStarted.count_down();
            // High timeout to prevent flakiness
            auto buffer = tripleBuffer.GetForRead(1s);
            if (buffer) {
                received = buffer.GetObject();
            }
        });

        std::thread writer([&] {
            readerStarted.wait();
            std::this_thread::sleep_for(10ms);
            auto write = tripleBuffer.GetForWrite();
            write.SetObject(obj);
        });

        reader.join();
        writer.join();

        REQUIRE(received == obj);
    }
}
