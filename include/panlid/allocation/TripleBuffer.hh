#pragma once

#include <array>
#include <atomic>
#include <cassert>
#include <chrono>
#include <concepts>
#include <cstdint>
#include <utility>

namespace panlid::allocation {

template <std::default_initializable T> class TripleBuffer {
  private:
    enum class UsageType : std::uint8_t {
        Read,
        Write,
    };

    struct Buffer {
        T Object{};
        UsageType LastUsage = UsageType::Read;
        int Index = 0;
    };

  public:
    static constexpr std::chrono::milliseconds kDefaultReadTimeout{100};

    class Lease {
      public:
        Lease() = default;

        Lease(const Lease&) = delete;
        Lease& operator=(const Lease&) = delete;
        Lease(Lease&& other) noexcept
            : owner_(std::exchange(other.owner_, nullptr)),
              buffer_(std::exchange(other.buffer_, nullptr)) {}
        Lease& operator=(Lease&& other) noexcept {
            if (this != &other) {
                release();
                owner_ = std::exchange(other.owner_, nullptr);
                buffer_ = std::exchange(other.buffer_, nullptr);
            }
            return *this;
        }

        ~Lease() { release(); }

        explicit operator bool() const { return buffer_ != nullptr; }

        [[nodiscard]] const T& GetObject() const { return buffer_->Object; }
        void SetObject(T value) { buffer_->Object = std::move(value); }

        [[nodiscard]] int GetIndex() const { return buffer_->Index; }

      private:
        friend class TripleBuffer;
        Lease(TripleBuffer* owner, Buffer* buffer) : owner_(owner), buffer_(buffer) {}

        void release() {
            if (owner_) {
                owner_->finishUsage(*buffer_);
            }
            owner_ = nullptr;
            buffer_ = nullptr;
        }

        TripleBuffer* owner_ = nullptr;
        Buffer* buffer_ = nullptr;
    };

    TripleBuffer() {
        for (int i = 0; i < kBufferCount; i++) {
            buffers_[i].Index = i;
        }
    }

    [[nodiscard]] Lease GetForWrite() {
        Buffer& usage = buffers_[writeIndex_];
        usage.LastUsage = UsageType::Write;
        return {this, &usage};
    }

    [[nodiscard]] Lease GetForRead(std::chrono::milliseconds timeout = kDefaultReadTimeout) {
        const auto deadline = std::chrono::steady_clock::now() + timeout;

        while (true) {
            flip(readIndex_);

            if (buffers_[readIndex_].LastUsage != UsageType::Read) {
                break;
            }

            if (timeout == std::chrono::milliseconds::zero() ||
                std::chrono::steady_clock::now() > deadline) {
                return {};
            }
        }

        Buffer& usage = buffers_[readIndex_];

        assert(usage.LastUsage == UsageType::Write);
        usage.LastUsage = UsageType::Read;

        return {this, &usage};
    }

  private:
    static constexpr int kBufferCount = 3;
    std::array<Buffer, kBufferCount> buffers_{};

    int writeIndex_ = 0;
    std::atomic<int> flipIndex_{1};
    int readIndex_ = 2;

    void finishUsage(const Buffer& usage) {
        if (usage.LastUsage == UsageType::Write) {
            flip(writeIndex_);
        }
    }
    void flip(int& localIndex) { localIndex = flipIndex_.exchange(localIndex); }
};

} // namespace panlid::allocation
