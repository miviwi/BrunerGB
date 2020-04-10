#pragma once

#include <gx/gx.h>

#include <exception>
#include <stdexcept>
#include <string>

namespace brdrive {

class GLFence {
public:
  enum : u64 {
    TimeoutInfinite = ~0ull,
  };

  enum WaitStatus {
    WaitStatusInvalid,

    WaitConditionSatisfied,
    WaitTimeoutExpired,
  };

  struct WaitError : public std::runtime_error {
    WaitError() :
      std::runtime_error("failed to wait on the fence!")
    { }
  };

  GLFence();
  GLFence(const GLFence&) = delete;
  GLFence(GLFence&& other);
  ~GLFence();

  auto operator=(GLFence&& other) -> GLFence&;

  auto fence() -> GLFence&;

  // Causes the program's execution to halt until
  //   either the fence is signaled or the timeout
  //   expires
  auto block(u64 timeout = TimeoutInfinite) -> WaitStatus;
  // Causes the driver to wait until the fence is
  //   either signaled or the timeout expires before
  //   issuing any commands to the GPU
  auto sync(u64 timeout = TimeoutInfinite) -> GLFence&;

  auto signaled() -> bool;

  auto label() const -> const char *;
  auto label(const char *name) -> GLFence&;

private:
  void /* GLsync */ *sync_;
  bool flushed_;

#if !defined(NDEBUG)
  std::string label_;
#endif
};

}
