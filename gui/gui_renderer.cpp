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

  emscripten_set_keydown_callback(
    EMSCRIPTEN_EVENT_TARGET_WINDOW,                                             // target
    this,                                                                       // userData
    false,                                                                      // useCapture
    [](int event_type, EmscriptenKeyboardEvent const *key_event, void *data){   // callback
      auto &gui{*static_cast<gui_renderer*>(data)};
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

void gui_renderer::draw() const {
  /// Render the top level GUI
  ImGui_ImplWGPU_NewFrame();
  ImGui_ImplEmscripten_NewFrame();
  ImGui::NewFrame();

  ImGui::ShowDemoWindow();

  ImGui::Render();                                                              // finalise draw data (actual rendering of draw data is done by the renderer later)
}

}
