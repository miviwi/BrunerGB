#pragma once

#include <cmath>

#include <types.h>
#include <util/bit.h>
#include <util/primitive.h>

namespace brdrive {

struct Color {
public:
  Color() :
    raw_(0),
    r_(&raw_), g_(&raw_), b_(&raw_), a_(&raw_)
  { }

  Color(u8 r, u8 g, u8 b, u8 a = 255) :
    Color()   // Initialize the r_,g_,b_,a_ BitRanges
  {
    r_ = r; g_ = g; b_ = b; a_ = a;
  }

  Color(float r, float g, float b, float a = 1.0f) :
    Color(denormalizef(r), denormalizef(g), denormalizef(b), denormalizef(a))
  { }

  Color(u32 rgba) :
    Color()  // Initialize BitRanges
  {
    raw_ = rgba;
  }

  Color(u24 rgb) :
    Color()
  {
    raw_ = rgb.get();
  }

  Color(const Color& other) :
    Color()
  {
    raw_ = other.raw_;
  }

  static auto transparent() -> Color { return { 0_u8, 0_u8, 0_u8, 0_u8 }; }
  static auto black() -> Color { return { 0_u8, 0_u8, 0_u8 }; }
  static auto white() -> Color { return { 255_u8, 255_u8, 255_u8 }; }
  static auto red() -> Color   { return { 255_u8, 0_u8, 0_u8 }; }
  static auto green() -> Color { return { 0_u8, 255_u8, 0_u8 }; }
  static auto blue() -> Color  { return { 0_u8, 0_u8, 255_u8 }; }

  auto rgba() const -> Natural<32> { return raw_; }
  auto rgb() const -> u24  { return u24(raw_); }
  auto bgra() const -> Natural<32> { return (bswap_u32(raw_) >> 8) | (a_ << 24); }
  auto bgr() const -> u24  { return u24(bswap_u32(raw_) >> 8); }

  auto r() const -> u8 { return (u8)r_.get(); }
  auto g() const -> u8 { return (u8)g_.get(); }
  auto b() const -> u8 { return (u8)b_.get(); }
  auto a() const -> u8 { return (u8)a_.get(); }

  auto fR() const -> float { return normalize(r()); }
  auto fG() const -> float { return normalize(g()); }
  auto fB() const -> float { return normalize(b()); }
  auto fA() const -> float { return normalize(a()); }

private:
  static constexpr float NormFactor = 255.0f;

  static u8 denormalizef(float f) { return (u8)roundf(f * NormFactor); }
  static float normalize(u8 u) { return (float)u / NormFactor; }

  u32 raw_;
  BitRange<32,  0,  7> r_;
  BitRange<32,  8, 15> g_;
  BitRange<32, 16, 23> b_;
  BitRange<32, 24, 31> a_;
};

}
