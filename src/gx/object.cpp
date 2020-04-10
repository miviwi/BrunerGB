#include <gx/object.h>

// OpenGL/gl3w
#include <GL/gl3w.h>

#include <cassert>

#include <utility>

namespace brdrive {

GLObject::GLObject() :
  GLObject(GL_INVALID_ENUM)
{
}

GLObject::GLObject(GLEnum ns) :
  namespace_(ns),
  id_(GLNullId)
{
}

GLObject::GLObject(GLObject&& other) :
  GLObject()
{
  other.swap(*this);
}

GLObject::~GLObject()
{
}

auto GLObject::operator=(GLObject&& other) -> GLObject&
{
  destroy();
  other.swap(*this);

  return *this;
}

auto GLObject::swap(GLObject& other) -> GLObject&
{
  std::swap(namespace_, other.namespace_);
  std::swap(id_, other.id_);

#if !defined(NDEBUG)
  std::swap(label_, other.label_);
#endif

  return *this;
}

auto GLObject::id() const -> GLId
{
  return id_;
}

auto GLObject::label() const -> const char *
{
#if !defined(NDEBUG)
  assert(id_ != GLNullId);

  return label_.data();
#else
  return "";
#endif
}

auto GLObject::label(const char *label) -> GLObject&
{
#if !defined(NDEBUG)
  assert(id_ != GLNullId);

  glObjectLabel(namespace_, id_, -1, label);
  assert(glGetError() == GL_NO_ERROR);
#endif

  return *this;
}

auto GLObject::destroy() -> GLObject&
{
  doDestroy();

  // Reset all member variables
  namespace_ = GL_INVALID_ENUM;
  id_ = GLNullId;

#if !defined(NDEBUG)
  label_.clear();
#endif

  return *this;
}

}
