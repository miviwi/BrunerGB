#pragma once

#include <osd/osd.h>

#include <exception>
#include <stdexcept>
#include <string>

namespace brgb {

// Forward declarations
class OSDSurface;
class GLProgram;

class OSDQuadShader {
public:
  struct AddToFrozenShaderError : public std::runtime_error {
    AddToFrozenShaderError() :
      std::runtime_error("attempted to add source to a frozen (i.e. compiled) OSDQuadShader")
    { }
  };

  ~OSDQuadShader();

  auto addSource(const char *src) -> OSDQuadShader&;
  auto addSource(const std::string& src) -> OSDQuadShader&;

  // The first call to this method compiles and
  //   links a GLProgram created from sources
  //   added with prior calls to addSource()
  //  - Calling this method 'freezes' the object
  //    (see comment above the frozen() method
  //    below)
  auto program() -> GLProgram&;

  // Returns true after the first call to program(),
  //   when this happens addSource() can no longer
  //   be called on this OSDQuadShader (it will
  //   throw an AddToFrozenShaderError exception)
  auto frozen() -> bool;

private:
  friend OSDSurface;

  OSDQuadShader() = default;

  auto destroy() -> void;

  // See comment above frozen()
  bool frozen_ = false;

  std::string source_;

  GLProgram *program_ = nullptr;
};

}
