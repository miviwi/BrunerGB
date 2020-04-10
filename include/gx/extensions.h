#pragma once

#include <gx/gx.h>

namespace brdrive {
// 'name' MUST be the WHOLE extension name string i.e.:
//            queryExtension("GL_ARB_buffer_storage")
//     *NOT*  queryExtension("ARB_buffer_storage")
//
// NOTE: gx_init() MUST be called (and succeed)
//       before this function can be used!
auto queryExtension(const char *name) -> bool;

// Private classes, enums, functions, variables etc.
namespace extensions_detail {

// Usage: define an instance of the struct object passing
//        the WHOLE extension name string i.e.:
//            CachedExtensionQuery("GL_ARB_buffer_storage")
//     *NOT*  CachedExtensionQuery("ARB_buffer_storage")
//        after that apply the call operator to the object
//        to query the avilability of the extension (or
//        use operator bool() either via an explicit or
//        a contextual cast)
struct CachedExtensionQuery {
  constexpr CachedExtensionQuery(const char *extension) :
    extension_(extension), query_result_(-1)
  { }

  auto operator()() -> bool;

  operator bool() { return (*this)(); }

private:
  const char * const extension_;
  int query_result_;
};

}

namespace ARB {
extern thread_local extensions_detail::CachedExtensionQuery vertex_attrib_binding;
extern thread_local extensions_detail::CachedExtensionQuery separate_shader_objects;
extern thread_local extensions_detail::CachedExtensionQuery tessellation_shader;
extern thread_local extensions_detail::CachedExtensionQuery compute_shader;
extern thread_local extensions_detail::CachedExtensionQuery texture_storage;
extern thread_local extensions_detail::CachedExtensionQuery buffer_storage;
extern thread_local extensions_detail::CachedExtensionQuery direct_state_access;
extern thread_local extensions_detail::CachedExtensionQuery texture_filter_anisotropic;
}

namespace EXT {
extern thread_local extensions_detail::CachedExtensionQuery direct_state_access;
extern thread_local extensions_detail::CachedExtensionQuery texture_filter_anisotropic;
}

}
