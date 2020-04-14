#include <bus/mappedrange.h>

#include <functional>
#include <regex>
#include <string>
#include <sstream>

#include <cassert>

namespace brgb {

struct AddressRangePart {
  u64 lo, hi;
};

static const auto re_address_range = std::regex(
    "0x[0-9a-fA-F]+-0x[0-9a-fA-F]+(,0x[0-9a-fA-F]+-0x[0-9a-fA-F]+)*",
    std::regex::optimize
);
auto validate_address_range(const char *address_range) -> bool
{
  return std::regex_match(address_range, re_address_range);
}

static const auto re_address_range_part = std::regex(
    "0x([0-9a-fA-F]+)-0x([0-9a-fA-F]+)",
    std::regex::optimize
);
auto parse_address_range_part(const std::string& range) -> AddressRangePart
{
  AddressRangePart result;

  // Extract the lo and hi parts of the range
  auto matches = std::smatch();
  auto was_matched = std::regex_match(range, matches, re_address_range_part);

  assert(was_matched && matches.size() == 3);   // Sanity check

  // Convert the matched strings into integers
  result.lo = std::stoull(matches[1].str(), nullptr, 16);
  result.hi = std::stoull(matches[2].str(), nullptr, 16);

  return result;
}

auto each_range_part(
    const char *address_range, std::function<void(AddressRangePart)> fn
  ) -> void
{
  std::istringstream stream(address_range);
  std::string range_part_str;

  // Split 'address_range' by commas (',')...
  while(std::getline(stream, range_part_str, ',')) {
    auto range_part = parse_address_range_part(range_part_str);

    fn(range_part);
  }
}

auto BusTransactionHandlerSet::base(Address b) -> BusTransactionHandlerSet&
{
  each([=](BusTransactionHandler& h) { h.base(b); });

  return *this;
}

auto BusTransactionHandlerSet::mask(Address m) -> BusTransactionHandlerSet&
{
  each([=](BusTransactionHandler& h) { h.mask(m); });

  return *this;
}

auto BusReadHandlerSet::from_address_range(const char *address_range) -> BusTransactionHandlerSetRef
{
  if(!validate_address_range(address_range))
    throw BusTransactionHandler::InvalidAddressRangeError();

  auto set = new BusReadHandlerSet();

  each_range_part(address_range, [=](AddressRangePart range_part) {
    // Allocate a BusTransactionHandler for this AddressRangPart
    auto handler = new BusReadHandler();

    handler->address_range = address_range;

    handler->lo = range_part.lo;
    handler->hi = range_part.hi;

    // Add the new handler to the result set
    set->handlers_.emplace_back(handler);
  });

  return BusTransactionHandlerSetRef(set);
}

auto BusReadHandlerSet::fn(BusReadHandler::ByteHandler f) -> BusReadHandlerSet&
{
  for(auto h : handlers_) h->fn(f);

  return *this;
}

auto BusReadHandlerSet::fn(BusReadHandler::WordHandler f) -> BusReadHandlerSet&
{
  for(auto h : handlers_) h->fn(f);

  return *this;
}

auto BusReadHandlerSet::eachPtr(
    std::function<void(BusTransactionHandler::Ptr)> f
  ) -> BusTransactionHandlerSet&
{
  for(auto& h : handlers_) f(h);

  return *this;
}

auto BusWriteHandlerSet::from_address_range(const char *address_range) -> BusTransactionHandlerSetRef
{
  if(!validate_address_range(address_range))
    throw BusTransactionHandler::InvalidAddressRangeError();

  auto set = new BusWriteHandlerSet();

  each_range_part(address_range, [=](AddressRangePart range_part) {
    // Allocate a BusTransactionHandler for this AddressRangPart
    auto handler = new BusWriteHandler();

    handler->address_range = address_range;

    handler->lo = range_part.lo;
    handler->hi = range_part.hi;

    // Add the new handler to the result set
    set->handlers_.emplace_back(handler);
  });

  return BusTransactionHandlerSetRef(set);
}

auto BusWriteHandlerSet::fn(BusWriteHandler::ByteHandler f) -> BusWriteHandlerSet&
{
  for(auto h : handlers_) h->fn(f);

  return *this;
}

auto BusWriteHandlerSet::fn(BusWriteHandler::WordHandler f) -> BusWriteHandlerSet&
{
  for(auto h : handlers_) h->fn(f);

  return *this;
}

auto BusWriteHandlerSet::eachPtr(
    std::function<void(BusTransactionHandler::Ptr)> f
  ) -> BusTransactionHandlerSet&
{
  for(auto& h : handlers_) f(h);

  return *this;
}

}
