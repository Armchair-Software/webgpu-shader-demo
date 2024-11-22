# WebGPU Emscripten demo 2 for the Armchair Engine

This is part two of a simple proof of concept, a minimal 3D engine written in C++, compiled to WASM with Emscripten.  Running in the browser, rendering with WebGPU.

For the basic initial demo, see https://github.com/Armchair-Software/webgpu-demo.

This is a follow-up, adding GUI rendering with [dear imgui](https://github.com/ocornut/imgui), demonstrating the new emscripten imgui backend.

This also demonstrates how you might set up gamepad input with the above backend.  Plug in a gamepad, joystick, or other controller to test the integration - the cube can be rotated, and the gui can be interacted with.  There is no dependency on GLFW.

![image](https://github.com/user-attachments/assets/7bb8d5bf-f627-4fa0-9bda-a6b5b47c9bbe)

## Live demo
Live demo: https://armchair-software.github.io/webgpu-demo2/

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
