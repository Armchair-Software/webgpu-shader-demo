#include <iostream>
#include <boost/throw_exception.hpp>
#include <emscripten.h>
#include <emscripten/html5.h>
//#include <GLFW/glfw3.h>
#include <imgui/imgui.h>
#include <imgui/imgui_impl_emscripten.h>
#include <imgui/imgui_impl_wgpu.h>
#include "logstorm/logstorm.h"
#include "render/webgpu_renderer.h"
#include "emscripten_browser_cursor.h"

using namespace std::string_literals;

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

  ImGui_ImplEmscripten_Init();


  emscripten_set_keydown_callback(
    EMSCRIPTEN_EVENT_TARGET_WINDOW,                                             // target
    this,                                                                       // userData
    false,                                                                      // useCapture
    [](int event_type, EmscriptenKeyboardEvent const *key_event, void *data){   // callback
      auto &gui{*static_cast<top_level*>(data)};
      auto &logger{gui.logger};
      logger << "DEBUG: event_type " << event_type;
      logger << "DEBUG: timestamp " << key_event->timestamp;
      logger << "DEBUG: location " << key_event->location;
      logger << "DEBUG: ctrlKey " << key_event->ctrlKey;
      logger << "DEBUG: shiftKey " << key_event->shiftKey;
      logger << "DEBUG: altKey " << key_event->altKey;
      logger << "DEBUG: metaKey " << key_event->metaKey;
      logger << "DEBUG: repeat " << key_event->repeat;
      logger << "DEBUG: charCode " << key_event->charCode;
      logger << "DEBUG: keyCode " << key_event->keyCode;                        // TODO: use this
      logger << "DEBUG: which " << key_event->which;
      logger << "DEBUG: key[EM_HTML5_SHORT_STRING_LEN_BYTES] " << key_event->key;
      logger << "DEBUG: code[EM_HTML5_SHORT_STRING_LEN_BYTES] " << key_event->code;
      logger << "DEBUG: charValue[EM_HTML5_SHORT_STRING_LEN_BYTES] " << key_event->charValue;
      logger << "DEBUG: locale[EM_HTML5_SHORT_STRING_LEN_BYTES] " << key_event->locale;
      logger << "DEBUG: key address " << reinterpret_cast<uintptr_t const>(key_event->key);
      logger << "DEBUG: code address " << reinterpret_cast<uintptr_t const>(key_event->code);
      //logger << "DEBUG: translate_emscripten_to_imgui_key " << translate_key(key_event->code);


      return false;                                                             // the event was consumed
    }
  );
}

void top_level::draw() const {
  /// Render the top level GUI
  ImGui_ImplWGPU_NewFrame();
  //ImGui_ImplGlfw_NewFrame();
  ImGui_ImplEmscripten_NewFrame();


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
