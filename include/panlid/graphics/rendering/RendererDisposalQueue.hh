#pragma once

#include <algorithm>
#include <cassert>
#include <functional>
#include <iterator>
#include <mutex>
#include <utility>
#include <vector>

namespace panlid::graphics::rendering {

// Manages disposal of renderer components in a thread-safe manner: work is queued from any thread
// and run, on the thread that calls CheckPendingDisposals, a fixed number of frames later.
class RendererDisposalQueue {
  public:
    // frameDelay is how many frames after scheduling a disposal runs (the renderer passes its max
    // draw nodes).
    explicit RendererDisposalQueue(int frameDelay) : frameDelay_{frameDelay} {}

    RendererDisposalQueue(const RendererDisposalQueue&) = delete;
    RendererDisposalQueue(RendererDisposalQueue&&) = delete;
    RendererDisposalQueue& operator=(const RendererDisposalQueue&) = delete;
    RendererDisposalQueue& operator=(RendererDisposalQueue&&) = delete;
    ~RendererDisposalQueue() = default;

    // Can be called concurrently from multiple threads.
    void ScheduleDisposal(std::function<void()> disposalAction) {
        std::scoped_lock lock{newDisposalsLock_};
        newDisposals_.push_back(
            {.Action = std::move(disposalAction), .RemainingFrameDelay = frameDelay_});
    }

    // Runs the disposals whose frame delay has expired. Must NOT be called concurrently from
    // multiple threads.
    void CheckPendingDisposals() {
        {
            std::scoped_lock lock{newDisposalsLock_};
            std::ranges::move(newDisposals_, std::back_inserter(pendingDisposals_));
            newDisposals_.clear();
        }

        // Disposals are added in batches every frame, and every frame decrements all remaining
        // delays by 1, so everything due this frame sits at the start of the list. Remember how
        // many are due and remove them in one go (removing them one by one can cost quadratic
        // time).
        int executedCount = 0;
        int index = 0;

        for (PendingDisposal& item : pendingDisposals_) {
            ++index;

            if (item.RemainingFrameDelay-- == 0) {
                item.Action();
                executedCount = index;
            }
        }

        if (executedCount == 0) {
            return;
        }

        const auto firstKept = pendingDisposals_.begin() + executedCount;
        // A 0 in the loop above is a -1 here, due to the postfix decrement.
        assert((firstKept - 1)->RemainingFrameDelay < 0);
        assert(firstKept == pendingDisposals_.end() || firstKept->RemainingFrameDelay >= 0);

        pendingDisposals_.erase(pendingDisposals_.begin(), firstKept);
    }

  private:
    struct PendingDisposal {
        std::function<void()> Action;
        int RemainingFrameDelay{};
    };

    int frameDelay_;
    std::mutex newDisposalsLock_;
    std::vector<PendingDisposal> newDisposals_;
    std::vector<PendingDisposal> pendingDisposals_;
};

} // namespace panlid::graphics::rendering
