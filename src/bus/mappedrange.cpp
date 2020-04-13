#include <bus/mappedrange.h>

#include <regex>
#include <string>
#include <sstream>

namespace brgb {

static const auto re_address_range = std::regex(
    "0x[0-9a-fA-F]+-0x[0-9a-fA-F]+(,0x[0-9a-fA-F]+-0x[0-9a-fA-F]+)*",
    std::regex::optimize
);
auto validate_address_range(const char *address_range) -> bool
{
  return std::regex_match(address_range, re_address_range);
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
  if(!validate_address_range(address_range)) throw BusTransactionHandler::InvalidAddressRangeError();

  auto set = new BusReadHandlerSet();

  // TODO: populate 'set'

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
  if(!validate_address_range(address_range)) throw BusTransactionHandler::InvalidAddressRangeError();

  auto set = new BusWriteHandlerSet();

  std::istringstream stream(address_range);
  std::string range;

  while(std::getline(stream, range, ',')) {
  }


  // TODO: populate 'set'

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
