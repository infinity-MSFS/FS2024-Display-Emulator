#pragma once
#include "FsShims/FsStructs.hpp"
#include "imgui.h"

#include <expected>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

typedef bool (*GaugeInitFunc)(unsigned long long ctx,
                              sGaugeInstallData *install_data);
typedef bool (*GaugeDrawFunc)(unsigned long long ctx,
                              sGaugeDrawData *draw_data);
typedef bool (*GaugeKillFunc)(unsigned long long ctx);
typedef bool (*GaugeUpdateFunc)(unsigned long long ctx, float dTime);
typedef void (*GaugeMouseHandlerFunc)(unsigned long long ctx, float fX,
                                      float fY, int iFlags);

class InstrumentRenderer;

class GaugeLoader {
public:
  struct Gauge {
    void *handle;
    GaugeInitFunc init;
    GaugeDrawFunc draw;
    GaugeKillFunc kill;
    GaugeUpdateFunc update;
    GaugeMouseHandlerFunc mouse_handler;
  };

  static GaugeLoader *GetInstance() {
    if (!m_Instance) {
      m_Instance = new GaugeLoader();
    }
    return m_Instance;
  }

  std::unordered_map<std::string, std::pair<unsigned long long, Gauge>>
  GetAllGauges() const {
    return m_Gauges;
  }

  std::vector<InstrumentRenderer> GetAllRenderers() { return m_Renderers; }

  std::pair<unsigned long long, Gauge>
  GetOrLoadGauge(const std::string &gauge_path, const std::string &gauge_name);

  std::expected<void, std::string> UnloadGauge(const std::string &gauge_name);
  std::expected<void, std::string> UnloadAllGauges();

private:
  std::expected<std::pair<unsigned long long, Gauge>, std::string>
  LoadGauge(const std::string &gauge_path, const std::string &gauge_name);

  std::pair<unsigned long long, Gauge>
  GetFromMap(const std::string &gauge_name) const;

private:
  static GaugeLoader *m_Instance;

  std::unordered_map<std::string, std::pair<unsigned long long, Gauge>>
      m_Gauges; // <gauge_name, <ctx, Gauge>
  std::vector<InstrumentRenderer> m_Renderers;
};

struct NVGcontext;

class InstrumentRenderer {

public:
  InstrumentRenderer(const std::string &title,
                     const unsigned long long gaugeCtx,
                     GaugeLoader::Gauge gauge)
      : m_Title(title), m_GaugeCtx(gaugeCtx), m_gauge(gauge) {}

  void CreateImGuiWindow();
  void RenderContents();

  std::string GetTitle() const { return m_Title; }

private:
  std::string m_Title;
  static ImVec2 m_Size;
  unsigned long long m_GaugeCtx;
  static ImVec2 m_Position;
  GaugeLoader::Gauge m_gauge;
};
