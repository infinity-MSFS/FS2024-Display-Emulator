#include <iostream>

#include "Application/Application.hpp"
#include "Application/Layer.hpp"
#include "FileDialog/FileDialog.hpp"
#include "GaugeLoader/GaugeLoader.hpp"

struct VariableConfig {
  float value;
  float min = -100.0f;
  float max = 100.0f;
  bool show_config = false;
};

static std::unordered_map<std::string, VariableConfig> variable_configs;

class RenderLayer : public Layer {
  public:
  void OnAttach() override { std::cout << "RenderLayer::OnAttach" << std::endl; }

  void OnUIRender() override {
    ImGui::PushFont(Application::Get().value()->GetFont("RobotoTitle"));
    ImGui::Text("FS24 WASM Emulator");
    ImGui::PushFont(Application::Get().value()->GetFont("RobotoRegular"));
    ImGui::Spacing();
    ImGui::Text("Emulation at its finest");
    ImGui::PopFont();
    ImGui::Separator();
    ImGui::PopFont();
    static std::string selected_file;
    FileDialog::ShowFileDialogButton("Open File", selected_file);
    ImGui::SameLine();
    auto file_name = FileDialog::GetFileName(selected_file);

    if (GaugeLoader::GetInstance()->IsUpdateQueued()) {
      if (!selected_file.empty()) {
        try {
          GaugeLoader::GetInstance()->GetOrLoadGauge(selected_file, file_name);

        } catch (const std::exception &e) {
          std::cerr << "Error loading gauge: " << e.what() << std::endl;
        }
        GaugeLoader::GetInstance()->SetUpdateQueued(false);
      }
    }
    ImGui::SameLine();
    if (ImGui::Button("Load Gauge")) {
      if (!selected_file.empty()) {
        try {
          GaugeLoader::GetInstance()->GetOrLoadGauge(selected_file, file_name);

        } catch (const std::exception &e) {
          std::cerr << "Error loading gauge: " << e.what() << std::endl;
        }
      }
    }
    ImGui::SameLine();
    if (ImGui::Button("Unload All Gauges")) {
      if (GaugeLoader::GetInstance()->AreGaugesLoaded()) {
        if (auto result = GaugeLoader::GetInstance()->UnloadAllGauges(); !result.has_value()) {
          std::cerr << "Error unloading gauges: " << result.error() << std::endl;
        }
      }
    }
    ImGui::Text(selected_file.c_str());
    ImGui::Begin("SimVars");
    ImGui::Text("SimVar Name    |     Value");

    auto gauge_loader = GaugeLoader::GetInstance();
    auto variables = gauge_loader->GetVariables();
    int index = 0;

    for (const auto &variable: variables) {
      const std::string &name = variable.first;
      float current_value = variable.second;

      auto &config = variable_configs[name];
      config.value = current_value;

      std::string slider_label = "##Slider" + name;
      std::string button_label = "Config##" + name;

      ImGui::Text(name.c_str());
      ImGui::SliderFloat(slider_label.c_str(), &config.value, config.min, config.max, "%.3f");
      ImGui::SameLine();

      if (ImGui::Button(button_label.c_str())) {
        config.show_config = !config.show_config;
      }

      if (config.show_config) {
        std::string config_window_label = "Config##Window_" + name;
        ImGui::Begin(config_window_label.c_str(), &config.show_config);
        ImGui::InputFloat("Min", &config.min);
        ImGui::InputFloat("Max", &config.max);
        ImGui::End();
      }

      ImGui::Separator();
      gauge_loader->UpdateVariable(index, config.value);
      index++;
    }

    ImGui::End();
  }

  void OnDetach() override {}

  void OnUpdate(float ts) override { GaugeLoader::GetInstance()->UpdateGauges(ts); }
};

int EntryPoint(const int argc, char **argv) {
  auto app = Application::CreateApplication(argc, argv, std::make_unique<RenderLayer>());
  if (!app) {
    std::cerr << "Failed to create application" << std::endl;
    return -1;
  }
  app->Run();
}

extern "C" void Linkage() { std::cerr << "WARNING: Dummy Linkage() called (this may cause issues!)" << std::endl; }

int main(const int argc, char **argv) { EntryPoint(argc, argv); }
