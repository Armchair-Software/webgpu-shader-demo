// dear imgui: Platform Backend for Emscripten HTML5
//
// This is a platform back-end, similar to and offering an alternative to imgui_impl_glfw.
// The intended use-case is for applications built with Emscripten, running in the browser, but *not* using GLFW.
// It uses Emscripten's HTML5 interface to tie callbacks to imgui input, handling window resizing,
// focus, cursor, keyboard input, touch, gamepad devices etc.  It does not attempt to handle rendering.
//
// A note about GLFW on Emscripten: Emscripten includes its own GLFW implementation, which wraps browser HTML5 callbacks to provide the standard GLFW input interface.
// This backend removes the middleman for input, providing a more efficient direct interface between Emscripten's functionality and imgui input.
//
// This is a useful accompaniment for WebGPU rendering (i.e. with imgui_impl_wgpu), where GLFW is not needed for rendering.
// In that case, this backend replaces all non-rendering-related functionality from GLFW, making it possible to avoid depending on GLFW altogether.
//
// For cursor rendering, this includes a cut-down implementation of the Emscripten Browser Cursor library: https://github.com/Armchair-Software/emscripten-browser-cursor
//
// Copyright 2024 Eugene Hopkinson

#pragma once

#ifndef __EMSCRIPTEN__
    #if defined(IMGUI_IMPL_WEBGPU_BACKEND_DAWN) == defined(IMGUI_IMPL_WEBGPU_BACKEND_WGPU)
    #error exactly one of IMGUI_IMPL_WEBGPU_BACKEND_DAWN or IMGUI_IMPL_WEBGPU_BACKEND_WGPU must be defined!
    #endif
#else
    #if defined(IMGUI_IMPL_WEBGPU_BACKEND_DAWN) || defined(IMGUI_IMPL_WEBGPU_BACKEND_WGPU)
    #error neither IMGUI_IMPL_WEBGPU_BACKEND_DAWN nor IMGUI_IMPL_WEBGPU_BACKEND_WGPU may be defined if targeting emscripten!
    #endif
#endif

/// Initialise the Emscripten backend, setting input callbacks.  This should be called after ImGui::CreateContext();
void ImGui_ImplEmscripten_Init();

/// Shut down the Emscripten backend.  This unsets all Emscripten input callbacks set by Init.
/// Note it'll also unset any Emscripten input callbacks set elsewhere in the program!
/// Note also there is no obligation to ever call this, as there is not necessarily any such concept as "shutting down" when running in the browser, and we have no resources to release.  The user can just close the tab.
void ImGui_ImplEmscripten_Shutdown();

/// Call every frame to update polled input events, i.e. gamepads, and update imgui's cursors.
/// If you aren't using gamepad input to control imgui, and you're not using browser native cursor rendering (i.e. if imgui is rendering cursors internally), you don't need to call this.
void ImGui_ImplEmscripten_NewFrame();
