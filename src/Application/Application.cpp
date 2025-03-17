#include "Application.hpp"

#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"

#include <chrono>
#include <mutex>
#include <thread>

constexpr int FPS_CAP = 144;
constexpr double FRAME_DURATION = 1.0 / FPS_CAP;

Application *Application::s_Instance = nullptr;

Application::Application(const ApplicationSpecifications &specifications)
    : m_Specification(specifications), m_Window(nullptr) {
  if (auto result = Init(); !result.has_value()) {
    result.error().Dispatch();
  }
  s_Instance = this;
}

Application::~Application() {
  if (auto result = Shutdown(); !result.has_value()) {
    result.error().Dispatch();
  }
  s_Instance = nullptr;
}

std::optional<Application *> Application::Get() {
  if (s_Instance == nullptr) {
    return std::nullopt;
  }
  return s_Instance;
}
void Application::GLFWErrorCallback(int error, const char *description) {
  std::cerr << "GLFW Error: " << error << ": " << description << "\n";
}

std::expected<void, Application::Error> Application::Init() {
  glfwSetErrorCallback(GLFWErrorCallback);

  if (!glfwInit()) {
    return std::unexpected(Error("Failed to initialize GLFW"));
  }

  const auto version = SetupGLVersion();

  m_Window =
      glfwCreateWindow(static_cast<int>(m_Specification.window_size.first),
                       static_cast<int>(m_Specification.window_size.second),
                       m_Specification.name.c_str(), nullptr, nullptr);

  if (m_Window == nullptr) {
    return std::unexpected(Error("Failed to create window"));
  }

  glfwMakeContextCurrent(m_Window);
  glfwSwapInterval(1);

  if (glewInit() != GLEW_OK) {
    return std::unexpected(Error("Failed to initialize GLEW"));
  }

  ImGui::CreateContext();
  ImGuiIO &io = ImGui::GetIO();
  (void)io;
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
  io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

  ImGui::StyleColorsDark();

  ImGui_ImplGlfw_InitForOpenGL(m_Window, true);
  ImGui_ImplOpenGL3_Init(version);

  // TODO: fonts

  return {};
}

const char *Application::SetupGLVersion() {
#if defined(IMGUI_IMPL_OPENGL_ES2)
  // GL ES 2.0 + GLSL 100
  const char *glsl_version = "#version 100";
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
  glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
#elif defined(__APPLE__)
  // GL 3.2 + GLSL 150
  const char *glsl_version = "#version 150";
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); // 3.2+ only
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // Required on Mac
#else
  // GL 3.0 + GLSL 130
  const char *glsl_version = "#version 130";
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
  // glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+
  // only glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // 3.0+ only
#endif

  return glsl_version;
}

std::expected<void, Application::Error> Application::Shutdown() {

  m_Layer->OnDetach();

  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();

  glfwDestroyWindow(m_Window);
  glfwTerminate();

  m_Running = false;

  return {};
}

std::expected<void, Application::Error> Application::Run() {
  m_Running = true;

  const auto clear_color = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
  ImGuiIO &io = ImGui::GetIO();

  io.IniFilename = nullptr;

  while (!glfwWindowShouldClose(m_Window) && m_Running) {
    const double start_time = glfwGetTime();
    glfwPollEvents();
    {
      std::scoped_lock lock(m_EventQueueMutex);

      while (!m_EventQueue.empty()) {
        auto &func = m_EventQueue.front();
        func();
        m_EventQueue.pop();
      }
    }
    m_Layer->OnUpdate(m_TimeStep);

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    {
      ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDocking;

      const ImGuiViewport *viewport = ImGui::GetMainViewport();
      const ImVec2 windowPos = viewport->Pos;
      ImGui::SetNextWindowPos(ImVec2(windowPos.x - 1, windowPos.y));
      ImGui::SetNextWindowSize(ImVec2(viewport->Size.x + 1, viewport->Size.y));
      ImGui::SetNextWindowViewport(viewport->ID);
      ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 4.0f);
      ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
      ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
      window_flags |= ImGuiWindowFlags_NoTitleBar |
                      ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize |
                      ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBackground;
      window_flags |=
          ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

      ImGui::Begin("DockSpaceWindow", nullptr, window_flags);

      m_Layer->OnUIRender();

      ImGui::PopStyleVar(3);

      ImGui::End();
    }

    ImGui::Render();

    int display_w, display_h;
    glfwGetFramebufferSize(m_Window, &display_w, &display_h);
    glViewport(0, 0, display_w, display_h);
    glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    glfwSwapBuffers(m_Window);

    const float time = GetTime();
    m_FrameTime = time - m_LastFrameTime;
#ifdef INFINITY_WINDOWS
    m_TimeStep = min(m_FrameTime, 0.0333f);
#else
    m_TimeStep = std::min(m_FrameTime, 0.0333f);
#endif
    m_LastFrameTime = time;

    const double endTime = glfwGetTime();

    if (const double frameTime = endTime - start_time;
        frameTime < FRAME_DURATION) {
      std::this_thread::sleep_for(
          std::chrono::duration<double>(FRAME_DURATION - frameTime));
    }
  }
  return {};
}

std::unique_ptr<Application>
Application::CreateApplication(int argc, char **argv,
                               std::unique_ptr<Layer> layer) {
  const auto specifications =
      ApplicationSpecifications{"WASM Emulator",
                                std::make_pair(1440, 1026),
                                std::make_pair(3840, 2160),
                                std::make_pair(1240, 680),
                                true,
                                false};
  auto app = std::make_unique<Application>(specifications);
  app->PushLayer(std::move(layer));

  return app;
}

void Application::Close() { m_Running = false; }
ImFont *Application::GetFont(const std::string &name) {
  if (!s_Instance->m_Fonts.contains(name)) {
    return nullptr;
  }
  return s_Instance->m_Fonts.at(name);
}
float Application::GetTime() { return static_cast<float>(glfwGetTime()); }
