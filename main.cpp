#include <iostream>
#include <functional>
#include <map>
#include <emscripten/html5.h>
#include <imgui/imgui_impl_wgpu.h>
#include "logstorm/logstorm.h"
#include "gui/gui_renderer.h"
#include "render/webgpu_renderer.h"

using namespace std::string_literals;

class game_manager {
  logstorm::manager logger{logstorm::manager::build_with_sink<logstorm::sink::console>()}; // logging system
  render::webgpu_renderer renderer{logger};                                     // WebGPU rendering system
  gui::gui_renderer gui{logger};                                                // GUI top level

  vec2f mouse_pos_rel{};                                                        // relative mouse position

  void loop_main();

public:
  game_manager();
};

game_manager::game_manager() {
  /// Run the game
  renderer.init(
    [&](render::webgpu_renderer::webgpu_data const& webgpu){
      ImGui_ImplWGPU_InitInfo imgui_wgpu_info;
      imgui_wgpu_info.Device = webgpu.device.Get();
      imgui_wgpu_info.RenderTargetFormat = static_cast<WGPUTextureFormat>(webgpu.surface_preferred_format);

      gui.init(imgui_wgpu_info);
      gui.shader_code = renderer.get_shader();
    },
    [&]{
      loop_main();
    }
  );
  std::unreachable();
}

void game_manager::loop_main() {
  /// Main pseudo-loop
  gui.draw();

  if(gui.shader_code_updated) {
    renderer.update_shader(gui.shader_code);
    gui.shader_code_updated = false;
  }

  mouse_pos_rel += vec2f{ImGui::GetMouseDragDelta()} * 0.00001f * vec2f{-1.0f, 1.0f};
  renderer.draw(mouse_pos_rel);
}

auto main()->int {
  try {
    game_manager game;
    std::unreachable();

  } catch (std::exception const &e) {
    std::cerr << "Exception: " << e.what() << std::endl;
    EM_ASM(alert("Error: Press F12 to see console for details."));
  }

  return EXIT_SUCCESS;
}
