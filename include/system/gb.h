#pragma once

#include <types.h>

namespace brgb {

// Forward declarations
class SystemBus;

namespace sm83 {
class Processor;
}

class Gameboy {
public:

private:
  SystemBus *bus_;

  sm83::Processor *cpu_;
};

}
