#include "GaugeLoader.hpp"
#include <dlfcn.h>
#include <iostream>
#include <ostream>
#include <ranges>
#include <stdexcept>

GaugeLoader *GaugeLoader::m_Instance = nullptr;

std::expected<GaugeLoader::Gauge, std::string>
GaugeLoader::LoadGauge(const std::string &gauge_path,
                       const std::string &gauge_name) {

  void *handle = dlopen(gauge_path.c_str(), RTLD_LAZY | RTLD_GLOBAL);
  if (!handle) {
    return std::unexpected(std::string("Failed to load gauge: ") + dlerror());
  }

  std::cout << "Loadinggauge " << gauge_path << std::endl;

  auto gauge = Gauge{
      handle,
      (GaugeInitFunc)(dlsym(handle,
                            std::string(gauge_name + "_gauge_init").c_str())),
      (GaugeDrawFunc)(dlsym(handle,
                            std::string(gauge_name + "_gauge_draw").c_str())),
      (GaugeKillFunc)(dlsym(handle,
                            std::string(gauge_name + "_gauge_kill").c_str()))};

  if (!gauge.init || !gauge.draw || !gauge.kill) {
    dlclose(handle);
    return std::unexpected(std::string("Failed to load gauge functions: ") +
                           dlerror());
  }
  std::cout << "GaugeLoader::LoadGauge: " << gauge.init << std::endl;
  return gauge;
}

GaugeLoader::Gauge GaugeLoader::GetOrLoadGauge(const std::string &gauge_path,
                                               const std::string &gauge_name) {
  if (m_Gauges.contains(gauge_name)) {
    return GetFromMap(gauge_name);
  }
  auto gauge_result = LoadGauge(gauge_path, gauge_name);
  if (!gauge_result) {
    throw std::runtime_error(gauge_result.error());
  }
  m_Gauges[gauge_name] = gauge_result.value();
  return m_Gauges[gauge_name];
}

GaugeLoader::Gauge
GaugeLoader::GetFromMap(const std::string &gauge_name) const {
  return m_Gauges.at(gauge_name);
}

std::expected<void, std::string>
GaugeLoader::UnloadGauge(const std::string &gauge_name) {
  const auto it = m_Gauges.find(gauge_name);
  if (it == m_Gauges.end()) {
    return std::unexpected(std::string("Gauge not found: ") + gauge_name);
  }

  const Gauge &gauge = it->second;
  if (gauge.kill) {
    gauge.kill(nullptr);
  }
  dlclose(gauge.handle);
  m_Gauges.erase(it);
  return {};
}

std::expected<void, std::string> GaugeLoader::UnloadAllGauges() {
  for (const auto &gauge : std::views::keys(m_Gauges)) {
    if (auto result = UnloadGauge(gauge); !result.has_value()) {
      return std::unexpected(result.error());
    }
  }
  m_Gauges.clear();
  return {};
}
