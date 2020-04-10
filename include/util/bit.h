#pragma once

#include <types.h>

#include <type_traits>
#include <algorithm>

namespace brdrive {

template <size_t Precision, int...>
struct BitRange;

// Staticly sized view into a range
//   of a number's bits
// Heavily inspired by byuu's 'nall' library (nall::BitRange)
template <size_t Precision, int LoIdx, int HiIdx>
struct BitRange<Precision, LoIdx, HiIdx> {
  static_assert(Precision >= 1 && Precision <= 64, "Invalid size specified!");

  // Type of the target number
  using Type = std::conditional_t<
/* if */      Precision <= 8,  u8,  std::conditional_t<
/* else if */ Precision <= 16, u16, std::conditional_t<
/* else if */ Precision <= 32, u32, std::conditional_t<
/* else if */ Precision <= 64, u64,
    void>>>>;

  enum : unsigned {
    Lo = LoIdx < 0 ? Precision+LoIdx : LoIdx,
    Hi = HiIdx < 0 ? Precision+HiIdx : HiIdx,

    Shift = Lo,
    Width = Hi - Lo + 1,

    // Declared for compatibility between static and dynamic BitRanges
    shift_ = Shift,
  };

  enum : Type {
    Mask = (~0ull >> (64 - Width)) << Lo,

    // Declared for compatibility between static and dynamic BitRanges
    mask_ = Mask,
  };

  BitRange(const BitRange& other) = delete;

  template <typename T>
  inline BitRange(T *source) :
    target_(*(Type *)source)
  {
    static_assert(sizeof(T) == sizeof(Type), "sanity check failed!");
  }

  inline auto get() const -> Type { return (target_ & Mask) >> Shift; }
  inline operator Type() const { return get(); }

  inline auto get() -> Type { return (target_ & Mask) >> Shift; }
  inline operator Type() { return get(); }

  inline auto& operator=(const BitRange& source)
  {
    auto source_bits = (source.target_ & source.mask_) >> source.shift_;

    target_ = bitsOutside() | shiftAndMask(source_bits);

    return *this;
  }

  inline auto operator++(int)
  {
    auto v = get();
    target_ = bitsOutside() | target_ + shiftAndMask(1);

    return v;
  }
  inline auto operator--(int)
  {
    auto v = get();
    target_ = bitsOutside() | target_ - shiftAndMask(1);

    return v;
  }
  inline auto& operator++()
  {
    target_ = bitsOutside() | target_ + shiftAndMask(1);
    return *this;
  }
  inline auto operator--()
  {
    target_ = bitsOutside() | target_ - shiftAndMask(1);
    return *this;
  }

  template <typename T>
  inline auto& operator=(const T& source)
  {
    target_ = bitsOutside() | shiftAndMask(source);
    return *this;
  }
  template <typename T>
  inline auto& operator+=(const T& source)
  {
    target_ = bitsOutside() | shiftAndMask(get() + source);
    return *this;
  }
  template <typename T>
  inline auto& operator-=(const T& source)
  {
    target_ = bitsOutside() | shiftAndMask(get() - source);
    return *this;
  }
  template <typename T>
  inline auto& operator*=(const T& source)
  {
    target_ = bitsOutside() | shiftAndMask(get() * source);
    return *this;
  }
  template <typename T>
  inline auto& operator/=(const T& source)
  {
    target_ = bitsOutside() | shiftAndMask(get() / source);
    return *this;
  }
  template <typename T>
  inline auto& operator%=(const T& source)
  {
    target_ = bitsOutside() | shiftAndMask(get() % source);
    return *this;
  }
  template <typename T>
  inline auto& operator<<=(const T& source)
  {
    target_ = bitsOutside() | shiftAndMask(get() << source);
    return *this;
  }
  template <typename T>
  inline auto& operator>>=(const T& source)
  {
    target_ = bitsOutside() | shiftAndMask(get() >> source);
    return *this;
  }
  template <typename T>
  inline auto& operator&=(const T& source)
  {
    target_ = bitsOutside() | shiftAndMask(get() & source);
    return *this;
  }
  template <typename T>
  inline auto& operator|=(const T& source)
  {
    target_ = bitsOutside() | shiftAndMask(get() | source);
    return *this;
  }
  template <typename T>
  inline auto& operator^=(const T& source)
  {
    target_ = bitsOutside() | shiftAndMask(get() ^ source);
    return *this;
  }

private:
  inline auto bitsOutside() const { return target_ & ~Mask; }
  inline auto shiftAndMask(Type v) const { return (v << Shift) & Mask; }

