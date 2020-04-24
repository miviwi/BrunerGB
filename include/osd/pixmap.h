#pragma once

#include <osd/osd.h>

#include <util/arrayview.h>

#include <array>

namespace brgb {

// Forward declarations
class GLPixelBuffer;
class GLTexture2D;

// PIMPL class
struct pOSDPixmap;

class OSDPixmap {
public:
  struct Color {
    u8 r, g, b, a;
  };

  OSDPixmap( unsigned width, unsigned height);
  OSDPixmap( const OSDPixmap&) = delete;
  ~OSDPixmap( );

  auto width() const -> unsigned;
  auto height() const -> unsigned;

  auto stride() const -> size_t;

  auto lock() -> ArrayView<Color>;
  // - Results is a no-op if the OSDPixmap wasn't locked 
  auto unlock() -> OSDPixmap&;

  auto locked() -> bool;

private:
  // Initializes the staging PBO 'staging_bufs_' and
  //   the OSDBitmap's backing texture 'tex_'
  auto init() -> void;

  // Destroys the resources owned by this OSDBitmap
  //   i.e. the staging buffer and backing texture
  //  - A call to destroy() when the resources
  //    haven't been initilized yet or have already
  //    been destroyed is a no-op
  auto destroy() -> void;

  auto wasInit() -> bool;

  unsigned width_ = 0;
  unsigned height_ = 0;

  Color *pixels_ = nullptr;

  // Lazy-initialized by init()
  std::array<GLPixelBuffer *, 2> staging_bufs_ = { nullptr, nullptr };
  GLTexture2D *tex_ = nullptr;

  // 'staging_bufs_' is double-buffered to allow for asynchronity
  //   between writing data to the buffer and uploading
  //   it to the texture
  unsigned current_staging_buf_ = 0;

  // Stores the current buffer mapping
  pOSDPixmap *p = nullptr;
};

}
