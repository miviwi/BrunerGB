#pragma once

#include <types.h>

namespace brgb {

void osd_init();
void osd_finalize();

auto osd_was_init() -> bool;

}
