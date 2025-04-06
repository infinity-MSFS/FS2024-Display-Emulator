#include <iostream>

#include "Application/Application.hpp"
#include "Application/Layer.hpp"
#include "FileDialog/FileDialog.hpp"
#include "GaugeLoader/GaugeLoader.hpp"

class RenderLayer : public Layer {
  public:
  void OnAttach() override { std::cout << "RenderLayer::OnAttach" << std::endl; }

  void OnUIRender() override {
    ImGui::Text("Render Layer");
    static std::string selected_file;
    FileDialog::ShowFileDialogButton("Open File", selected_file);
    auto file_name = FileDialog::GetFileName(selected_file);
    ImGui::Text("%s", file_name.c_str());
    if (ImGui::Button("Load Gauge")) {
      if (!selected_file.empty()) {
        try {
          GaugeLoader::GetInstance()->GetOrLoadGauge(selected_file, file_name);

        } catch (const std::exception &e) {
          std::cerr << "Error loading gauge: " << e.what() << std::endl;
        }
      }
    }
    if (ImGui::Button("Unload All Gauges")) {
      if (GaugeLoader::GetInstance()->AreGaugesLoaded()) {
        if (auto result = GaugeLoader::GetInstance()->UnloadAllGauges(); !result.has_value()) {
          std::cerr << "Error unloading gauges: " << result.error() << std::endl;
        }
      }
    }
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
