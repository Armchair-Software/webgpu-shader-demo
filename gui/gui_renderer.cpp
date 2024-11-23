#include "gui_renderer.h"
#include <emscripten/html5.h>
#include <imgui/imgui_impl_emscripten.h>
#include <imgui/imgui_impl_wgpu.h>
#include "logstorm/logstorm.h"

namespace gui {

gui_renderer::gui_renderer(logstorm::manager &this_logger)
  :logger{this_logger} {
  /// Construct the top level GUI and initialise ImGUI
  logger << "GUI: Initialising";
  #ifndef NDEBUG
    IMGUI_CHECKVERSION();
  #endif // NDEBUG
  ImGui::CreateContext();
  auto &imgui_io{ImGui::GetIO()};

  imgui_io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
  imgui_io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
}

void gui_renderer::init(ImGui_ImplWGPU_InitInfo &imgui_wgpu_info) {
  /// Any additional initialisation that needs to occur after WebGPU has been initialised
  ImGui_ImplWGPU_Init(&imgui_wgpu_info);
  ImGui_ImplEmscripten_Init();
}

void gui_renderer::draw() const {
  /// Render the top level GUI
  ImGui_ImplWGPU_NewFrame();
  ImGui_ImplEmscripten_NewFrame();
  ImGui::NewFrame();

  ImGui::ShowDemoWindow();

  ImGui::Render();                                                              // finalise draw data (actual rendering of draw data is done by the renderer later)
}

}
