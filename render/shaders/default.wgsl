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

struct mandelbrot_out {
  unbounded: f32,
  bounded: f32,
};

fn mandelbrot(c: vec2f) -> mandelbrot_out {
  var out: mandelbrot_out;
  var z: vec2f = vec2(0.0, 0.0);
  var dist: f32 = 0.0;
  for (var i: u32 = 0; i < 64u; i = i + 1u) {
    z = vec2(
      z.x * z.x - z.y * z.y,
      2.0 * z.x * z.y
    ) + c;

    if (dot(z, z) > 128.0) {
      out.unbounded = f32(i) - log2(log2(dot(z, z)));
      return out;
    }

    dist = dist + distance(c, z);
    dist = dist / 2.0;
  }
  out.bounded = log(dist + 1.5);
  return out;
}

@vertex
fn vs_main(in: vertex_input) -> vertex_output {
  var out: vertex_output;
  out.position = vec4f(in.position, 0.0, 1.0);
  out.uv = in.uv;
  return out;
}

@fragment
fn fs_main(in: vertex_output) -> @location(0) vec4f {
  let c = vec2f(
    in.uv.x * 3.5 - 2.5,
    in.uv.y * 2.0 - 1.0
  );

  let values = mandelbrot(c);

  let color = vec3f(
    values.unbounded / 16.0 + values.bounded,
    0.5 + (values.unbounded / 128.0) + values.bounded / 4.0,
    0.5 - (values.unbounded / 64.0),
  );
  return vec4f(color, 1.0);
}
