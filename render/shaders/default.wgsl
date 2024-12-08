struct vertex_input {
  @location(0) position: vec2f,
  @location(1) uv: vec2f,
};

struct vertex_output {
  @builtin(position) position: vec4f,
  @location(1) uv: vec2f,
};

struct uniform_struct {
  interactive_input: vec2f,
  // TODO: use this
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