  Type& target_;
};

// Dynamically sized view into a range
//   of a number's bits
// Heavily inspired by byuu's 'nall' library (nall::BitRange)
template <size_t Precision>
struct BitRange<Precision> {
  static_assert(Precision >= 1 && Precision <= 64, "Invalid size specified!");

  // Type of the target number
  using Type = std::conditional_t<
/* if */      Precision <= 8,  u8,  std::conditional_t<
/* else if */ Precision <= 16, u16, std::conditional_t<
/* else if */ Precision <= 32, u32, std::conditional_t<
/* else if */ Precision <= 64, u64,
    void>>>>;

  template <typename T>
  inline BitRange(T *target, int index) :
    target_(*(Type *)target)
  {
    static_assert(sizeof(T) == sizeof(Type), "sanity check failed!");

    // Negative index indicates we're indexing from the RIGHT
    if(index < 0) index = Precision + index;

    mask_  = 1ull << index;
    shift_ = index;
  }

  template <typename T>
  inline BitRange(T *target, int lo, int hi) :
    target_(*(Type *)target)
  {
    static_assert(sizeof(T) == sizeof(Type), "sanity check failed!");

    // Negative lo/hi indicates we're indexing from the RIGHT
    if(lo < 0) lo = Precision + lo;
    if(hi < 0) hi = Precision + hi;

    // The range could get flipped when converting from
    //   right-based indices to left-based ones
    if(lo > hi) std::swap(lo, hi);

    auto range_width = hi-lo + 1;
    auto base_mask   = (1ull << range_width)-1;

    mask_  = base_mask << lo;
    shift_ = lo;
  }

  BitRange(const BitRange&) = delete;

  inline auto get() const -> Type { return (target_ & mask_) >> shift_; }
  inline operator Type() const { return get(); }

  inline auto get() -> Type { return (target_ & mask_) >> shift_; }
  inline operator Type() { return get(); }

  inline auto& operator=(const BitRange& source)
  {
    auto source_bits = (source.target_ & source.mask_) >> source.shift_;

    target_ = bitsOutside() | shiftAndMask(source_bits);

    return *this;
  }

  inline auto operator++(int)
  {
    auto v = get();
    target_ = bitsOutside() | target_ + shiftAndMask(1);

    return v;
  }
  inline auto operator--(int)
  {
    auto v = get();
    target_ = bitsOutside() | target_ - shiftAndMask(1);

    return v;
  }
  inline auto& operator++()
  {
    target_ = bitsOutside() | target_ + shiftAndMask(1);
    return *this;
  }
  inline auto operator--()
  {
    target_ = bitsOutside() | target_ - shiftAndMask(1);
    return *this;
  }

  template <typename T>
  inline auto& operator=(const T& source)
  {
    target_ = bitsOutside() | shiftAndMask(source);
    return *this;
  }
  template <typename T>
  inline auto& operator+=(const T& source)
  {
    target_ = bitsOutside() | shiftAndMask(get() + source);
    return *this;
  }
  template <typename T>
  inline auto& operator-=(const T& source)
  {
    target_ = bitsOutside() | shiftAndMask(get() - source);
    return *this;
  }
  template <typename T>
  inline auto& operator*=(const T& source)
  {
    target_ = bitsOutside() | shiftAndMask(get() * source);
    return *this;
  }
  template <typename T>
  inline auto& operator/=(const T& source)
  {
    target_ = bitsOutside() | shiftAndMask(get() / source);
    return *this;
  }
  template <typename T>
  inline auto& operator%=(const T& source)
  {
    target_ = bitsOutside() | shiftAndMask(get() % source);
    return *this;
  }
  template <typename T>
  inline auto& operator<<=(const T& source)
  {
    target_ = bitsOutside() | shiftAndMask(get() << source);
    return *this;
  }
  template <typename T>
  inline auto& operator>>=(const T& source)
  {
    target_ = bitsOutside() | shiftAndMask(get() >> source);
    return *this;
  }
  template <typename T>
  inline auto& operator&=(const T& source)
  {
    target_ = bitsOutside() | shiftAndMask(get() & source);
    return *this;
  }
  template <typename T>
  inline auto& operator|=(const T& source)
  {
    target_ = bitsOutside() | shiftAndMask(get() | source);
    return *this;
  }
  template <typename T>
  inline auto& operator^=(const T& source)
  {
    target_ = bitsOutside() | shiftAndMask(get() ^ source);
    return *this;
  }

private:
  inline auto bitsOutside() const { return target_ & ~mask_; }
  inline auto shiftAndMask(Type v) const { return (v << shift_) & mask_; }

  Type& target_;
  Type mask_;
  unsigned shift_;
};

static inline auto bswap_u32(u32 v) -> u32
{
  return __builtin_bswap32(v);
}

}
