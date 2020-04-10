#include <x11/x11.h>
#include <x11/connection.h>

#include <cassert>

#include <memory>

namespace brdrive {

std::unique_ptr<X11Connection> g_x11;

void x11_init()
{
  g_x11.reset(new X11Connection());
  g_x11->connect();
}

void x11_finalize()
{
  g_x11.reset();
}

auto x11_was_init() -> bool
{
  return g_x11.get();
}

auto x11() -> X11Connection&
{
  assert(g_x11.get() && "x11() can only be called AFTER x11_init()!");

  return *g_x11.get();
}

}
