#pragma once
#include <expected>
#include <fstream>
#include <iostream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "FsShims/FsStructs.hpp"
#include "imgui.h"
#include "nlohmann/json.hpp"

typedef bool (*GaugeInitFunc)(unsigned long long ctx, sGaugeInstallData *install_data);
typedef bool (*GaugeDrawFunc)(unsigned long long ctx, sGaugeDrawData *draw_data);
typedef bool (*GaugeKillFunc)(unsigned long long ctx);
typedef bool (*GaugeUpdateFunc)(unsigned long long ctx, float dTime);
typedef void (*GaugeMouseHandlerFunc)(unsigned long long ctx, float fX, float fY, int iFlags);

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


    struct MountParams {
      int width;
      int height;
      std::string str_params;
    };
    MountParams mount_params;
  };

  static GaugeLoader *GetInstance() {
    if (!m_Instance) {
      m_Instance = new GaugeLoader();
    }
    return m_Instance;
  }


  void UpdateGauges(float dTime) {
    for (const auto &gauge: m_Gauges) {
      if (gauge.second.second.update) {
        gauge.second.second.update(gauge.second.first, dTime);
      }
    }
  }
  std::unordered_map<std::string, std::pair<unsigned long long, Gauge>> GetAllGauges() const { return m_Gauges; }
  std::vector<std::pair<std::string, double>> GetVariables() const { return m_Variables; }
  int AddVariable(const std::string &name, double value) {
    m_Variables.emplace_back(name, value);
    return m_Variables.size() - 1;
  }
  void RemoveVariable(const std::string &name) {
    for (int i = 0; i < m_Variables.size(); ++i) {
      if (m_Variables[i].first == name) {
        m_Variables.erase(m_Variables.begin() + i);
      }
    }
  }
  double GetVariable(const std::string &name) {
    for (int i = 0; i < m_Variables.size(); ++i) {
      if (m_Variables[i].first == name) {
        return m_Variables[i].second;
      }
    }
    return 0;
  }
  double GetVariable(const int id) {
    if (id >= 0 && id < m_Variables.size()) return m_Variables[id].second;
    return 0;
  }
  void AddVariable(const std::vector<std::pair<std::string, double>> &values) {
    for (const auto &value: values) {
      AddVariable(value.first, value.second);
    }
  }

  void UpdateVariable(const int id, double value) { m_Variables[id].second = value; }

  std::vector<InstrumentRenderer> GetAllRenderers() { return m_Renderers; }

  std::pair<unsigned long long, Gauge> GetOrLoadGauge(const std::string &gauge_path, const std::string &gauge_name);

  std::expected<void, std::string> UnloadGauge(const std::string &gauge_name);
  std::expected<void, std::string> UnloadAllGauges();

  bool AreGaugesLoaded() const { return !m_Gauges.empty(); }

  private:
  std::expected<std::pair<unsigned long long, Gauge>, std::string> LoadGauge(const std::string &gauge_path,
                                                                             const std::string &gauge_name);

  std::pair<unsigned long long, Gauge> GetFromMap(const std::string &gauge_name) const;

  static std::optional<Gauge::MountParams> ParseJson(const std::string &json_path) {
    Gauge::MountParams params{0, 0, ""};
    if (!std::filesystem::exists(json_path)) {
      return std::nullopt;
    }

    std::ifstream file(json_path);
    if (!file.is_open()) {
      return std::nullopt;
    }

    nlohmann::json json;
    file >> json;

    try {
      params.width = json["gauge"]["size"]["width"].get<int>();
      params.height = json["gauge"]["size"]["height"].get<int>();
      params.str_params = json["gauge"]["string_params"].get<std::string>();
    } catch (const std::exception &e) {
      std::cerr << e.what() << std::endl;
      return std::nullopt;
    }

    return params;
  }

  private:
  static GaugeLoader *m_Instance;

  std::unordered_map<std::string, std::pair<unsigned long long, Gauge>> m_Gauges;  // <gauge_name, <ctx, Gauge>
  std::vector<InstrumentRenderer> m_Renderers;
  std::vector<std::pair<std::string, double>> m_Variables;
};

struct NVGcontext;

class InstrumentRenderer {
  public:
  InstrumentRenderer(const std::string &title, const unsigned long long gaugeCtx, GaugeLoader::Gauge gauge)
      : m_Title(title)
      , m_GaugeCtx(gaugeCtx)
      , m_gauge(gauge) {}

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
