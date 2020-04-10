#pragma once

#include <types.h>
#include <util/bit.h>

#include <type_traits>
#include <algorithm>

namespace brdrive {

// Forward declaration
template <size_t NumBits>
struct Integer;

// Unsigned arbitrary-width integer
//
// Heavily inspired by byuu's 'nall' library (nall::Integer)
template <size_t NumBits>
struct Natural {
  static_assert(NumBits >= 1 && NumBits <= 64, "Invalid size specified!");

  static constexpr size_t Bits = NumBits;

  // Underlying native type used for storage
  using StorageType = std::conditional_t<
/* if */      Bits <= 8,  u8,  std::conditional_t<
/* else if */ Bits <= 16, u16, std::conditional_t<
/* else if */ Bits <= 32, u32, std::conditional_t<
/* else if */ Bits <= 64, u64,
    void>>>>;

  enum : StorageType {
    Mask = ~0ull >> (64 - NumBits),
    Sign = 1ull << (NumBits-1ull),
  };

  inline Natural() : data_(0) { }
  template <size_t OtherBits> inline Natural(Natural<OtherBits> other) : data_(cast(other.get())) { }
  template <typename T> inline Natural(const T& other) : data_(cast(other)) { }

  inline auto get() const -> StorageType { return data_; }
  inline operator StorageType() const { return get(); }

  inline auto get() -> StorageType { return data_; }
  inline operator StorageType() { return get(); }

  inline auto operator++(int)
  {
    auto v = *this;
    data_ = cast(data_ + 1);

    return v;
  }
  inline auto operator--(int)
  {
    auto v = *this;
    data_ = cast(data_ - 1);

    return v;
  }

  inline auto& operator++() { data_ = cast(data_ + 1); return *this; }
  inline auto& operator--() { data_ = cast(data_ - 1); return *this; }

  template <typename T>
  inline auto& operator=(const T& v)
  {
    data_ = cast(v);
    return *this;
  }
  template <typename T>
  inline auto& operator+=(const T& v)
  {
    data_ = cast(data_ + v);
    return *this;
  }
  template <typename T>
  inline auto& operator-=(const T& v)
  {
    data_ = cast(data_ - v);
    return *this;
  }
  template <typename T>
  inline auto& operator*=(const T& v)
  {
    data_ = cast(data_ * v);
    return *this;
  }
  template <typename T>
  inline auto& operator/=(const T& v)
  {
    data_ = cast(data_ / v);
    return *this;
  }
  template <typename T>
  inline auto& operator%=(const T& v)
  {
    data_ = cast(data_ % v);
    return *this;
  }
  template <typename T>
  inline auto& operator<<=(const T& v)
  {
    data_ = cast(data_ << v);
    return *this;
  }
  template <typename T>
  inline auto& operator>>=(const T& v)
  {
    data_ = cast(data_ >> v);
    return *this;
  }
  template <typename T>
  inline auto& operator&=(const T& v)
  {
    data_ = cast(data_ & v);
    return *this;
  }
  template <typename T>
  inline auto& operator|=(const T& v)
  {
    data_ = cast(data_ | v);
    return *this;
  }
  template <typename T>
  inline auto& operator^=(const T& v)
  {
    data_ = cast(data_ ^ v);
    return *this;
  }

  inline auto bit(int bit_index) -> BitRange<NumBits> { return { &data_, bit_index }; }
  inline auto bit(int bit_index) const -> const BitRange<NumBits> { return { &data_, bit_index }; }

  inline auto bit(int lo, int hi) -> BitRange<NumBits> { return { &data_, lo, hi }; }
  inline auto bit(int lo, int hi) const -> const BitRange<NumBits> { return { &data_, lo, hi }; }

  inline auto operator()(int bit_index) -> BitRange<NumBits> { return bit(bit_index); }
  inline auto operator()(int bit_index) const -> const BitRange<NumBits> { return bit(bit_index); }

  inline auto operator()(int lo, int hi) -> BitRange<NumBits> { return bit(lo, hi); }
  inline auto operator()(int lo, int hi) const -> const BitRange<NumBits> { return bit(lo, hi); }

  inline auto byte(int index) -> BitRange<NumBits> { return { &data_, index*8 + 0, index*8 + 7 }; }
  inline auto byte(int index) const -> const BitRange<NumBits> { return { &data_, index*8 + 0, index*8 + 7 }; }

  inline auto slice(int bit_index) const;
  inline auto slice(int lo, int hi) const;

  inline auto clamp(unsigned bits) const -> StorageType
  {
    const i64 b = (1ull << bits) - 1;
    const i64 m = b - 1;

    return std::min(m, std::max(-b, data_));
  }

  inline auto clip(unsigned bits) const -> StorageType
  {
    const i64 b = (1ull << bits) - 1;
    const i64 m = b*2 - 1;

    return (data_ & m ^ b) - b;
  }

  inline auto toInteger() const -> Integer<NumBits>;

private:
  // Down/upcast a value to a width of 'Bits'
  //   WITHOUT performing ign extension
  static inline auto cast(StorageType val) -> StorageType
  {
    return val & Mask;
  }

  StorageType data_;

};

using u1 = Natural<1>;   using u2 = Natural<2>;   using u3 = Natural<3>;   using u4 = Natural<4>;
using u5 = Natural<5>;   using u6 = Natural<6>;   using u7 = Natural<7>;   /* using u8 = Natural<8>; */
using u9 = Natural<9>;   using u10 = Natural<10>; using u11 = Natural<11>; using u12 = Natural<12>;
using u13 = Natural<13>; using u14 = Natural<14>; using u15 = Natural<15>; /* using u16 = Natural<16>; */
using u17 = Natural<17>; using u18 = Natural<18>; using u19 = Natural<19>; using u20 = Natural<20>;
using u21 = Natural<21>; using u22 = Natural<22>; using u23 = Natural<23>; using u24 = Natural<24>;
using u25 = Natural<25>; using u26 = Natural<26>; using u27 = Natural<27>; using u28 = Natural<28>;
using u29 = Natural<29>; using u30 = Natural<30>; using u31 = Natural<31>; /* using u32 = Natural<32>; */
using u33 = Natural<33>; using u34 = Natural<34>; using u35 = Natural<35>; using u36 = Natural<36>;
using u37 = Natural<37>; using u38 = Natural<38>; using u39 = Natural<39>; using u40 = Natural<40>;
using u41 = Natural<41>; using u42 = Natural<42>; using u43 = Natural<43>; using u44 = Natural<44>;
using u45 = Natural<45>; using u46 = Natural<46>; using u47 = Natural<47>; using u48 = Natural<48>;
using u49 = Natural<49>; using u50 = Natural<50>; using u51 = Natural<51>; using u52 = Natural<52>;
using u53 = Natural<53>; using u54 = Natural<54>; using u55 = Natural<55>; using u56 = Natural<56>;
using u57 = Natural<57>; using u58 = Natural<58>; using u59 = Natural<59>; using u60 = Natural<60>;
using u61 = Natural<61>; using u62 = Natural<62>; using u63 = Natural<63>; /* using u64 = Natural<64>; */

}
