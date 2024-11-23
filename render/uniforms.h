#pragma once

#include <array>
#include "vectorstorm/matrix/matrix3.h"
#include "vectorstorm/matrix/matrix4.h"

namespace render {

struct alignas(16) uniforms {
  mat4f view_projection;
  mat3fwgpu normal;
};

}
