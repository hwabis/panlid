#pragma once

#include "panlid/graphics/Rgba32.hh"
#include "panlid/graphics/primitives/RectangleI.hh"
#include <span>

namespace panlid::graphics::textures {

// A block of pixel data queued for upload into a texture. Implementations release their buffer in
// the destructor.
class ITextureUpload {
  public:
    ITextureUpload() = default;
    ITextureUpload(const ITextureUpload&) = delete;
    ITextureUpload(ITextureUpload&&) = delete;
    ITextureUpload& operator=(const ITextureUpload&) = delete;
    ITextureUpload& operator=(ITextureUpload&&) = delete;
    virtual ~ITextureUpload() = default;

    // The raw data to be uploaded.
    [[nodiscard]] virtual std::span<const Rgba32> GetData() const = 0;

    // The target mipmap level to upload into.
    [[nodiscard]] virtual int GetLevel() const = 0;

    // The target bounds for this upload. If not specified, assumed to be (0, 0, width, height).
    [[nodiscard]] virtual primitives::RectangleI GetBounds() const = 0;
    virtual void SetBounds(const primitives::RectangleI& bounds) = 0;
};

} // namespace panlid::graphics::textures
