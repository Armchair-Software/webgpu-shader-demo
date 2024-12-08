# WebGPU Emscripten shader demo for the Armchair Engine

A simple WGSL shader test program running a fullscreen quad, written in C++, compiled to WASM with Emscripten.  Running in the browser, rendering with WebGPU.

For the previous demos, see:
- https://github.com/Armchair-Software/webgpu-demo
- https://github.com/Armchair-Software/webgpu-demo2
- https://github.com/Armchair-Software/boids-webgpu-demo

This demo just renders a simple full-screen quad, useful for testing shaders.  There is a minimal integrated editor for the shader code.

![image](https://github.com/user-attachments/assets/7b2b1e46-3ac0-411b-9047-615a2c7305c8)

## Live demo
Live demo: https://armchair-software.github.io/webgpu-shader-demo/

This requires Firefox Nightly, or a recent version of Chrome or Chromium, with webgpu and Vulkan support explicitly enabled.

## Dependencies
- [Emscripten](https://emscripten.org/)
- CMake
- [VectorStorm](https://github.com/Armchair-Software/vectorstorm) (included)
- [LogStorm](https://github.com/VoxelStorm-Ltd/logstorm) (included)
- [magic_enum](https://github.com/Neargye/magic_enum) (included)
- [dear imgui](https://github.com/ocornut/imgui) with the proposed `imgui_impl_emscripten` backend (included)

## Building
The easiest way to assemble everything (including in-tree shader resource assembly) is to use the included build script:
```sh
./build.sh
```

To launch a local server and bring up a browser:
```sh
./run.sh
```

For manual builds with CMake, and to adjust how the example is run locally, inspect the `build.sh` and `run.sh` scripts.
