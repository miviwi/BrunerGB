#pragma once

#include <cassert>

#include <numeric>

#include <util/natural.h>
#include <util/integer.h>

namespace brdrive {

// Specializations with default width arguments
template <size_t NumBits = 64> struct Natural;
template <size_t NumBits = 64> struct Integer;

template <size_t NumBits>
auto Natural<NumBits>::slice(int bit_index) const  { return Natural<>{ bit(bit_index) }; }
template <size_t NumBits>
auto Natural<NumBits>::slice(int lo, int hi) const { return Natural<>{ bit(lo, hi) }; }

template <size_t NumBits>
auto Integer<NumBits>::slice(int bit_index) const  { return Natural<>{ bit(bit_index) }; }
template <size_t NumBits>
auto Integer<NumBits>::slice(int lo, int hi) const { return Natural<>{ bit(lo, hi) }; }

template <size_t NumBits>
auto Natural<NumBits>::toInteger() const -> Integer<NumBits>
{
  return Integer<NumBits>(*this);
}
template <size_t NumBits>
auto Integer<NumBits>::toNatural() const -> Natural<NumBits>
{
  return Integer<NumBits>(*this);
}

static inline u8 operator"" _u8(unsigned long long v)
{
  assert(v <= std::numeric_limits<u8>::max() &&
      "the value suppiled to '_u8' is outside the range of a byte! (> 255)");

  return (u8)v;
}

static inline u16 operator"" _u16(unsigned long long v)
{
  assert(v <= std::numeric_limits<u16>::max() &&
      "the value suppiled to '_u16' is outside the range of a half-word! (> 65535)");

  return (u16)v;
}

}
