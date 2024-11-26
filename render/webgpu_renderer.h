#pragma once

#include <emscripten/em_types.h>
#include <webgpu/webgpu_cpp.h>
#include "logstorm/logstorm_forward.h"
#include "vectorstorm/vector/vector2.h"
#include "vectorstorm/vector/vector3.h"
#include "indirect.h"
#include "uniforms.h"
#include "triangle_index.h"
#include "vertex.h"

namespace render {

class webgpu_renderer {
  logstorm::manager &logger;

public:
  struct webgpu_data {
    wgpu::Instance instance{wgpu::CreateInstance()};                            // the underlying WebGPU instance
    wgpu::Surface surface;                                                      // the canvas surface for rendering
    wgpu::Adapter adapter;                                                      // WebGPU adapter once it has been acquired
    wgpu::Device device;                                                        // WebGPU device once it has been acquired
    wgpu::Queue queue;                                                          // the queue for this device, once it has been acquired
    wgpu::BindGroupLayout bind_group_layout;                                    // layout for the uniform bind group
    wgpu::RenderPipeline pipeline;                                              // the render pipeline currently in use

    wgpu::SwapChain swapchain;                                                  // the swapchain providing a texture view to render to

    wgpu::Texture depth_texture;                                                // depth buffer
    wgpu::TextureView depth_texture_view;

    wgpu::TextureFormat surface_preferred_format{wgpu::TextureFormat::Undefined}; // preferred texture format for this surface
    static constexpr wgpu::TextureFormat depth_texture_format{wgpu::TextureFormat::Depth24Plus}; // what format to use for the depth texture

  private:
    webgpu_data() = default;
    friend class webgpu_renderer;
  };

  // TODO, rearrange scene content meaningfully
  std::vector<wgpu::RenderBundle> render_bundles;
  wgpu::Buffer vertex_buffer;
  wgpu::Buffer instance_buffer;
  wgpu::Buffer index_buffer;
  wgpu::Buffer uniform_buffer;
  wgpu::Buffer indirect_buffer;

  wgpu::RenderPassDescriptor render_pass_descriptor;

  static unsigned int constexpr grid_count{50};
  static vec3ui constexpr grid_size{grid_count, 10, grid_count};
  static unsigned int constexpr num_instances{grid_size.x * grid_size.y * grid_size.z};

  uniforms uniform_data;
  indirect_indexed_command indirect_data;

  std::vector<vertex> vertex_data{
    {{-1.0f, -1.0f, -1.0f}, { 0.0f, -1.0f,  0.0f}, {1.0f, 0.75f, 0.0f, 1.0f}},  // bottom face normal & colour
    {{+1.0f, -1.0f, -1.0f}, {+1.0f,  0.0f,  0.0f}, {1.0f, 0.75f, 0.0f, 1.0f}},  // right face normal & colour
    {{+1.0f, +1.0f, -1.0f}, { 0.0f,  0.0f, -1.0f}, {1.0f, 0.75f, 0.0f, 1.0f}},  // front face normal & colour
    {{-1.0f, +1.0f, -1.0f}, {-1.0f,  0.0f,  0.0f}, {1.0f, 0.75f, 0.0f, 1.0f}},  // left face normal & colour
    {{-1.0f, -1.0f, +1.0f}, { 0.0f,  0.0f,  0.0f}, {1.0f, 0.75f, 0.0f, 1.0f}},  // normal & colour not used
    {{+1.0f, -1.0f, +1.0f}, { 0.0f,  0.0f,  0.0f}, {1.0f, 0.75f, 0.0f, 1.0f}},  // normal & colour not used
    {{+1.0f, +1.0f, +1.0f}, { 0.0f, +1.0f,  0.0f}, {1.0f, 0.75f, 0.0f, 1.0f}},  // top face normal & colour
    {{-1.0f, +1.0f, +1.0f}, { 0.0f,  0.0f, +1.0f}, {1.0f, 0.75f, 0.0f, 1.0f}},  // back face normal & colour
  };

  std::vector<triangle_index> index_data{
    {0, 1, 5}, {0, 5, 4},                                                       // bottom face (y = -1)
    {1, 6, 5}, {1, 2, 6},                                                       // right face (x = +1)
    {2, 1, 0}, {2, 0, 3},                                                       // front face (z = -1)
    {3, 0, 4}, {3, 4, 7},                                                       // left face (x = -1)
    {6, 3, 7}, {6, 2, 3},                                                       // top face (y = +1)
    {7, 4, 5}, {7, 5, 6},                                                       // back face (z = +1)
  };

private:
  webgpu_data webgpu;

  struct window_data {
    vec2ui viewport_size;                                                       // our idea of the size of the viewport we render to, in real pixels
    float device_pixel_ratio{1.0f};
  } window;

  std::function<void(webgpu_data const&)> postinit_callback;                    // the callback that is called once when init completes (it cannot return normally because of emscripten's loop mechanism)
  std::function<void()> main_loop_callback;                                     // the callback that is called repeatedly for the main loop after init

public:
  webgpu_renderer(logstorm::manager &logger);

  void init(std::function<void(webgpu_data const&)> &&postinit_callback, std::function<void()> &&main_loop_callback);

private:
  void init_swapchain();
  void init_depth_texture();

  void wait_to_configure_loop();
  void configure();

  void update_imgui_size();

  void build_scene();

public:
  void draw(vec2f const& rotation);
};

}
