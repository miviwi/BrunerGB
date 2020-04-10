#include <gx/fence.h>

// OpenGL/gl3w
#include <GL/gl3w.h>

#include <cassert>

#include <algorithm>
#include <utility>

namespace brdrive {

GLFence::GLFence() :
  sync_(nullptr),
  flushed_(false)
{
}

GLFence::GLFence(GLFence&& other) :
  GLFence()
{
  std::swap(sync_, other.sync_);
  std::swap(flushed_, other.flushed_);

#if !defined(NDEBUG)
  std::swap(label_, other.label_);
#endif
}

GLFence::~GLFence()
{
  if(!sync_) return;

  glDeleteSync((GLsync)sync_);
}

auto GLFence::operator=(GLFence&& other) -> GLFence&
{
  this->~GLFence();
  sync_ = nullptr; flushed_ = false;

  std::swap(sync_, other.sync_);
  std::swap(flushed_, other.flushed_);

#if !defined(NDEBUG)
  std::swap(label_, other.label_);
#endif

  return *this;
}

auto GLFence::fence() -> GLFence&
{
  sync_ = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);

  return *this;
}

auto GLFence::block(u64 timeout) -> WaitStatus
{
  assert(sync_ && "attempted to block() on a null fence!");

  auto flags  = !flushed_ ? GL_SYNC_FLUSH_COMMANDS_BIT : 0;
  auto result = glClientWaitSync((GLsync)sync_, flags, timeout);

  switch(result) {
  case GL_ALREADY_SIGNALED:
  case GL_CONDITION_SATISFIED:
    return WaitConditionSatisfied;

  case GL_TIMEOUT_EXPIRED:
    return WaitTimeoutExpired;

  case GL_WAIT_FAILED:
    throw WaitError();
  }

  return WaitStatusInvalid;   // Unreachable
}

auto GLFence::sync(u64 timeout) -> GLFence&
{
  assert(sync_ && "attempted to sync() on a null fence!");

  glWaitSync((GLsync)sync_, 0, timeout);
  if(flushed_) return *this;      // Fence was already flushed to the GPU's
                                  //   command queue before
  glFlush();
  flushed_ = true;

  return *this;
}

auto GLFence::signaled() -> bool
{
  // A null fence is always considered unsignaled
  if(!sync_) return false;

  int result = -1;
  glGetSynciv((GLsync)sync_, GL_SYNC_STATUS, sizeof(int), nullptr, &result);

  assert(glGetError() == GL_NO_ERROR);

  switch(result) {
  case GL_SIGNALED:   return true;
  case GL_UNSIGNALED: return false;
  }

  return false;   // Unreachable
}

auto GLFence::label() const -> const char *
{
#if !defined(NDEBUG)
  return label_.data();
#else
  return "";
#endif
}

auto GLFence::label(const char *name) -> GLFence&
{
#if !defined(NDEBUG)
  assert(sync_);

  glObjectPtrLabel(sync_, -1, name);
  label_ = name;
#endif

  return *this;
}

}
