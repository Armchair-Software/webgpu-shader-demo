#pragma once

#include <array>
#include "vectorstorm/matrix/matrix3.h"

struct alignas(16) uniforms {
  std::array<mat4f, 16 * 16> model_view_projection_matrix;
  mat3fwgpu normal_matrix;
};
