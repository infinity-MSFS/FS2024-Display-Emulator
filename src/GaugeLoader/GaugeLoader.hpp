#pragma once
#include <expected>
#include <string>
#include <unordered_map>

typedef bool (*GaugeInitFunc)(void *ctx, void *install_data);
typedef bool (*GaugeDrawFunc)(void *ctx, void *draw_data);
typedef bool (*GaugeKillFunc)(void *ctx);

class GaugeLoader {
public:
  struct Gauge {
    void *handle;
    GaugeInitFunc init;
    GaugeDrawFunc draw;
    GaugeKillFunc kill;
  };

  static GaugeLoader *GetInstance() {
    if (!m_Instance) {
      m_Instance = new GaugeLoader();
    }
    return m_Instance;
  }

  Gauge GetOrLoadGauge(const std::string &gauge_path,
                       const std::string &gauge_name);

  std::expected<void, std::string> UnloadGauge(const std::string &gauge_name);
  std::expected<void, std::string> UnloadAllGauges();

private:
  static std::expected<Gauge, std::string>
  LoadGauge(const std::string &gauge_path, const std::string &gauge_name);

  Gauge GetFromMap(const std::string &gauge_name) const;

private:
  static GaugeLoader *m_Instance;

  std::unordered_map<std::string, Gauge> m_Gauges;
};
