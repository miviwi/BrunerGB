#pragma once

#include <osd/osd.h>

#include <window/geometry.h>

#include <vector>
#include <optional>

namespace brdrive {

enum ExtendedCharacter : u8 {

};

struct OSDBitmapFontGlyph {
};

class OSDBitmapFont {
public:
  OSDBitmapFont();

  auto loadBitmap1bppFile(const char *file) -> OSDBitmapFont&;
  auto loadBitmap1bpp(const void *data, size_t size) -> OSDBitmapFont&;

  auto pixelData() const -> const u8 *;
  auto pixelDataSize() const -> size_t;

  auto numGlyphs() const -> size_t;

  // In units of pixels
  auto glyphDimensions() const -> ivec2;

  // In units of glyphs - i.e. the whole bitmap's size is:
  //     glyphDimensions()*glyphGridLayoutDimensions()
  auto glyphGridLayoutDimensions() const -> ivec2;

private:
  // The font's pixels expanded to an 8bpp format
  //   (with values 0x00 and 0xFF) with the glyphs
  //   arranged in memory like so:
  //      <glyph 'A' row> ... <glyph 'A' row> <glyph 'B' row>
  std::vector<u8> font_pixels_;

  bool loaded_;
};

}
