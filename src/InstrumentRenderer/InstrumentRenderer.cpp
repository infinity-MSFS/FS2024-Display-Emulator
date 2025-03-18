#include "InstrumentRenderer.hpp"
#include "nanovg.h"

void InstrumentRenderer::RenderContents() {
  nvgBeginFrame(m_NVGContext, m_Size.x, m_Size.y, 1.0f);
  m_RenderCallback(*m_NVGContext);
  nvgEndFrame(m_NVGContext);
}

void InstrumentRenderer::CreateImGuiWindow() {
  ImGui::Begin("NanoVG");
  m_Position = ImGui::GetCursorScreenPos();
  m_Size = ImGui::GetContentRegionAvail();
  ImGui::InvisibleButton("canvas", {1.0f, 1.0f});
  ImGui::End();
}
