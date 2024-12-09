#include "clipboard.h"
#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>
#include "emscripten_browser_clipboard.h"
#include "secure_cleanse.h"

#define DEBUG_CLIPBOARD

#ifdef NDEBUG
  // clipboard debugging must always be disabled in release builds for security
  #undef DEBUG_CLIPBOARD
#endif // NDEBUG

#ifdef DEBUG_CLIPBOARD
  #include <iostream>
  #include <iomanip>
#endif // DEBUG_CLIPBOARD

namespace gui {

clipboard::clipboard() {
  /// Default constructor
  emscripten_browser_clipboard::paste([](std::string &&paste_data, void *callback_data){
    /// Callback to handle clipboard paste from browser
    auto &parent{*static_cast<clipboard*>(callback_data)};
    #ifdef DEBUG_CLIPBOARD
      std::cout << "DEBUG: Clipboard: browser paste event with data " << std::quoted(paste_data) << std::endl;
    #endif // DEBUG_CLIPBOARD
    parent.content = std::move(paste_data);
  }, this);

  /*
  emscripten_browser_clipboard::copy([](void *callback_data){
    /// Callback to offer data for clipboard copy to browser
    auto &parent{*static_cast<clipboard*>(callback_data)};
    #ifdef DEBUG_CLIPBOARD
      std::cout << "DEBUG: Clipboard: browser copy event, sending data " << std::quoted(parent.content) << std::endl;
    #endif // DEBUG_CLIPBOARD
    return parent.content.c_str();
  });
  */
}

clipboard::~clipboard() {
  /// Default destructor
  scrub();                                                                      // clean up memory for security reasons on destruction
}

void clipboard::set_imgui_callbacks() {
  /// Set imgui clipboard callbacks - must be done after imgui context is initialised
  #ifdef DEBUG_CLIPBOARD
    std::cout << "DEBUG: Clipboard: Setting imgui callbacks" << std::endl;
  #endif // DEBUG_CLIPBOARD

  ImGuiPlatformIO &imgui_platform_io{ImGui::GetPlatformIO()};
  imgui_platform_io.Platform_ClipboardUserData = this;

  imgui_platform_io.Platform_GetClipboardTextFn = [](ImGuiContext *ctx){
    /// Callback for imgui, to return clipboard content
    auto &parent{*static_cast<clipboard*>(ctx->PlatformIO.Platform_ClipboardUserData)};

    #ifdef DEBUG_CLIPBOARD
      std::cout << "DEBUG: Clipboard: imgui requested content, returning " << std::quoted(parent.content) << std::endl;
    #endif // DEBUG_CLIPBOARD
    return parent.content.c_str();
  };

  imgui_platform_io.Platform_SetClipboardTextFn = [](ImGuiContext *ctx, char const *text){
    /// Callback for imgui, to set clipboard content
    auto &parent{*static_cast<clipboard*>(ctx->PlatformIO.Platform_ClipboardUserData)};

    parent.content = text;
    #ifdef DEBUG_CLIPBOARD
      std::cout << "DEBUG: Clipboard: imgui set content, now " << std::quoted(parent.content) << std::endl;
    #endif // DEBUG_CLIPBOARD
    emscripten_browser_clipboard::copy(parent.content);
  };
}

void clipboard::scrub() {
  /// Wipe memory securely
  #ifdef DEBUG_CLIPBOARD
    std::cout << "DEBUG: Clipboard: Scrubbing" << std::endl;
  #endif // DEBUG_CLIPBOARD
  secure_cleanse(content);
}

}
