#pragma once
#include <cstdlib>
#include <imgui.h>
#include <string>

class FileDialog {
public:
  static std::string OpenFileDialog() {
    const char *command =
        "zenity --file-selection "
        "--file-filter='Shared libraries (*.so) | *.so *.so.*' "
        "--title='Select SO MSFS Gauge' 2>/dev/null";

    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(command, "r"), pclose);
    if (!pipe) {
      return "";
    }

    char buffer[128];
    std::string result;
    while (fgets(buffer, sizeof(buffer), pipe.get()) != nullptr) {
      result += buffer;
    }

    result.erase(std::remove(result.begin(), result.end(), '\n'), result.end());

    return result;
  }

  static void ShowFileDialogButton(const char *buttonLabel,
                                   std::string &selectedFile) {
    if (ImGui::Button(buttonLabel)) {
      selectedFile = OpenFileDialog();
    }
    ImGui::SameLine();
    ImGui::TextUnformatted(selectedFile.c_str());
  }

  static std::string GetFileName(const std::string &file_path) {
    size_t lastSlash = file_path.find_last_of('/');
    std::string filename = (lastSlash != std::string::npos)
                               ? file_path.substr(lastSlash + 1)
                               : file_path;

    size_t soPos = filename.find(".so");

    return (soPos != std::string::npos) ? filename.substr(0, soPos) : filename;
  }
};