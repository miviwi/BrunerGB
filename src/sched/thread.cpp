#include <sched/thread.h>
#include <sched/scheduler.h>
#include <sched/device.h>

// libco
#include <libco/libco.h>

#include <functional>
#include <vector>
#include <unordered_set>

#include <cassert>

namespace brgb {

static thread_local std::unordered_set<Thread::Ptr> p_threads;

auto Thread::create(double frequency, ISchedDevice *device) -> Thread::Ptr
{
  auto thread = new Thread();

  device->frequency(frequency);
  device->clock(0);

  thread->thread_ = co_create(4 * 1024*1024 /* 4 MiB */, &Thread::cothread_trampoline);
  thread->device_ = device;

  auto ptr = Thread::Ptr(thread);

  p_threads.insert(ptr);    // Will be cleaned-up by Thread::cothread_trampoline()

  return ptr;
}

auto Thread::cothread_trampoline() -> void
{
  auto self = Thread::Ptr();
  for(const auto& thread : p_threads) {
    if(co_active() == thread->handle()) {
      self = thread;
      break;
    }
  }

  assert(self && "cothread_trampoline(): failed to find current thread in 'p_threads'!");

  p_threads.erase(self);

  assert(self->sched_ && "Thread ran before it was asigned to a Scheduler!");
  while(true) {
    self->sched_->sync();   // Mark this point as safe for pausing execution

    self->device()->main();
  }
}

Thread::~Thread()
{
  if(!thread_) return;

  co_delete(thread_);
}

auto Thread::handle() -> cothread_t
{
  return thread_;
}

auto Thread::device() -> ISchedDevice *
{
  return device_;
}

}
