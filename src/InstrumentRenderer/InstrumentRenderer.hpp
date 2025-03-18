#pragma once

#include <functional>
#include <imgui.h>
#include <string>
#include <vector>

struct NVGcontext;

class InstrumentRenderer {

public:
  InstrumentRenderer(NVGcontext *ctx, const std::string &title,
                     std::function<void(NVGcontext)> &renderCallback)
      : m_NVGContext(ctx), m_Title(title), m_RenderCallback(renderCallback) {}

  void CreateImGuiWindow();
  void RenderContents();

private:
  NVGcontext *m_NVGContext;
  std::string m_Title;
  ImVec2 m_Size;
  ImVec2 m_Position;
  std::function<void(NVGcontext)> m_RenderCallback;
};
