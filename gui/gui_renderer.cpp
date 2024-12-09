#include "gui_renderer.h"
#include <emscripten/html5.h>
#include <imgui/imgui_impl_emscripten.h>
#include <imgui/imgui_impl_wgpu.h>
#include <imgui/imgui_stdlib.h>
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

  clipboard.set_imgui_callbacks();
}

void gui_renderer::draw() {
  /// Render the top level GUI
  ImGui_ImplWGPU_NewFrame();
  ImGui_ImplEmscripten_NewFrame();
  ImGui::NewFrame();

  draw_shader_code_window();

  //ImGui::ShowDemoWindow();

  ImGui::Render();                                                              // finalise draw data (actual rendering of draw data is done by the renderer later)
}

void gui_renderer::draw_shader_code_window() {
  /// Draw the shader code editor window
  if(!ImGui::Begin("Shader")) {
    ImGui::End();
    return;
  }
  ImGui::SetWindowSize(ImVec2(600, 600), ImGuiCond_FirstUseEver);

  ImVec2 available_space{ImGui::GetContentRegionAvail()};
  available_space.y -= ImGui::GetFrameHeightWithSpacing(); // subtract button height

  ImGui::InputTextMultiline("#shader_code", &shader_code, available_space);
  if(ImGui::Button("Update")) shader_code_updated = true;

  ImGui::End();
}

}
