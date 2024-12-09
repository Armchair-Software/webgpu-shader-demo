#pragma once
#include <string>
#include "clipboard.h"
#include "logstorm/logstorm_forward.h"

class ImGui_ImplWGPU_InitInfo;

namespace gui {

class gui_renderer {
  logstorm::manager &logger;

  clipboard clipboard;

public:
  std::string shader_code;
  bool shader_code_updated{false};

  gui_renderer(logstorm::manager &logger);

  void init(ImGui_ImplWGPU_InitInfo &wgpu_info);

  void draw();
  void draw_shader_code_window();
};

}
