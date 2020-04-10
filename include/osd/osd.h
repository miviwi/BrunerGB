#pragma once

#include <types.h>

namespace brdrive {

void osd_init();
void osd_finalize();

auto osd_was_init() -> bool;

}
