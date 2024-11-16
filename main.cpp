#include <iostream>
#include <boost/throw_exception.hpp>
#include <emscripten.h>
#include <emscripten/html5.h>
#include <emscripten/val.h>
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

static const std::unordered_map<std::string, ImGuiKey> key_translate_lookup{
  {"Backquote",            ImGuiKey_GraveAccent},
  {"Backslash",            ImGuiKey_Backslash},
  {"BracketLeft",          ImGuiKey_LeftBracket},
  {"BracketRight",         ImGuiKey_RightBracket},
  {"Comma",                ImGuiKey_Comma},
  {"Digit0",               ImGuiKey_0},
  {"Digit1",               ImGuiKey_1},
  {"Digit2",               ImGuiKey_2},
  {"Digit3",               ImGuiKey_3},
  {"Digit4",               ImGuiKey_4},
  {"Digit5",               ImGuiKey_5},
  {"Digit6",               ImGuiKey_6},
  {"Digit7",               ImGuiKey_7},
  {"Digit8",               ImGuiKey_8},
  {"Digit9",               ImGuiKey_9},
  {"Equal",                ImGuiKey_Equal},
  {"IntlBackslash",        ImGuiKey_Backslash},                                 // Mapping to generic backslash
  {"IntlRo",               ImGuiKey_Slash},                                     // Closest match for non-standard layouts
  {"IntlYen",              ImGuiKey_Backslash},                                 // Closest match for non-standard layouts
  {"KeyA",                 ImGuiKey_A},
  {"KeyB",                 ImGuiKey_B},
  {"KeyC",                 ImGuiKey_C},
  {"KeyD",                 ImGuiKey_D},
  {"KeyE",                 ImGuiKey_E},
  {"KeyF",                 ImGuiKey_F},
  {"KeyG",                 ImGuiKey_G},
  {"KeyH",                 ImGuiKey_H},
  {"KeyI",                 ImGuiKey_I},
  {"KeyJ",                 ImGuiKey_J},
  {"KeyK",                 ImGuiKey_K},
  {"KeyL",                 ImGuiKey_L},
  {"KeyM",                 ImGuiKey_M},
  {"KeyN",                 ImGuiKey_N},
  {"KeyO",                 ImGuiKey_O},
  {"KeyP",                 ImGuiKey_P},
  {"KeyQ",                 ImGuiKey_Q},
  {"KeyR",                 ImGuiKey_R},
  {"KeyS",                 ImGuiKey_S},
  {"KeyT",                 ImGuiKey_T},
  {"KeyU",                 ImGuiKey_U},
  {"KeyV",                 ImGuiKey_V},
  {"KeyW",                 ImGuiKey_W},
  {"KeyX",                 ImGuiKey_X},
  {"KeyY",                 ImGuiKey_Y},
  {"KeyZ",                 ImGuiKey_Z},
  {"Minus",                ImGuiKey_Minus},
  {"Period",               ImGuiKey_Period},
  {"Quote",                ImGuiKey_Apostrophe},
  {"Semicolon",            ImGuiKey_Semicolon},
  {"Slash",                ImGuiKey_Slash},

  {"AltLeft",              ImGuiKey_LeftAlt},
  {"AltRight",             ImGuiKey_RightAlt},
  {"Backspace",            ImGuiKey_Backspace},
  {"CapsLock",             ImGuiKey_CapsLock},
  {"ContextMenu",          ImGuiKey_Menu},
  {"ControlLeft",          ImGuiKey_LeftCtrl},
  {"ControlRight",         ImGuiKey_RightCtrl},
  {"Enter",                ImGuiKey_Enter},
  {"MetaLeft",             ImGuiKey_LeftSuper},
  {"MetaRight",            ImGuiKey_RightSuper},
  {"ShiftLeft",            ImGuiKey_LeftShift},
  {"ShiftRight",           ImGuiKey_RightShift},
  {"Space",                ImGuiKey_Space},
  {"Tab",                  ImGuiKey_Tab},

  {"Delete",               ImGuiKey_Delete},
  {"End",                  ImGuiKey_End},
  //{"Help",                 ImGuiKey_PrintScreen},                               // Best approximation
  {"Home",                 ImGuiKey_Home},
  {"Insert",               ImGuiKey_Insert},
  {"PageDown",             ImGuiKey_PageDown},
  {"PageUp",               ImGuiKey_PageUp},

  {"ArrowDown",            ImGuiKey_DownArrow},
  {"ArrowLeft",            ImGuiKey_LeftArrow},
  {"ArrowRight",           ImGuiKey_RightArrow},
  {"ArrowUp",              ImGuiKey_UpArrow},

  {"NumLock",              ImGuiKey_NumLock},
  {"Numpad0",              ImGuiKey_Keypad0},
  {"Numpad1",              ImGuiKey_Keypad1},
  {"Numpad2",              ImGuiKey_Keypad2},
  {"Numpad3",              ImGuiKey_Keypad3},
  {"Numpad4",              ImGuiKey_Keypad4},
  {"Numpad5",              ImGuiKey_Keypad5},
  {"Numpad6",              ImGuiKey_Keypad6},
  {"Numpad7",              ImGuiKey_Keypad7},
  {"Numpad8",              ImGuiKey_Keypad8},
  {"Numpad9",              ImGuiKey_Keypad9},
  {"NumpadAdd",            ImGuiKey_KeypadAdd},
  {"NumpadBackspace",      ImGuiKey_Backspace},                                 // No direct mapping; backspace functionality
  //{"NumpadClear",          ImGuiKey_KeypadClear},                               // Custom-defined if needed
  //{"NumpadClearEntry",     ImGuiKey_KeypadClear},                               // Custom-defined if needed
  {"NumpadComma",          ImGuiKey_KeypadDecimal},                             // Closest match
  {"NumpadDecimal",        ImGuiKey_KeypadDecimal},
  {"NumpadDivide",         ImGuiKey_KeypadDivide},
  {"NumpadEnter",          ImGuiKey_KeypadEnter},
  {"NumpadEqual",          ImGuiKey_KeypadEqual},
  {"NumpadHash",           ImGuiKey_Backslash},                                 // Mapped to # on UK keyboard
  //{"NumpadMemoryAdd",      ImGuiKey_None},                                      // No defined mapping
  //{"NumpadMemoryClear",    ImGuiKey_None},                                      // No defined mapping
  //{"NumpadMemoryRecall",   ImGuiKey_None},                                      // No defined mapping
  //{"NumpadMemoryStore",    ImGuiKey_None},                                      // No defined mapping
  //{"NumpadMemorySubtract", ImGuiKey_None},                                      // No defined mapping
  {"NumpadMultiply",       ImGuiKey_KeypadMultiply},
  {"NumpadParenLeft",      ImGuiKey_LeftBracket},                               // Closest available
  {"NumpadParenRight",     ImGuiKey_RightBracket},                              // Closest available
  {"NumpadStar",           ImGuiKey_KeypadMultiply},                            // Same as multiply
  {"NumpadSubtract",       ImGuiKey_KeypadSubtract},

  {"Escape",               ImGuiKey_Escape},
  {"F1",                   ImGuiKey_F1},
  {"F2",                   ImGuiKey_F2},
  {"F3",                   ImGuiKey_F3},
  {"F4",                   ImGuiKey_F4},
  {"F5",                   ImGuiKey_F5},
  {"F6",                   ImGuiKey_F6},
  {"F6",                   ImGuiKey_F6},
  {"F7",                   ImGuiKey_F7},
  {"F8",                   ImGuiKey_F8},
  {"F9",                   ImGuiKey_F9},
  {"F10",                  ImGuiKey_F10},
  {"F11",                  ImGuiKey_F11},
  {"F12",                  ImGuiKey_F12},
  //{"Fn",                   ImGuiKey_None},                                      // No direct mapping
  //{"FnLock",               ImGuiKey_None},                                      // No direct mapping
  {"PrintScreen",          ImGuiKey_PrintScreen},
  {"ScrollLock",           ImGuiKey_ScrollLock},
  {"Pause",                ImGuiKey_Pause},
};

