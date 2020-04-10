#pragma once

#include <types.h>
#include <util/bit.h>

#include <type_traits>
#include <algorithm>

namespace brdrive {

// Forward declaration
template <size_t NumBits>
struct Natural;

// Signed arbitrary-width integer
//
// Heavily inspired by byuu's 'nall' library (nall::Integer)
template <size_t NumBits>
struct Integer {
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

  inline Integer() : data_(0) { }
  template <size_t OtherBits> inline Integer(Integer<OtherBits> other) : data_(cast(other.get())) { }
  template <typename T> inline Integer(const T& other) : data_(cast(other)) { }

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

  inline auto toNatural() const -> Natural<NumBits>;

private:
  // Down/upcast a value to a width of 'Bits'
  //   performing sign extension when downcasting
  static inline auto cast(StorageType val) -> StorageType
  {
    return (val & Mask ^ Sign) - Sign;
  }

  StorageType data_;

};

using i1 = Integer<1>;   using i2 = Integer<2>;   using i3 = Integer<3>;   using i4 = Integer<4>;
using i5 = Integer<5>;   using i6 = Integer<6>;   using i7 = Integer<7>;   /* using i8 = Integer<8>; */
using i9 = Integer<9>;   using i10 = Integer<10>; using i11 = Integer<11>; using i12 = Integer<12>;
using i13 = Integer<13>; using i14 = Integer<14>; using i15 = Integer<15>; /* using i16 = Integer<16>; */
using i17 = Integer<17>; using i18 = Integer<18>; using i19 = Integer<19>; using i20 = Integer<20>;
using i21 = Integer<21>; using i22 = Integer<22>; using i23 = Integer<23>; using i24 = Integer<24>;
using i25 = Integer<25>; using i26 = Integer<26>; using i27 = Integer<27>; using i28 = Integer<28>;
using i29 = Integer<29>; using i30 = Integer<30>; using i31 = Integer<31>; /* using i32 = Integer<32>; */
using i33 = Integer<33>; using i34 = Integer<34>; using i35 = Integer<35>; using i36 = Integer<36>;
using i37 = Integer<37>; using i38 = Integer<38>; using i39 = Integer<39>; using i40 = Integer<40>;
using i41 = Integer<41>; using i42 = Integer<42>; using i43 = Integer<43>; using i44 = Integer<44>;
using i45 = Integer<45>; using i46 = Integer<46>; using i47 = Integer<47>; using i48 = Integer<48>;
using i49 = Integer<49>; using i50 = Integer<50>; using i51 = Integer<51>; using i52 = Integer<52>;
using i53 = Integer<53>; using i54 = Integer<54>; using i55 = Integer<55>; using i56 = Integer<56>;
using i57 = Integer<57>; using i58 = Integer<58>; using i59 = Integer<59>; using i60 = Integer<60>;
using i61 = Integer<61>; using i62 = Integer<62>; using i63 = Integer<63>; /* using i64 = Integer<64>; */

}
