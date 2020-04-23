#pragma once

#include <locale>
#include <types.h>

#include <memory>

// libco
#include <libco/libco.h>

namespace brgb {

// Forward declarations
class Scheduler;
class ISchedDevice;

class Thread {
public:
  using Id = u32;
  enum : Id {
    InvalidId = ~0u,
  };

  using Ptr = std::shared_ptr<Thread>;

  // Create a new Thread object and setup 'device'
  static auto create(double frequency, ISchedDevice *device) -> Thread::Ptr;

  ~Thread();

  auto handle() -> cothread_t;

  // Returns 'true' when the thread has been initialized
  //   i.e. when it's handle() != nullptr
  operator bool() const { return thread_; }

  auto device() -> ISchedDevice *;

private:
  friend Scheduler;

  // Entry-point passed to cothread_create()
  static auto cothread_trampoline() -> void;

  // Force using Thred::create() to construct Thread objects
  Thread() = default;

  Id id_ = InvalidId;
  cothread_t thread_ = nullptr;

  // The object is NOT managed by the Thread and care must be
  //   taken so it's resources are freed elsewhere
  ISchedDevice *device_ = nullptr;
};

}
