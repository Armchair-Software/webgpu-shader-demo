#pragma once

#include <cstdint>

namespace render {

struct alignas(4) indirect_command {
  uint32_t vertex_count{0};
  uint32_t instance_count{0};
  uint32_t first_vertex{0};
  uint32_t first_instance{0};
} __attribute__((__packed__));

struct alignas(4) indirect_indexed_command {
  uint32_t index_count{0};
  uint32_t instance_count{0};
  uint32_t first_index{0};
  uint32_t base_vertex{0};
  uint32_t first_instance{0};
} __attribute__((__packed__));

}
