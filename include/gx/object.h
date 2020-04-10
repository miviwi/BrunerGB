#pragma once

#include <gx/gx.h>

#include <string>

namespace brdrive {

class GLObject {
public:
  GLObject(const GLObject&) = delete;
  GLObject(GLObject&& other);
  virtual ~GLObject();

  auto operator=(GLObject&& other) -> GLObject&;

  auto id() const -> GLId;

  auto label() const -> const char *;
  auto label(const char *label) -> GLObject&;

  auto destroy() -> GLObject&;

protected:
  GLObject(GLEnum ns);

  auto swap(GLObject& other) -> GLObject&;

  // Can be called when 'id_' == GLNullId
  virtual auto doDestroy() -> GLObject& = 0;

  // OpenGL name of the object
  GLId id_;

private:
  GLObject();

  // One of GL_BUFFER, GL_SHADER, GL_PROGRAM, GL_VERTEX_ARRAY, GL_QUERY,
  //   GL_TRANSFORM_FEEDBACK, GL_SAMPLER, GL_TEXTURE, GL_RENDERBUFFER, GL_FRAMEBUFFER
  GLEnum namespace_;

#if !defined(NDEBUG)
  std::string label_;
#endif
};

}
