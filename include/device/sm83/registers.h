#pragma once

#include <util/natural.h>
#include <util/integer.h>
#include <util/bit.h>

namespace brgb::sm83 {

class Registers {
public:
  Natural<16> af;
  Natural<16> bc;
  Natural<16> de;
  Natural<16> hl;

  Natural<16> sp;
  Natural<16> pc;

  // AF
  BitRange<16, 8, 15> a;
  BitRange<16, 0, 7> f;

  // BC
  BitRange<16, 8, 15> b;
  BitRange<16, 0, 7> c;

  // DE
  BitRange<16, 8, 15> d;
  BitRange<16, 0, 7> e;

  // HL
  BitRange<16, 8, 15> h;
  BitRange<16, 0, 7> l;

  struct Flags {
    Bit<16, 4> c;
    Bit<16, 5> h;
    Bit<16, 6> n;
    Bit<16, 7> z;

    Flags(Natural<16>& af) :
      c(af.ptr()), h(af.ptr()), n(af.ptr()), z(af.ptr())
    { }
  } flags;

  Registers() :
    a(af.ptr()), f(af.ptr()),
    b(bc.ptr()), c(bc.ptr()),
    d(de.ptr()), e(de.ptr()),
    h(hl.ptr()), l(hl.ptr()),
    flags(af)
  { }

private:
};

}
