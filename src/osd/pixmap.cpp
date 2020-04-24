#include <osd/pixmap.h>
#include <gx/gx.h>
#include <gx/buffer.h>
#include <gx/texture.h>
#include <util/format.h>

#include <optional>

#include <cassert>

namespace brgb {

struct pOSDPixmap {
  std::optional<GLBufferMapping> mapping = std::nullopt;
};

OSDPixmap::OSDPixmap(unsigned width, unsigned height) :
  width_(width), height_(height)
{
}

OSDPixmap::~OSDPixmap()
{
  destroy();
}

auto OSDPixmap::width() const -> unsigned
{
  return width_;
}

auto OSDPixmap::height() const -> unsigned
{
  return height_;
}

auto OSDPixmap::stride() const -> size_t
{
  return width_;
}

auto OSDPixmap::lock() -> ArrayView<Color>
{
  if(!wasInit()) init();    // Lazy-initialize the backing objects

  auto staging = staging_bufs_.at(current_staging_buf_);

  // Map the buffer...
  p->mapping.emplace(staging->map(GLBuffer::MapWrite | GLBuffer::MapInvalidateBuffer));

  //  ...and get a pointer to the mapped memory
  pixels_ = p->mapping->get<Color>();

  return ArrayView(pixels_, width_*height_);
}

auto OSDPixmap::unlock() -> OSDPixmap&
{
  if(!locked()) return *this;

  assert(wasInit());  // Sanity check (!wasInit() && !locked() should be invariant)

  auto staging = staging_bufs_.at(current_staging_buf_);

  // First unmap the staging buffer...
  assert(p && p->mapping.has_value());

  p->mapping = std::nullopt;

  //  ...upload it to the backing texture...
  staging->uploadTexture(*tex_, 0, rgba, GLType::u8);

  //  ...and swap the staging buffer
  current_staging_buf_ ^= 1;

  pixels_ = nullptr;

  return *this;
}

auto OSDPixmap::locked() -> bool
{
  return pixels_;
}

auto OSDPixmap::init() -> void
{
  p = new pOSDPixmap();

  staging_bufs_ = {
    new GLPixelBuffer(GLPixelBuffer::Upload), new GLPixelBuffer(GLPixelBuffer::Upload)
  };
  tex_ = new GLTexture2D();

  // Allocate memory for all the staging GLPixelBuffers...
  for(auto staging : staging_bufs_) {
    staging->alloc(width_*height_ * sizeof(Color), GLBuffer::DynamicDraw, GLBuffer::MapWrite);
  }

  //  ...and the backing texture
  tex_->alloc(width_, height_, 1, rgba8);

  // Give all the GL objects labels
  for(size_t i = 0; i < staging_bufs_.size(); i++) {
    auto staging = staging_bufs_.at(i);
    staging->label(util::fmt("bp.OSD.BitmapStaging%zu", i).data());
  }
  tex_->label("t2d.OSD.BitmapTex");
}

auto OSDPixmap::wasInit() -> bool
{
  return tex_;
}

auto OSDPixmap::destroy() -> void
{
  if(!wasInit()) return;

  // XXX: is unmapping the buffer before it's
  //      destroyed necessary?
  if(locked()) unlock();

  delete p;

  for(auto staging : staging_bufs_) delete staging;
  delete tex_;

  p = nullptr;

  staging_bufs_ = { nullptr, nullptr };
  tex_ = nullptr;
}


}
