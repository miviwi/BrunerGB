#pragma once

#include <string>
#include <string_view>
#include <vector>
#include <utility>
#include <functional>
#include <type_traits>

namespace brgb::util {

std::string p_fmt(const char *fmt, ...);

template <typename T>
struct FmtForwarder {
  static T&& forward(std::remove_reference_t<T>& t)
  {
    return std::forward<T>(t);
  }
};

template <>
struct FmtForwarder<std::string> {
  static const char *forward(const std::string& s)
  {
    return s.data();
  }
};

template <>
struct FmtForwarder<std::string_view> {
  static const char *forward(const std::string_view& s)
  {
    return s.data();
  }
};

template <>
struct FmtForwarder<std::vector<char>> {
  static const char *forward(const std::vector<char>& s)
  {
    return s.data();
  }
};

// Works just like sprintf, except the '%s' placeholder can accept
//       std::string, std::string_view, std::vector<char>
// directly and do the correct thing (that is - it will call .data()
// on them internally)
template <typename... Args>
std::string fmt(const char *fmt, Args... args)
{
  return p_fmt(fmt, FmtForwarder<Args>::forward(args)...);
}

// calls 'callback' for every wrapped substring of 'line'
//   with the current (0-based) line number as 'line_no'
void linewrap(const std::string& line, size_t wrap_limit,
  std::function<void(const std::string& line, size_t line_no)> callback);

}

