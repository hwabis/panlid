#pragma once

#include "panlid/graphics/textures/ITextureUpload.hh"
#include <cstdint>
#include <memory>
#include <optional>
#include <string>

namespace panlid::graphics::rendering {

class Renderer;

class INativeTexture {
  public:
    INativeTexture() = default;
    INativeTexture(const INativeTexture&) = delete;
    INativeTexture(INativeTexture&&) = delete;
    INativeTexture& operator=(const INativeTexture&) = delete;
    INativeTexture& operator=(INativeTexture&&) = delete;
    virtual ~INativeTexture() = default;

    // The renderer that created this texture.
    [[nodiscard]] virtual Renderer& GetRenderer() const = 0;

    // An identifier for this texture, to show up in the texture visualiser.
    [[nodiscard]] virtual std::string GetIdentifier() const = 0;

    // Maximum texture size in any direction.
    [[nodiscard]] virtual int GetMaxSize() const = 0;

    [[nodiscard]] virtual int GetWidth() const = 0;
    virtual void SetWidth(int width) = 0;

    [[nodiscard]] virtual int GetHeight() const = 0;
    virtual void SetHeight(int height) = 0;

    // The mip level to display, or empty to display all levels automatically (used by the texture
    // visualiser).
    [[nodiscard]] virtual std::optional<int> GetMipLevel() const = 0;
    virtual void SetMipLevel(std::optional<int> mipLevel) = 0;

    // Whether the texture is in a usable state.
    [[nodiscard]] virtual bool IsAvailable() const = 0;

    // The total number of times this texture has been bound, ever. Updated by Renderer.
    [[nodiscard]] virtual std::uint64_t GetTotalBindCount() const = 0;
    virtual void SetTotalBindCount(std::uint64_t count) = 0;

    // By default, uploads are queued for the start of each frame. When true, that is bypassed and
    // the texture is only uploaded on use. Set it for every-frame uploads to avoid overloading the
    // global queue.
    [[nodiscard]] virtual bool IsBypassingTextureUploadQueueing() const = 0;
    virtual void SetBypassTextureUploadQueueing(bool bypass) = 0;

    // Whether the latest data has been uploaded.
    [[nodiscard]] virtual bool IsUploadComplete() const = 0;

    // Flushes any unprocessed uploads without actually uploading.
    virtual void FlushUploads() = 0;

    // Sets the pixel data of the texture. The texture takes ownership of the upload.
    virtual void SetData(std::unique_ptr<textures::ITextureUpload> upload) = 0;

    // Uploads this texture. Returns whether any uploads occurred.
    virtual bool Upload() = 0;

    // The size of this texture in bytes.
    [[nodiscard]] virtual int GetByteSize() const = 0;
};

} // namespace panlid::graphics::rendering
