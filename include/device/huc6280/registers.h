#pragma once

#include <util/natural.h>
#include <util/integer.h>
#include <util/bit.h>

namespace brgb::huc6280 {

class Registers {
public:
  Natural<8> a;
  Natural<8> x, y;

  Natural<8> s;   // Stack pointer

  Natural<16> pc;

  Natural<8> mpr[8];
  Natural<8> mpl;    // MPR latch

  Natural<8> p;   // Processor status/flags

  Natural<8> cs;  // Code speed (3=fast/7.159MHz, 12=slow/1.789MHz)

  struct Flags {
    Bit<8, 0> c;  // Carry
    Bit<8, 1> z;  // Zero
    Bit<8, 2> i;  // Interrupt disable
    Bit<8, 3> d;  // Decimal moode
    Bit<8, 4> b;  // Break/BRK
    Bit<8, 5> t;  // T flag (memory-to-memory op)
    Bit<8, 6> v;  // Overflow
    Bit<8, 7> n;  // Negative

    Flags(Natural<8>& p) :
      c(p.ptr()), z(p.ptr()), i(p.ptr()), d(p.ptr()),
      b(p.ptr()), t(p.ptr()), v(p.ptr()), n(p.ptr())
    { }
  } flags;

  Registers() :
    flags(p)
  { }
};

}
