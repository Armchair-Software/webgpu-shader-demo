#include <iostream>
#include <boost/throw_exception.hpp>
#include <emscripten.h>
#include <emscripten/html5.h>
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

  // TODO: move these into new dedicated ImGUI back-end
  emscripten_set_mousemove_callback(
    EMSCRIPTEN_EVENT_TARGET_WINDOW,                                             // target
    nullptr,                                                                    // userData
    false,                                                                      // useCapture
    [](int /*event_type*/, EmscriptenMouseEvent const *mouse_event, void */*data*/){ // callback, event_type == EMSCRIPTEN_EVENT_MOUSEMOVE
      auto imgui_io{ImGui::GetIO()};
      imgui_io.AddMousePosEvent(
        static_cast<float>(mouse_event->clientX),
        static_cast<float>(mouse_event->clientY)
      );
      return true;                                                              // the event was consumed
    }
  );
  emscripten_set_mousedown_callback(
    EMSCRIPTEN_EVENT_TARGET_WINDOW,                                             // target
    nullptr,                                                                    // userData
    false,                                                                      // useCapture
    [](int /*event_type*/, EmscriptenMouseEvent const *mouse_event, void */*data*/){ // callback, event_type == EMSCRIPTEN_EVENT_MOUSEDOWN
      auto imgui_io{ImGui::GetIO()};
      imgui_io.AddMouseButtonEvent(mouse_event->button, true);                  // button, down
      return true;                                                              // the event was consumed
    }
  );
  emscripten_set_mouseup_callback(
    EMSCRIPTEN_EVENT_TARGET_WINDOW,                                             // target
    nullptr,                                                                    // userData
    false,                                                                      // useCapture
    [](int /*event_type*/, EmscriptenMouseEvent const *mouse_event, void */*data*/){ // callback, event_type == EMSCRIPTEN_EVENT_MOUSEUP
      auto imgui_io{ImGui::GetIO()};
      imgui_io.AddMouseButtonEvent(mouse_event->button, false);                 // button, up
      return true;                                                              // the event was consumed
    }
  );
  emscripten_set_mouseenter_callback(
    EMSCRIPTEN_EVENT_TARGET_DOCUMENT,                                           // target - WINDOW doesn't produce mouseenter events
    nullptr,                                                                    // userData
    false,                                                                      // useCapture
    [](int /*event_type*/, EmscriptenMouseEvent const *mouse_event, void */*data*/){ // callback, event_type == EMSCRIPTEN_EVENT_MOUSEENTER
      auto imgui_io{ImGui::GetIO()};
      imgui_io.AddMousePosEvent(
        static_cast<float>(mouse_event->clientX),
        static_cast<float>(mouse_event->clientY)
      );
      return true;                                                              // the event was consumed
    }
  );
  emscripten_set_mouseleave_callback(
    EMSCRIPTEN_EVENT_TARGET_DOCUMENT,                                           // target - WINDOW doesn't produce mouseenter events
    nullptr,                                                                    // userData
    false,                                                                      // useCapture
    [](int /*event_type*/, EmscriptenMouseEvent const */*mouse_event*/, void */*data*/){ // callback, event_type == EMSCRIPTEN_EVENT_MOUSELEAVE
      auto imgui_io{ImGui::GetIO()};
      imgui_io.AddMousePosEvent(-FLT_MAX, -FLT_MAX);                            // cursor is not in the window
      return true;                                                              // the event was consumed
    }
  );
  emscripten_set_wheel_callback(
    EMSCRIPTEN_EVENT_TARGET_DOCUMENT,                                           // target - WINDOW doesn't produce mouseenter events
    nullptr,                                                                    // userData
    false,                                                                      // useCapture
    [](int /*event_type*/, EmscriptenWheelEvent const *wheel_event, void */*data*/){ // callback, event_type == EMSCRIPTEN_EVENT_WHEEL
      float scale{1.0f};
      switch(wheel_event->deltaMode) {
      case DOM_DELTA_PIXEL:                                                     // scrolling in pixels
        scale = 1.0f / 100.0f;
        break;
      case DOM_DELTA_LINE:                                                      // scrolling by lines
        scale = 1.0f / 3.0f;
        break;
      case DOM_DELTA_PAGE:                                                      // scrolling by pages
        scale = 80.0f;
        break;
      }
      // TODO: make scrolling speeds configurable
      auto imgui_io{ImGui::GetIO()};
      imgui_io.AddMouseWheelEvent(
        -static_cast<float>(wheel_event->deltaX) * scale,
        -static_cast<float>(wheel_event->deltaY) * scale
      );
      return true;                                                              // the event was consumed
    }
  );

}

void top_level::draw() const {
  /// Render the top level GUI
  ImGui_ImplWGPU_NewFrame();
  //ImGui_ImplGlfw_NewFrame();
  ImGui::NewFrame();

  ImGui::ShowDemoWindow();

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
