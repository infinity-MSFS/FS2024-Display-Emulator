#pragma once
#define GLFW_INCLUDE_NONE
#define GLFW_EXPOSE_NATIVE_WAYLAND

#include <expected>
#include <functional>
#include <imgui_internal.h>
#include <iostream>
#include <memory>
#include <optional>
#include <queue>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "GL/glew.h"
//
#include "GL/gl.h"
#include "GLFW/glfw3.h"
#include "Layer.hpp"
#include "imgui.h"

struct NVGcontext;

class Application {
  struct ApplicationSpecifications {
    std::string name;
    std::pair<uint32_t, uint32_t> window_size;
    std::pair<uint32_t, uint32_t> max_size;
    std::pair<uint32_t, uint32_t> min_size;
    bool resizable;
    bool custom_titlebar;
  };

  struct Error {
    std::string message;

    explicit Error(const std::string &msg)
        : message(msg) {}

    void Dispatch() const { std::cerr << "Error: " << message << std::endl; }
  };

  public:
  explicit Application(const ApplicationSpecifications &specifications);
  ~Application();

  static std::unique_ptr<Application> CreateApplication(int argc, char **argv, std::unique_ptr<Layer> layer);

  template<typename T>
  void PushLayer() {
    static_assert(std::is_base_of_v<Layer, T>, "T must derive from Layer");
    m_Layer = std::make_shared<T>();
    m_Layer->OnAttach();
  }

  void PushLayer(const std::shared_ptr<Layer> &layer) { m_Layer = layer; }

  GLFWwindow *GetHandle() { return m_Window; }

  std::expected<void, Error> Run();

  void Close();

  [[nodiscard]] static float GetTime();

  static ImFont *GetFont(const std::string &name);

  template<typename F>
  void QueueEvent(F &&func) {
    m_EventQueue.push(func);
  }

  static std::optional<Application *> Get();

  ApplicationSpecifications GetSpecifications() { return m_Specification; }

  private:
  std::expected<void, Error> Init();
  static const char *SetupGLVersion();
  std::expected<void, Error> Shutdown();

  static void GLFWErrorCallback(int error, const char *description);

  private:
  ApplicationSpecifications m_Specification;
  static Application *s_Instance;
  GLFWwindow *m_Window;
  // NVGcontext *m_NVGContext = nullptr;

  std::unordered_map<std::string, ImFont *> m_Fonts;

  bool m_Running = true;

  float m_TimeStep = 0.0f;
  float m_FrameTime = 0.0f;
  float m_LastFrameTime = 0.0f;

  std::shared_ptr<Layer> m_Layer;

  std::mutex m_EventQueueMutex;
  std::queue<std::function<void()>> m_EventQueue;
};
