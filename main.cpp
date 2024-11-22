#include <iostream>
#include <functional>
#include <map>
#include <boost/throw_exception.hpp>
#include <emscripten/html5.h>
#include <imgui/imgui_impl_wgpu.h>
#include "logstorm/logstorm.h"
#include "gui/gui_renderer.h"
#include "render/webgpu_renderer.h"

using namespace std::string_literals;

#ifdef BOOST_NO_EXCEPTIONS
void boost::throw_exception(std::exception const & e) {
  /// Custom exception replacement function when boost exceptions are disabled
  std::cerr << "ERROR: Boost would have thrown exception: " << e.what() << std::endl;
  abort();
  std::unreachable();
}
#endif // BOOST_NO_EXCEPTIONS


struct gamepad {
  using analogue_button_callback = std::function<void(double)>;                 // callback for analogue button values
  using digital_button_callback = std::function<void(bool)>;                    // callback for digital button values
  using axis_callback = std::function<void(double)>;                            // callback for analogue axis values

  std::map<unsigned int, analogue_button_callback> analogue_buttons;
  std::map<unsigned int, digital_button_callback> digital_buttons;
  std::map<unsigned int, axis_callback> axes;
};

class game_manager {
  logstorm::manager logger{logstorm::manager::build_with_sink<logstorm::sink::console>()}; // logging system
  render::webgpu_renderer renderer{logger};                                     // WebGPU rendering system
  gui::gui_renderer gui{logger};                                                // GUI top level

  std::map<int, gamepad> gamepads;

  void register_gamepad_events();
  void set_gamepad_callbacks(gamepad const& this_gamepad);
  void handle_gamepad_events();

  void loop_main();

public:
  game_manager();
};

game_manager::game_manager() {
  /// Run the game
  register_gamepad_events();

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

void game_manager::register_gamepad_events() {
  /// Register gamepad event callbacks
  emscripten_set_gamepadconnected_callback(
    this,                                                                       // userData
    false,                                                                      // useCapture
    ([](int /*event_type*/, EmscriptenGamepadEvent const *event, void *data) {  // event_type == EMSCRIPTEN_EVENT_GAMEPADCONNECTED
      auto &game{*static_cast<game_manager*>(data)};
      auto &logger{game.logger};

      logger << "DEBUG: gamepad connected, timestamp " << event->timestamp;
      logger << "DEBUG: gamepad connected, numAxes " << event->numAxes;
      logger << "DEBUG: gamepad connected, numButtons " << event->numButtons;
      logger << "DEBUG: gamepad connected, connected " << event->connected;
      logger << "DEBUG: gamepad connected, index " << event->index;
      logger << "DEBUG: gamepad connected, id " << event->id;
      logger << "DEBUG: gamepad connected, mapping " << event->mapping;

      auto [new_gamepad_it, success]{game.gamepads.emplace(event->index, gamepad{})};
      assert(success);
      game.set_gamepad_callbacks(new_gamepad_it->second);

      return true;                                                              // the event was consumed
    })
  );
  emscripten_set_gamepaddisconnected_callback(
    this,                                                                       // userData
    false,                                                                      // useCapture
    [](int /*event_type*/, EmscriptenGamepadEvent const *event, void *data) {   // event_type == EMSCRIPTEN_EVENT_GAMEPADDISCONNECTED
      auto &game{*static_cast<game_manager*>(data)};
      auto &logger{game.logger};

      logger << "DEBUG: gamepad " << event->index << " disconnected";

      game.gamepads.erase(event->index);

      return true;                                                              // the event was consumed
    }
  );
}

void game_manager::set_gamepad_callbacks(gamepad const& this_gamepad) {
  /// Set up gamepad button and axis callbacks on the given gamepad
  // TODO
}

void game_manager::handle_gamepad_events() {
  /// Handle gamepad events, calling any callbacks that have been set
  if(gamepads.empty()) return;
  if(emscripten_sample_gamepad_data() != EMSCRIPTEN_RESULT_SUCCESS) return;

  for(auto [gamepad_index, this_gamepad] : gamepads) {
    EmscriptenGamepadEvent gamepad_state;
    if(emscripten_get_gamepad_status(gamepad_index, &gamepad_state) != EMSCRIPTEN_RESULT_SUCCESS) continue;
    for(auto const &[button_index, button] : this_gamepad.analogue_buttons) {
      assert(button_index < static_cast<unsigned int>(gamepad_state.numButtons));
      assert(button);
      button(gamepad_state.analogButton[button_index]);
    }
    for(auto const &[button_index, button] : this_gamepad.digital_buttons) {
      assert(button_index < static_cast<unsigned int>(gamepad_state.numButtons));
      assert(button);
      button(gamepad_state.digitalButton[button_index]);
    }
    for(auto const &[axis_index, axis] : this_gamepad.axes) {
      assert(axis_index < static_cast<unsigned int>(gamepad_state.numAxes));
      assert(axis);
      axis(gamepad_state.axis[axis_index]);
    }
  }
}

void game_manager::loop_main() {
  /// Main pseudo-loop
  handle_gamepad_events();
  gui.draw();
  renderer.draw();
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
