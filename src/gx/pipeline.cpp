#include <gx/pipeline.h>
#include <gx/vertex.h>
#include <gx/program.h>

// OpenGL/gl3w
#include <GL/gl3w.h>

#include <cstdio>
#include <cstring>

#include <algorithm>

namespace brdrive {

GLPipeline::GLPipeline()
{
  std::fill(state_structs_.begin(), state_structs_.end(), std::monostate());

  state_structs_ = {
    Viewport { 0, 0, 1280, 720 },
    VertexInput { 0, GLType::u16 },
    InputAssembly { GLPrimitive::TriangleFan, 0xFFFF },
  };

  StateStructArray other_state;
  std::fill(other_state.begin(), other_state.end(), std::monostate());

  other_state = {
    Viewport { 0, 0, 1280, 576 },
    VertexInput { 0, GLType::u16 },
    InputAssembly { GLPrimitive::TriangleFan, 0xFFFF },
    Blend { 1 },
  };


  diff(other_state);



}

auto GLPipeline::diff(const StateStructArray& other) -> StateStructArray
{
  auto order_state_structs = [](const StateStructArray& array) -> StateStructArray {
    StateStructArray ordered;

    std::fill(ordered.begin(), ordered.end(), std::monostate());
    for(const auto& st : array) {
      if(std::holds_alternative<std::monostate>(st)) continue;

      ordered[st.index()-1] = st;
    }

    return ordered;
  };

  auto print_state = [this](const auto& state) {
    for(const auto& st : state) {
      if(std::holds_alternative<std::monostate>(st)) continue;

      printf("    %zu,\n", st.index());
    }
    puts("");
  };

  puts("Before sort (self):");
  print_state(state_structs_);

  puts("Before sort (other):");
  print_state(other);

  StateStructArray ordered = order_state_structs(state_structs_);
  StateStructArray other_ordered = order_state_structs(other);

  puts("After sort (self):");
  print_state(ordered);

  puts("After sort (other):");
  print_state(other_ordered);

  StateStructArray difference;
  std::fill(difference.begin(), difference.end(), std::monostate());

  for(size_t i = 0; i < difference.size(); i++) {
    const auto& a = ordered[i];
    const auto& b = other_ordered[i];

    const void *a_ptr = &a;
    const void *b_ptr = &b;

    auto result = memcmp(a_ptr, b_ptr, sizeof(StateStruct));

    // The structs are equal - so move on
    if(!result) continue;

    // Otherwise - add 'b' to the difference
    difference[i] = b;
  }

  puts("difference:");
  print_state(difference);

  return difference;
}

}
