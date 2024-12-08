#pragma once

// This file is automatically generated from render/shaders/default.wgsl by ./compile_resource_to_raw_string.sh

namespace render::shaders {

inline constexpr char const *default_wgsl{R"9e667cb64785e0c8(struct vertex_input {
  @location(0) position: vec2f,
  @location(1) uv: vec2f,
};
struct vertex_output {
  @builtin(position) position: vec4f,
  @location(1) uv: vec2f,
};
struct uniform_struct {
  interactive_input: vec2f,
};
@group(0) @binding(0) var<uniform> uniforms: uniform_struct;
@vertex
fn vs_main(in: vertex_input) -> vertex_output {
  var out: vertex_output;
  out.position = vec4f(in.position, 0.0, 1.0);
  out.uv = in.uv;
  return out;
}
@fragment
fn fs_main(in: vertex_output) -> @location(0) vec4f {
  return vec4f(in.uv.x, in.uv.y, 0.0f, 1.0f);
}
)9e667cb64785e0c8"};

} // namespace render::shaders
