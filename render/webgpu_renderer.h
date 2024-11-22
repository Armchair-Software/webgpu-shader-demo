#pragma once

#include <emscripten/em_types.h>
#include <webgpu/webgpu_cpp.h>
#include "logstorm/logstorm_forward.h"
#include "vectorstorm/vector/vector2.h"

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

public:
  void draw();
};

}
