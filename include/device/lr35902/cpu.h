#pragma once

#include <bus/bus.h>
#include <bus/device.h>

#include <device/lr35902/registers.h>

namespace brgb::lr35902 {

class Processor {
public:

protected:
  Bus<16> *bus_;

private:

  Registers r;
};

}
