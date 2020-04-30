#pragma once

#include <types.h>

#include <type_traits>
#include <limits>
#include <utility>
#include <memory>
#include <stdexcept>
#include <functional>
#include <vector>
#include <optional>

#include <cassert>

namespace brgb {

// Forward declarations
class DeviceMemoryMap;

class BusTransactionHandlerSetRef;

class BusTransactionHandler {
public:
  using Address = u64;

  enum : Address {
    AddressRangeInvalid = (Address)~0ull,
  };

  using Ptr = std::shared_ptr<BusTransactionHandler>;

  struct InvalidAddressRangeError : public std::runtime_error {
    InvalidAddressRangeError() :
      std::runtime_error("The specified address range is invalid!")
    { }
  };

  const char *address_range = "<invalid>";   // For debug purposes

  // Start and end of the handled address range
  //   - 'hi' (i.e. the end of the range) is inclusive
  Address lo, hi;


  auto mask() const -> Address { return mask_; }
  auto base() const -> Address { return base_; }

  auto mask(Address m) -> BusTransactionHandler& { return setMask(m); }
  auto base(Address b) -> BusTransactionHandler& { return setBase(b); }

  auto maskAndOffsetAddress(Address addr) const -> Address
  {
    return (addr & mask_) - base_;
  }

protected:
  BusTransactionHandler() = default;  // Declared as protected to force
                                      //   BusTransactionHandler to be
                                      //   inherited from

  template <typename Self = BusTransactionHandler>
  auto setMask(Address m) -> Self&
  {
    mask_ = m;

    return *(Self *)this;
  }

  template <typename Self = BusTransactionHandler>
  auto setBase(Address b) -> Self&
  {
    base_ = b;

    return *(Self *)this;
  }

  Address mask_ = std::numeric_limits<Address>::max(); // Must be applied to addresses before
                                                       //   passing them into a handler

  Address base_ = (Address)0x0; // Must be subtracted from addresses
                                //   after applying the mask before
                                //   passing them into a handler
};

class BusReadHandler final : public BusTransactionHandler {
public:
  using ByteHandler = std::function<u8(Address)>;
  using WordHandler = std::function<u16(Address)>;

  template <typename HandlerAddress, typename HandlerFnType>
  static auto for_u8_with_addr_width(HandlerFnType handler) -> ByteHandler
  {
    // Ensure 'HandlerAddress' has one of the allowed types...
    static_assert(std::is_unsigned_v<HandlerAddress>,
        "The 'HandlerAddress' must be an unsigned integer type");

    //  ...and that 'HandlerFnType' is a callable which matches
    //   the signature for a read handler:
    //       u8(HandlerAddress addr),

    static_assert(
        //   BusReadHandler::ByteHandler
        std::is_invocable_r_v<u8, HandlerFnType, HandlerAddress>,
        "'HandlerFnType' has an incompatible call signature!"
    );

    return ByteHandler([=](Address addr) -> u8 {
        return handler((HandlerAddress)addr);
    });
  }

  template <typename HandlerAddress, typename HandlerFnType>
  static auto for_u16_with_addr_width(HandlerFnType handler) -> WordHandler
  {
    // Ensure 'HandlerAddress' has one of the allowed types...
    static_assert(std::is_unsigned_v<HandlerAddress>,
        "The 'HandlerAddress' must be an unsigned integer type");

    //  ...and that 'HandlerFnType' is a callable which matches
    //   the signature for a read handler:
    //       u16(HandlerAddress addr),

    static_assert(
        //   BusReadHandler::WordHandler
        std::is_invocable_r_v<u16, HandlerFnType, HandlerAddress>,
        "'HandlerFnType' has an incompatible call signature!"
    );

    return WordHandler([=](Address addr) -> u8 {
        return handler((HandlerAddress)addr);
    });
  }

  BusReadHandler() = default;

  auto mask() const -> Address { return mask_; }
  auto base() const -> Address { return base_; }

  auto mask(Address m) -> BusReadHandler& { return setMask<BusReadHandler>(m); }
  auto base(Address b) -> BusReadHandler& { return setBase<BusReadHandler>(b); }

  auto fn(ByteHandler byte) -> BusReadHandler&
  {
    byte_.emplace(byte);

    return *this;
  }

  auto fn(WordHandler word) -> BusReadHandler&
  {
    word_.emplace(word);

    return *this;
  }

  auto readByte() const -> const ByteHandler& { return byte_.value(); }
  auto readWord() const -> const WordHandler& { return word_.value(); }

private:
  std::optional<ByteHandler> byte_;
  std::optional<WordHandler> word_;
};

class BusWriteHandler final : public BusTransactionHandler {
public:
  using ByteHandler = std::function<void(Address, u8 /* data */)>;
  using WordHandler = std::function<void(Address, u16 /* data */)>;

