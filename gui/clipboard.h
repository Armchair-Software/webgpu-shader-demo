#pragma once

#include <string>

namespace gui {

class clipboard {
  std::string content;

public:
  clipboard();
  ~clipboard();

  void set_imgui_callbacks();

  void scrub();
};

}
