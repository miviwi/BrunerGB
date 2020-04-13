#pragma once

#include <types.h>

namespace brgb {

// Forward declarations
class SystemBus;

namespace lr35902 {
class Processor;
}

class Gameboy {
public:

private:
  SystemBus *bus_;

  lr35902::Processor *cpu_;
};

}