  template <typename HandlerAddress, typename HandlerFnType>
  static auto for_u8_with_addr_width(HandlerFnType handler) -> ByteHandler
  {
    // Ensure 'HandlerAddress' has one of the allowed types...
    static_assert(std::is_unsigned_v<HandlerAddress>,
        "The 'HandlerAddress' must be an unsigned integer type");

    //  ...and that 'HandlerFnType' is a callable which matches
    //   the signature for a write handler:
    //       void(HandlerAddress addr, u8 data)

    static_assert(
        //   BusWriteHandler::ByteHandler
        std::is_invocable_r_v<void, HandlerFnType, HandlerAddress, u8>,
        "'HandlerFnType' has an incompatible call signature!"
    );

    return ByteHandler([=](Address addr, u8 data) -> void {
        return handler((HandlerAddress)addr, data);
    });
  }

  template <typename HandlerAddress, typename HandlerFnType>
  static auto for_u16_with_addr_width(HandlerFnType handler) -> WordHandler
  {
    // Ensure 'HandlerAddress' has one of the allowed types...
    static_assert(std::is_unsigned_v<HandlerAddress>,
        "The 'HandlerAddress' must be an unsigned integer type");

    //  ...and that 'HandlerFnType' is a callable which matches
    //   the signature for a write handler:
    //       void(HandlerAddress addr, u16 data)

    static_assert(
        //   BusWriteHandler::WordHandler
        std::is_invocable_r_v<void, HandlerFnType, HandlerAddress, u16>,
        "'HandlerFnType' has an incompatible call signature!"
    );

    return WordHandler([=](Address addr, u16 data) -> void {
        return handler((HandlerAddress)addr, data);
    });
  }

  BusWriteHandler() = default;

  auto mask() const -> Address { return mask_; }
  auto base() const -> Address { return base_; }

  auto mask(Address m) -> BusWriteHandler& { return setMask<BusWriteHandler>(m); }
  auto base(Address b) -> BusWriteHandler& { return setBase<BusWriteHandler>(b); }

  auto fn(ByteHandler byte) -> BusWriteHandler&
  {
    byte_.emplace(byte);

    return *this;
  }

  auto fn(WordHandler word) -> BusWriteHandler&
  {
    word_.emplace(word);

    return *this;
  }

  auto writeByte() const -> const ByteHandler& { return byte_.value(); }
  auto writeWord() const -> const WordHandler& { return word_.value(); }

private:
  std::optional<ByteHandler> byte_;
  std::optional<WordHandler> word_;
};

class BusTransactionHandlerSet {
public:
  using Address = BusTransactionHandler::Address;

  // Call base() for each BusReadHandler in this set
  auto base(Address b) -> BusTransactionHandlerSet&;

  // Call mask() for each BusReadHandler in this set
  auto mask(Address m) -> BusTransactionHandlerSet&;

  virtual auto eachPtr(
      std::function<void(BusTransactionHandler::Ptr)> fn
    ) -> BusTransactionHandlerSet& = 0;

  auto each(std::function<void(BusTransactionHandler&)> fn) -> BusTransactionHandlerSet&
  {
    eachPtr([&](auto ptr) { fn(*ptr); });

    return *this;
  }

protected:
  BusTransactionHandlerSet() = default;
};

class BusReadHandlerSet final : public BusTransactionHandlerSet {
public:
  static auto from_address_range(const char *address_range) -> BusTransactionHandlerSetRef;

  // Call fn() for each BusReadHandler in this set
  auto fn(BusReadHandler::ByteHandler f) -> BusReadHandlerSet&;
  auto fn(BusReadHandler::WordHandler f) -> BusReadHandlerSet&;

protected:
  virtual auto eachPtr(
      std::function<void(BusTransactionHandler::Ptr)> fn
    ) -> BusTransactionHandlerSet& final;

private:
  friend BusTransactionHandlerSet;
  BusReadHandlerSet() = default;

  std::vector<std::shared_ptr<BusReadHandler>> handlers_;
};

class BusWriteHandlerSet final : public BusTransactionHandlerSet {
public:
  static auto from_address_range(const char *address_range) -> BusTransactionHandlerSetRef;

  // Call fn() for each BusWriteHandler in this set
  auto fn(BusWriteHandler::ByteHandler f) -> BusWriteHandlerSet&;
  auto fn(BusWriteHandler::WordHandler f) -> BusWriteHandlerSet&;

protected:
  virtual auto eachPtr(
      std::function<void(BusTransactionHandler::Ptr)> fn
    ) -> BusTransactionHandlerSet& final;

private:
  friend BusTransactionHandlerSet;
  BusWriteHandlerSet() = default;

  std::vector<std::shared_ptr<BusWriteHandler>> handlers_;
};

class BusTransactionHandlerSetRef {
public:
  BusTransactionHandlerSetRef(BusTransactionHandlerSet *set) :
    ref_(set)
  { }
  BusTransactionHandlerSetRef(BusTransactionHandlerSetRef&& other) :
    ref_()
  {
    ref_.swap(other.ref_);
  }

  template <typename T = BusTransactionHandlerSet>
  auto get() -> T&
  {
    static_assert(
        std::is_base_of_v<BusTransactionHandlerSet, T>,
        "T must be derived from BusTransactionHandlerSet!"
    );

    return *(T *)ref_.get();
  }

private:
  std::unique_ptr<BusTransactionHandlerSet> ref_;
};

}
