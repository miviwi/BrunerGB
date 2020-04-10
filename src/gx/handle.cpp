#include <gx/handle.h>
#include <gx/vertex.h>

namespace brdrive {

GLVertexArrayHandle::GLVertexArrayHandle() :
  ptr_(nullptr)
{
}

GLVertexArrayHandle::GLVertexArrayHandle(ConstructorFriendKey, GLVertexArray *ptr) :
  ptr_(ptr)
{
}

GLVertexArrayHandle::GLVertexArrayHandle(GLVertexArrayHandle&& other) :
  GLVertexArrayHandle()
{
  std::swap(ptr_, other.ptr_);
}

GLVertexArrayHandle::~GLVertexArrayHandle()
{
  destroy();
}

auto GLVertexArrayHandle::operator=(GLVertexArrayHandle&& other) -> GLVertexArrayHandle&
{
  this->~GLVertexArrayHandle();
  ptr_ = nullptr;

  std::swap(ptr_, other.ptr_);

  return *this;
}

auto GLVertexArrayHandle::get() const -> const GLVertexArray*
{
  return ptr_;
}

auto GLVertexArrayHandle::get() -> GLVertexArray*
{
  return ptr_;
}

auto GLVertexArrayHandle::destroy() -> GLVertexArrayHandle&
{
  if(!ptr_) return *this;      // Make sure not to call a destructor on a null object

  ptr_->~GLVertexArray();
  free(ptr_);
  ptr_ = nullptr;

  return *this;
}

}

namespace brdrive::vertex_format_detail {

auto GLVertexArray_to_ptr(GLVertexArray&& array) -> GLVertexArrayHandle
{
  auto array_ptr = (GLVertexArray *)malloc(sizeof(GLVertexArray));
  new(array_ptr) GLVertexArray(std::move(array));

  return GLVertexArrayHandle(GLVertexArrayHandle::ConstructorFriendKey(), array_ptr);
}

}


