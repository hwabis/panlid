#pragma once

namespace panlid::graphics::rendering {

// The depth value used to draw 2D objects to the screen.
// Starts at -1 and increments towards 1 for each Drawable which draws an opaque interior through
// DrawNode::DrawOpaqueInterior.
class DepthValue {
  public:
    // Increments the depth value and returns the post-incremented value.
    float Increment() {
        if (count_ == kMaxCount) {
            return depth_;
        }

        depth_ += kIncrement;
        ++count_;

        return depth_;
    }

    void Reset() {
        depth_ = kInitialDepth;
        count_ = 0;
    }

    // Sets the depth to a given value. After this, the depth value can no longer be incremented.
    void Set(float value) {
        count_ = kMaxCount;
        depth_ = value;
    }

    [[nodiscard]] bool CanIncrement() const { return count_ < kMaxCount; }

    [[nodiscard]] float GetValue() const { return depth_; }

  private:
    // A safe value, such that rounding issues don't occur within 16-bit float precision.
    static constexpr float kIncrement = 0.001F;

    // Calculated as (1 - (-1)) / kIncrement - 1.
    // -1 is subtracted since a depth of 1 conflicts with the default backbuffer clear value.
    static constexpr int kMaxCount = 1999;

    static constexpr float kInitialDepth = -1.0F;

    float depth_ = kInitialDepth;
    int count_ = 0;
};

} // namespace panlid::graphics::rendering
