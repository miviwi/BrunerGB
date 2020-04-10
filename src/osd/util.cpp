#include <osd/util.h>

namespace brdrive {

auto osd_ortho(float t, float l, float b, float r, float n, float f) -> mat4
{
  float p00 =  2.0f/(r-l), p30 = -(r+l)/(r-l),
        p11 =  2.0f/(t-b), p31 = -(t+b)/(t-b),
        p22 = -2.0f/(f-n), p32 = -(f+n)/(f-n);

  return {
     p00, 0.0f, 0.0f,  p30,
    0.0f,  p11, 0.0f,  p31,
    0.0f, 0.0f,  p22,  p32,
    0.0f, 0.0f, 0.0f, 1.0f,
  };
}

}
