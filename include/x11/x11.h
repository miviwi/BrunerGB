#pragma once

#include <types.h>

namespace brdrive {

// Forward declaration
class X11Connection;

void x11_init();
void x11_finalize();

// Returns 'true' if x11_init() was previously called
auto x11_was_init() -> bool;

auto x11() -> X11Connection&;

}
