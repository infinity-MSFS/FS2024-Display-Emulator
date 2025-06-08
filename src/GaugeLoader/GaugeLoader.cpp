#include "GaugeLoader.hpp"

#include "Application/Application.hpp"
#include "FileDialog/FileDialog.hpp"
//
#include <GLFW/glfw3.h>
#include <atomic>
#include <dlfcn.h>
#include <iostream>
#include <ostream>
#include <ranges>
#include <stdexcept>
#include <thread>

#include "nanovg.h"

GaugeLoader *GaugeLoader::m_Instance = nullptr;

ImVec2 InstrumentRenderer::m_Position = {0.0f, 0.0f};
ImVec2 InstrumentRenderer::m_Size = {0.0f, 0.0f};

static unsigned long long base_ctx = 1;


void reload_gauge(const std::string &gauge_name, const std::string &gauge_path) {
  auto gauge_loader = GaugeLoader::GetInstance();
  gauge_loader->UnloadAllGauges();
  std::thread([gauge_path, gauge_name]() {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));  // Wait for this shit to work
    std::cout << "reloading: " << gauge_path << " " << gauge_name << std::endl;
    try {
      //  GaugeLoader::GetInstance()->GetOrLoadGauge(gauge_path, gauge_name);
      GaugeLoader::GetInstance()->SetUpdateQueued(true);
    } catch (const std::exception &e) {
      std::cerr << "Error loading gauge: " << e.what() << std::endl;
    }
  }).detach();
}

void start_gauge_watcher(const std::filesystem::path &gauge_path, const std::string &gauge_name) {
  std::thread([gauge_path, gauge_name]() {
    std::cout << "[Watcher] Started for " << gauge_name << std::endl;
    std::error_code error_code;
    auto last_write_time = std::filesystem::last_write_time(gauge_path, error_code);

    while (true) {
      std::this_thread::sleep_for(std::chrono::milliseconds(500));

      auto current_time = std::filesystem::last_write_time(gauge_path, error_code);
      if (error_code) {
        std::cerr << "[Watcher] Error checking file: " << error_code.message() << std::endl;
        continue;
      }
      if (current_time != last_write_time) {
        std::cout << "[Watcher] File changed: " << gauge_name << std::endl;
        last_write_time = current_time;

        std::thread([gauge_path, gauge_name]() { reload_gauge(gauge_name, gauge_path); }).detach();
        return;
      }
    }
  }).detach();
  ;
}

std::expected<std::pair<unsigned long long, GaugeLoader::Gauge>, std::string> GaugeLoader::LoadGauge(
    const std::string &gauge_path, const std::string &gauge_name) {
  void *handle = dlopen(gauge_path.c_str(), RTLD_LAZY | RTLD_GLOBAL);
  if (!handle) {
    return std::unexpected(std::string("Failed to load gauge: ") + dlerror());
  }

  std::cout << "Loading gauge " << gauge_path << std::endl;
  auto json_path = FileDialog::GetJsonFilePath(gauge_path);
  if (!json_path.has_value()) {
    dlclose(handle);
    return std::unexpected(std::string("Failed to find JSON file for gauge: ") + gauge_name);
  }
  auto mount_params = ParseJson(json_path.value());
  if (!mount_params.has_value()) {
    dlclose(handle);
    return std::unexpected(std::string("Failed to parse JSON file for gauge: ") + json_path.value());
  }

  auto gauge = Gauge{
      handle,
      (GaugeInitFunc) (dlsym(handle, std::string(gauge_name + "_gauge_init").c_str())),
      (GaugeDrawFunc) (dlsym(handle, std::string(gauge_name + "_gauge_draw").c_str())),
      (GaugeKillFunc) (dlsym(handle, std::string(gauge_name + "_gauge_kill").c_str())),
      (GaugeUpdateFunc) (dlsym(handle, std::string(gauge_name + "_gauge_update").c_str())),
      (GaugeMouseHandlerFunc) (dlsym(handle, std::string(gauge_name + "_gauge_mouse_handler").c_str())),
      mount_params.value(),
  };


  if (!gauge.init || !gauge.draw || !gauge.kill || !gauge.update || !gauge.mouse_handler) {
    dlclose(handle);
    return std::unexpected(std::string("Failed to load gauge functions: ") + dlerror());
  }

  InstrumentRenderer renderer(gauge_name, base_ctx, gauge);

  m_Renderers.push_back(renderer);

  gauge.init(base_ctx, nullptr);

  std::cout << "GaugeLoader::LoadGauge: " << gauge.init << std::endl;

  std::pair<unsigned long long, Gauge> ret = std::make_pair(base_ctx, gauge);
  base_ctx++;

  start_gauge_watcher(gauge_path, gauge_name);

  return ret;
}

