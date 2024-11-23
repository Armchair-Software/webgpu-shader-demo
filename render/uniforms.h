#pragma once

#include <array>
#include "vectorstorm/matrix/matrix3.h"

namespace render {

struct alignas(16) uniforms {
  std::array<mat4f, 30 * 30> model_view_projection_matrix;
  mat3fwgpu normal_matrix;
};

}
