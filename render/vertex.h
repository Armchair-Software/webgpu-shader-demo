#pragma once

#include "vectorstorm/vector/vector2.h"

namespace render {

struct vertex {
  vec2f position;
  vec2f uv;
};
static_assert(sizeof(vertex) == sizeof(vertex::position) + sizeof(vertex::uv)); // make sure the struct is packed

}