ImGuiKey translate_key(char const* emscripten_key) __attribute__((__const__));
ImGuiKey translate_key(char const* emscripten_key) {
  /// Translate an emscripten-provided browser string describing a keycode to an imgui key code
  if(auto it{key_translate_lookup.find(emscripten_key)}; it != key_translate_lookup.end()) {
    return it->second;
  }
  return ImGuiKey_None;
}

constexpr ImGuiMouseButton translate_mousebutton(unsigned short emscripten_button) __attribute__((__const__));
constexpr ImGuiMouseButton translate_mousebutton(unsigned short emscripten_button) {
  /// Translate an emscripten-provided integer describing a mouse button to an imgui mouse button
  if(emscripten_button == 1) return ImGuiMouseButton_Middle;                    // 1 = middle mouse button
  if(emscripten_button == 2) return ImGuiMouseButton_Right;                     // 2 = right mouse button
  if(emscripten_button > ImGuiMouseButton_COUNT) return ImGuiMouseButton_Middle; // treat any weird clicks on unexpected buttons (button 6 upwards) as middle mouse
  return emscripten_button;                                                     // any other button translates 1:1
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
      imgui_io.AddMouseButtonEvent(translate_mousebutton(mouse_event->button), true); // translated button, down
      return true;                                                              // the event was consumed
    }
  );
  emscripten_set_mouseup_callback(
    EMSCRIPTEN_EVENT_TARGET_WINDOW,                                             // target
    nullptr,                                                                    // userData
    false,                                                                      // useCapture
    [](int /*event_type*/, EmscriptenMouseEvent const *mouse_event, void */*data*/){ // callback, event_type == EMSCRIPTEN_EVENT_MOUSEUP
      auto imgui_io{ImGui::GetIO()};
      imgui_io.AddMouseButtonEvent(translate_mousebutton(mouse_event->button), false); // translated button, up
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
    EMSCRIPTEN_EVENT_TARGET_WINDOW,                                             // target
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
      return false;                                                             // the event was not consumed
    }
  );
  emscripten_set_keydown_callback(
    EMSCRIPTEN_EVENT_TARGET_WINDOW,                                             // target
    nullptr,                                                                    // userData
    false,                                                                      // useCapture
    [](int /*event_type*/, EmscriptenKeyboardEvent const *key_event, void */*data*/){ // callback, event_type == EMSCRIPTEN_EVENT_KEYDOWN
      auto imgui_io{ImGui::GetIO()};
      imgui_io.AddKeyEvent(translate_key(key_event->code), true);
      return false;                                                             // the event was not consumed
    }
  );
  emscripten_set_keyup_callback(
    EMSCRIPTEN_EVENT_TARGET_WINDOW,                                             // target
    nullptr,                                                                    // userData
    false,                                                                      // useCapture
    [](int /*event_type*/, EmscriptenKeyboardEvent const *key_event, void */*data*/){ // callback, event_type == EMSCRIPTEN_EVENT_KEYUP
      auto imgui_io{ImGui::GetIO()};
      imgui_io.AddKeyEvent(translate_key(key_event->code), false);
      return false;                                                             // the event was not consumed
    }
  );

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
      logger << "DEBUG: translate_emscripten_to_imgui_key " << translate_key(key_event->code);


      return false;                                                             // the event was consumed
    }
  );

  emscripten_set_resize_callback(
    EMSCRIPTEN_EVENT_TARGET_WINDOW,                                             // target
    nullptr,                                                                    // userData
    false,                                                                      // useCapture
    ([](int /*event_type*/, EmscriptenUiEvent const *event, void */*data*/) {   // event_type == EMSCRIPTEN_EVENT_RESIZE
      auto &imgui_io{ImGui::GetIO()};
      imgui_io.DisplaySize.x = static_cast<float>(event->windowInnerWidth);
      imgui_io.DisplaySize.y = static_cast<float>(event->windowInnerHeight);
      return true;                                                              // the event was consumed
    })
  );

  {
    // set up initial display size values
    auto &imgui_io{ImGui::GetIO()};
    imgui_io.DisplaySize.x = emscripten::val::global("window")["innerWidth"].as<float>();
    imgui_io.DisplaySize.y = emscripten::val::global("window")["innerHeight"].as<float>();
    auto const device_pixel_ratio{emscripten::val::global("window")["devicePixelRatio"].as<float>()};
    imgui_io.DisplayFramebufferScale.x = device_pixel_ratio;
    imgui_io.DisplayFramebufferScale.y = device_pixel_ratio;
  }
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
