#include <osd/font.h>

#include <cassert>

#include <limits>

namespace brdrive {

OSDBitmapFont::OSDBitmapFont() :
  loaded_(false)
{
}

auto OSDBitmapFont::loadBitmap1bppFile(const char *file) -> OSDBitmapFont&
{
  assert(0 && "unimplemented!");

  return *this;
}

auto OSDBitmapFont::loadBitmap1bpp(const void *data, size_t size) -> OSDBitmapFont&
{
  font_pixels_.reserve(size * 8);   // 1bpp  ->  8bpp

  auto packed_font_pixels = (const u8 *)data;
  for(size_t i = 0; i < size; i++) {
    auto packed = packed_font_pixels[i];
    for(int k = 0; k < 8; k++) {
      // Take the bits from MSB to LSB one by one
      //   and expand them to bytes with value
      //   0x00 or 0xFF
      u8 pixel = (packed >> 7)*0xFF;

      font_pixels_.push_back(pixel);
      packed <<= 1;  // Shift out the expanded pixel
    }
  }

  loaded_ = true;

  return *this;
}

auto OSDBitmapFont::pixelData() const -> const u8 *
{
  return loaded_ ? font_pixels_.data() : nullptr;
}

auto OSDBitmapFont::pixelDataSize() const -> size_t
{
  return loaded_ ? font_pixels_.size() : std::numeric_limits<size_t>::max();
}

/*
 * TODO: GET RID of all these hard-coded values :)
 */
auto OSDBitmapFont::numGlyphs() const -> size_t
{
  return 256;
}

auto OSDBitmapFont::glyphDimensions() const -> ivec2
{
  return { 8, 16 };
}

auto OSDBitmapFont::glyphGridLayoutDimensions() const -> ivec2
{
  return { 1, 256 };
}
/*
 *
 */

}
