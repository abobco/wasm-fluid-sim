#pragma once
#ifdef __EMSCRIPTEN__
#include "../external/imgui.h"
#else
#include "imgui.h"
#include "imgui_impl_opengl3.h"
#include "imgui_impl_sdl.h"

#endif
#include <iostream>
#include <stdio.h>

#define DUMP(a)                                                                \
  { std::cout << #a " = " << (a) << std::endl; }

namespace xn {
#define IM_PRINTFARGS(FMT)
struct LogWindow {
  ImGuiTextBuffer Buf;
  bool ScrollToBottom;

  void Clear() { Buf.clear(); }

  void AddLog(const char *fmt, ...) IM_PRINTFARGS(2) {
    va_list args;
    va_start(args, fmt);
    Buf.appendfv(fmt, args);
    va_end(args);
    ScrollToBottom = true;
  }

  void AppendChars(const char *c_str) { Buf.appendf("%s", c_str); }

  void DrawTextWrapped() {
    ImGui::TextWrapped("%s", Buf.begin());
    if (ScrollToBottom)
      ImGui::SetScrollHere(1.0f);
    ScrollToBottom = false;
  }

  void Draw(const char *title, bool *p_opened = NULL) {
    // ImGui::SetNextWindowBgAlpha(0.7);
    ImGui::Begin(title, p_opened);
    DrawTextWrapped();
    ImGui::End();
  }
};

static LogWindow log_local;
static LogWindow log_rpi;
static LogWindow log_esp;

// printf to std::cout to capture logs easier
void xn_printf(const char *fmt, ...) IM_PRINTFARGS(2) {
  // get length of formatted string
  va_list args;
  va_start(args, fmt);
  size_t len = std::vsnprintf(NULL, 0, fmt, args);
  va_end(args);

  // print into string
  std::string str_formatted;
  str_formatted.reserve(len + 1);
  va_start(args, fmt);
  vsnprintf(&str_formatted[0], len + 1, fmt, args);
  va_end(args);

  std::cout << str_formatted;
}
} // namespace xn