std::pair<unsigned long long, GaugeLoader::Gauge> GaugeLoader::GetOrLoadGauge(const std::string &gauge_path,
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

std::pair<unsigned long long, GaugeLoader::Gauge> GaugeLoader::GetFromMap(const std::string &gauge_name) const {
  return m_Gauges.at(gauge_name);
}

std::expected<void, std::string> GaugeLoader::UnloadGauge(const std::string &gauge_name) {
  // std::cout << "Unloading gauge: " << gauge_name << std::endl;
  if (m_Gauges.empty()) {
    return std::unexpected(std::string("Gauge is already unloaded: ") + gauge_name);
  }
  const auto gauge = m_Gauges.at("MFD");

  if (gauge.second.kill) {
    gauge.second.kill(gauge.first);
  }
  m_Gauges.clear();
  m_Renderers.clear();
  dlclose(gauge.second.handle);

  return {};
}

std::expected<void, std::string> GaugeLoader::UnloadAllGauges() {
  for (const auto &gauge: m_Gauges) {
    if (auto result = UnloadGauge("MFD"); !result.has_value()) {
      return std::unexpected(result.error());
    }
  }
  m_Gauges.clear();
  return {};
}

void InstrumentRenderer::RenderContents() {
  int display_w, display_h;
  glfwGetFramebufferSize(Application::Get().value()->GetHandle(), &display_w, &display_h);
  glViewport(0, 0, display_w, display_h);

  int fbWidth = static_cast<int>(m_Size.x);
  int fbHeight = static_cast<int>(m_Size.y);
  if (fbWidth > 0 && fbHeight > 0) {
    GLint last_viewport[4];
    glGetIntegerv(GL_VIEWPORT, last_viewport);
    GLboolean last_scissor_test;
    glGetBooleanv(GL_SCISSOR_TEST, &last_scissor_test);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_SCISSOR_TEST);

    glViewport(m_Position.x, display_h - m_Position.y - fbHeight, fbWidth, fbHeight);

    sGaugeDrawData gaugeData{ImGui::GetMousePos().x - m_Position.x,
                             ImGui::GetMousePos().y - m_Position.y,
                             static_cast<double>(glfwGetTime()),
                             0.0f,  // TODO: calculate delta time
                             static_cast<int>(m_Size.x),
                             static_cast<int>(m_Size.y),
                             static_cast<int>(m_Size.x),
                             static_cast<int>(m_Size.y)};

    m_gauge.draw(m_GaugeCtx, &gaugeData);

    glViewport(last_viewport[0], last_viewport[1], last_viewport[2], last_viewport[3]);
    if (last_scissor_test) glEnable(GL_SCISSOR_TEST);
  }
}

void InstrumentRenderer::CreateImGuiWindow() {
  ImGui::SetNextWindowSize(
      {static_cast<float>(m_gauge.mount_params.width), static_cast<float>(m_gauge.mount_params.height)});
  std::string title =
      m_Title + " " + std::to_string(m_gauge.mount_params.width) + "x" + std::to_string(m_gauge.mount_params.height);
  ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));

  ImGui::Begin(title.c_str(), nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);
  ImVec2 position = ImGui::GetCursorScreenPos();
  ImVec2 size = ImGui::GetContentRegionAvail();
  m_Position = position;
  m_Size = size;

  if (ImGui::InvisibleButton("canvas", m_Size)) {
    float mouse_x_pos, mouse_y_pos;
    mouse_x_pos = ImGui::GetMousePos().x - m_Position.x;
    mouse_y_pos = ImGui::GetMousePos().y - m_Position.y;
    m_gauge.mouse_handler(m_GaugeCtx, mouse_x_pos, mouse_y_pos,
                          0);  // TODO: handle mouse flags
  }

  ImGui::End();
  ImGui::PopStyleColor();
}
