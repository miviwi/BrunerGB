#include <bus/mappedrange.h>

namespace brgb {

auto BusTransactionHandlerSet::base(Address b) -> BusTransactionHandlerSet&
{
  each([&](BusTransactionHandler& h) { h.base(b); });

  return *this;
}

auto BusTransactionHandlerSet::mask(Address m) -> BusTransactionHandlerSet&
{
  each([&](BusTransactionHandler& h) { h.mask(m); });

  return *this;
}

auto BusReadHandlerSet::from_address_range(const char *address_range) -> BusTransactionHandlerSetRef
{
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
  auto set = new BusWriteHandlerSet();

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
