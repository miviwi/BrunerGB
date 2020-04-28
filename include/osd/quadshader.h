#pragma once

#include <osd/osd.h>

#include <exception>
#include <stdexcept>
#include <string>
#include <tuple>
#include <vector>

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

  struct EntrypointUndefinedError : public std::runtime_error {
    EntrypointUndefinedError() :
      std::runtime_error("attempted to compile an OSDQuadShader without an entrypoint defined!")
    { }
  };

  OSDQuadShader() = default;

  OSDQuadShader(OSDQuadShader&& other);
  OSDQuadShader(const OSDQuadShader&) = delete;
  ~OSDQuadShader();

  auto addSource(const char *src) -> OSDQuadShader&;
  auto addSource(const std::string& src) -> OSDQuadShader&;

  // Adds a uniform sampler2D[len] to the shader's source
  auto addPixmapArray(const char *name, size_t len) -> OSDQuadShader&;

  // Forward declare a function
  auto declFunction(const char *signature) -> OSDQuadShader&;

  // Syntax sugar for separate calls to declFunction()/addSource()
  auto addFunction(const char *signature, const char *src) -> OSDQuadShader&;

  // main() will delegate to the function whose name is specified by the argument
  //   - The function must return a vec4 which will become the shaded fragment's color
  //        TODO: add an assertion for this :)
  auto entrypoint(const char *func_name) -> OSDQuadShader&;

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

  auto destroy() -> void;

  using UniformPixmapArray = std::tuple<std::string /* name */, size_t /* len */>;

  // Appended to via calls to addPixmapArray()
  std::vector<UniformPixmapArray> pixmap_arrays_;

  // Appended to via calls to declFunction()
  std::vector<std::string /* signature */> function_decls_;

  // main() delegates to this function
  std::string entrypoint_;

  // See comment above frozen()
  bool frozen_ = false;

  // GLShader(Fragment) source code for the program
  std::string source_;

  GLProgram *program_ = nullptr;
};

}
