#include <gx/extensions.h>

// OpenGL/gl3w
#include <GL/gl3w.h>

#include <unordered_set>
#include <string_view>
#include <utility>

#include <cassert>

namespace brdrive {

// 'g_num_extensions' and 'g_extension' are lazy-initialized
//   by queryExtension(), since the required data can only
//   be queried after acquiring a GLContext and querying
//   for GL function's pointers (which is gx_init()'s
//   responsibility)
thread_local int g_num_extensions = -1;
thread_local std::unordered_set<std::string_view> g_extensions;

auto queryExtension(const char *name) -> bool
{
  assert(gx_was_init() && "gx_init() must be called before this function can be used!");

  [[using gnu: cold]] 
  if(g_num_extensions < 0) {    // Need to initialize the 'g_extensions' set
    glGetIntegerv(GL_NUM_EXTENSIONS, &g_num_extensions);
    assert(g_num_extensions >= 0);

    for(int i = 0; i < g_num_extensions; i++) {
      std::string_view extension = (const char *)glGetStringi(GL_EXTENSIONS, i);
      g_extensions.emplace(extension);
    }
  }

  return g_extensions.find(name) != g_extensions.end();
}

// - 'ext' initially has a value < 0 to indicate it hasn't
//   been initialized yet
// - Otherwise, if 'ext' >= 0 it's value interpreted as a
//   boolean indicates the avilability of the extension
inline auto query_extension_cached(const char *name, int *ext) -> bool
{
  // HOT path - the avilability of the extension has already been cached
  [[using gnu: hot]] if(*ext >= 0) return *ext;

  // Need to query 'g_extensions'
  return *ext = queryExtension(name);
}

namespace extensions_detail {

// Defined here to avoid exposing query_extension_cached() in extensions.h
auto CachedExtensionQuery::operator()() -> bool
{
  return query_extension_cached(extension_, &query_result_);
}

}

#define STRINGIFIED(str) #str

namespace ARB {
#define DEFINE_ARB_ExtensionQuery(name) \
  thread_local extensions_detail::CachedExtensionQuery name("GL_ARB_" STRINGIFIED(name));

DEFINE_ARB_ExtensionQuery(vertex_attrib_binding);
DEFINE_ARB_ExtensionQuery(separate_shader_objects);
DEFINE_ARB_ExtensionQuery(tessellation_shader);
DEFINE_ARB_ExtensionQuery(compute_shader);
DEFINE_ARB_ExtensionQuery(texture_storage);
DEFINE_ARB_ExtensionQuery(buffer_storage);
DEFINE_ARB_ExtensionQuery(direct_state_access);
DEFINE_ARB_ExtensionQuery(texture_filter_anisotropic);

#undef DEFINE_ARB_ExtensionQuery
}

namespace EXT {
#define DEFINE_EXT_ExtensionQuery(name) \
  thread_local extensions_detail::CachedExtensionQuery name("GL_EXT_" STRINGIFIED(name));

DEFINE_EXT_ExtensionQuery(direct_state_access);
DEFINE_EXT_ExtensionQuery(texture_filter_anisotropic);

#undef DEFINE_EXT_ExtensionQuery
}

#undef STRINGIFIED

}
