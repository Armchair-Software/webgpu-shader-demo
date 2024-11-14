#include <iostream>
#include <boost/throw_exception.hpp>
#include <emscripten.h>
//#include <GLFW/glfw3.h>
#include <imgui/imgui.h>
#include <imgui/imgui_impl_wgpu.h>
#include "logstorm/logstorm.h"
#include "render/webgpu_renderer.h"

#ifdef BOOST_NO_EXCEPTIONS
void boost::throw_exception(std::exception const & e) {
  /// Custom exception replacement function when boost exceptions are disabled
  std::cerr << "ERROR: Boost would have thrown exception: " << e.what() << std::endl;
  abort();
  std::unreachable();
}
#endif // BOOST_NO_EXCEPTIONS


namespace gui {

class top_level {
  logstorm::manager &logger;

public:
  top_level(logstorm::manager &logger);

  void init(ImGui_ImplWGPU_InitInfo &wgpu_info);

  void draw() const;
};

top_level::top_level(logstorm::manager &this_logger)
  :logger{this_logger} {
  /// Construct the top level GUI and initialise ImGUI
  logger << "GUI: Initialising";
  #ifndef NDEBUG
    IMGUI_CHECKVERSION();
  #endif // NDEBUG
  ImGui::CreateContext();
  //ImGui::GetIO();
}

void top_level::init(ImGui_ImplWGPU_InitInfo &imgui_wgpu_info) {
  /// Any additional initialisation that needs to occur after WebGPU has been initialised
  //ImGui_ImplGlfw_InitForOther(m_window, true);
  ImGui_ImplWGPU_Init(&imgui_wgpu_info);
}

void top_level::draw() const {
  /// Render the top level GUI
  ImGui_ImplWGPU_NewFrame();
  //ImGui_ImplGlfw_NewFrame();
  ImGui::NewFrame();


  ImGui::Begin("Hello, world!");
  ImGui::TextUnformatted("Some text");
  ImGui::End();


  ImGui::Render();                                                              // finalise draw data (actual rendering of draw data is done by the renderer later)
}

}

class game_manager {
  logstorm::manager logger{logstorm::manager::build_with_sink<logstorm::sink::console>()}; // logging system
  render::webgpu_renderer renderer{logger};                                     // WebGPU rendering system
  gui::top_level gui{logger};                                                   // GUI top level

  //std::function<void(int, char const*)> glfw_callback_error;                    // callback for unhandled GLFW errors

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
      imgui_wgpu_info.DepthStencilFormat = static_cast<WGPUTextureFormat>(webgpu.depth_texture_format);

      gui.init(imgui_wgpu_info);
    },
    [&]{
      loop_main();
    }
  );
  std::unreachable();
}

void game_manager::loop_main() {
  /// Main pseudo-loop
  //glfwPollEvents();

  gui.draw();
  renderer.draw();

  //glfwSwapBuffers(window.glfw_window);
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
