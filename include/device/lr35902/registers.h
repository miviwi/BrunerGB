#pragma once

#include <util/natural.h>
#include <util/integer.h>
#include <util/bit.h>

namespace brgb::lr35902 {

class Registers {
public:
  Natural<16> af;
  Natural<16> bc;
  Natural<16> de;
  Natural<16> hl;

  Natural<16> sp;
  Natural<16> pc;

  // AF
  BitRange<16, 0, 7> a;
  BitRange<16, 8, 15> f;

  // BC
  BitRange<16, 0, 7> b;
  BitRange<16, 8, 15> c;

  // DE
  BitRange<16, 0, 7> d;
  BitRange<16, 8, 15> e;

  // HL
  BitRange<16, 0, 7> h;
  BitRange<16, 8, 15> l;

  /*
  struct {
    Bit<16, 8+4> c;
    Bit<16, 8+5> h;
    Bit<16, 8+6> n;
    Bit<16, 8+7> z;
  } flags;
  */

  Registers() :
    a(af.ptr()), f(af.ptr()),
    b(bc.ptr()), c(bc.ptr()),
    d(de.ptr()), e(de.ptr()),
    h(hl.ptr()), l(hl.ptr())
  { }

private:
};

}
