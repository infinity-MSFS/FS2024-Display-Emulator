#include "Application/Application.hpp"
#include "Application/Layer.hpp"
#include "FileDialog/FileDialog.hpp"
#include "GaugeLoader/GaugeLoader.hpp"
#include <iostream>

class RenderLayer : public Layer {
public:
  void OnAttach() override {
    std::cout << "RenderLayer::OnAttach" << std::endl;
  }

  void OnUIRender() override {
    ImGui::Text("Render Layer");
    static std::string selected_file;
    FileDialog::ShowFileDialogButton("Open File", selected_file);
    auto file_name = FileDialog::GetFileName(selected_file);
    ImGui::Text("%s", file_name.c_str());
    if (ImGui::Button("Load Gauge")) {
      if (!selected_file.empty()) {
        try {
          auto gauge = GaugeLoader::GetInstance()->GetOrLoadGauge(selected_file,
                                                                  file_name);
          gauge.init(nullptr, nullptr);
        } catch (const std::exception &e) {
          std::cerr << "Error loading gauge: " << e.what() << std::endl;
        }
      }
    }
  }

  void OnDetach() override {}

  void OnUpdate(float ts) override {}
};

int EntryPoint(const int argc, char **argv) {
  auto app = Application::CreateApplication(argc, argv,
                                            std::make_unique<RenderLayer>());
  if (!app) {
    std::cerr << "Failed to create application" << std::endl;
    return -1;
  }
  app->Run();
}

extern "C" void Linkage() {
  std::cerr << "WARNING: Dummy Linkage() called (this may cause issues!)"
            << std::endl;
}

int main(const int argc, char **argv) { EntryPoint(argc, argv); }