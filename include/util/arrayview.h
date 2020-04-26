#pragma once

#include <types.h>

#include <stdexcept>

#include <cassert>

template <typename T>
class ArrayView {
public:
  struct IndexedPastEndError : public std::runtime_error {
    IndexedPastEndError() :
      std::runtime_error("attempted to index the ArrayView past it's end!")
    { }
  };

  ArrayView(T *array, size_t count) :
    begin_(array), end_(array+count)
  { }
  ArrayView(T *begin, T *end) :
    begin_(begin), end_(end)
  { }

  auto size() const -> size_t { return end_-begin_; }

  auto begin() -> T* { return begin_; }
  auto end() -> T* { return end_; }

  auto begin() const -> const T* { return begin_; }
  auto end() const -> const T* { return end_; }

  auto operator[](size_t idx) -> T&
  {
    assertSize(idx);
    return *(begin() + idx);
  }
  auto operator[](size_t idx) const -> const T&
  {
    assertSize(idx);
    return *(begin() + idx);
  }

  auto at(size_t idx) -> T&
  {
    if(idx >= size()) throw IndexedPastEndError();
    return *(begin() + idx);
  }
  auto at(size_t idx) const -> const T&
  {
    if(idx >= size()) throw IndexedPastEndError();
    return *(begin() + idx);
  }

  auto data() -> T* { return begin_; }
  auto data() const -> const T* { return begin_; }

private:
  auto assertSize(size_t idx) -> void
  {
    assert(idx < size() && "ArrayView: attempted to index into the view past it's end!");
  }

  T *begin_;
  T *end_;
};
