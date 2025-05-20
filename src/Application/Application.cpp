#include "Application.hpp"

#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"
#include "nanovg.h"
#define NANOVG_GL3_IMPLEMENTATION
#include <chrono>
#include <mutex>
#include <thread>

#include "GaugeLoader/GaugeLoader.hpp"
#include "nanovg_gl.h"

constexpr int FPS_CAP = 144;
constexpr double FRAME_DURATION = 1.0 / FPS_CAP;

#include "Roboto-Regular.h"

Application *Application::s_Instance = nullptr;

Application::Application(const ApplicationSpecifications &specifications)
    : m_Specification(specifications)
    , m_Window(nullptr) {
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

  m_Window = glfwCreateWindow(static_cast<int>(m_Specification.window_size.first),
                              static_cast<int>(m_Specification.window_size.second), m_Specification.name.c_str(),
                              nullptr, nullptr);

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
  (void) io;
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
  io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

  ImGui::StyleColorsDark();

  ImGuiStyle &style = ImGui::GetStyle();
  ImVec4 *colors = style.Colors;

  // Primary background
  colors[ImGuiCol_WindowBg] = ImVec4(0.07f, 0.07f, 0.09f, 1.00f);  // #131318
  colors[ImGuiCol_MenuBarBg] = ImVec4(0.12f, 0.12f, 0.15f, 1.00f);  // #131318

  colors[ImGuiCol_PopupBg] = ImVec4(0.18f, 0.18f, 0.22f, 1.00f);

  // Headers
  colors[ImGuiCol_Header] = ImVec4(0.18f, 0.18f, 0.22f, 1.00f);
  colors[ImGuiCol_HeaderHovered] = ImVec4(0.30f, 0.30f, 0.40f, 1.00f);
  colors[ImGuiCol_HeaderActive] = ImVec4(0.25f, 0.25f, 0.35f, 1.00f);

  // Buttons
  colors[ImGuiCol_Button] = ImVec4(0.20f, 0.22f, 0.27f, 1.00f);
  colors[ImGuiCol_ButtonHovered] = ImVec4(0.30f, 0.32f, 0.40f, 1.00f);
  colors[ImGuiCol_ButtonActive] = ImVec4(0.35f, 0.38f, 0.50f, 1.00f);

  // Frame BG
  colors[ImGuiCol_FrameBg] = ImVec4(0.15f, 0.15f, 0.18f, 1.00f);
  colors[ImGuiCol_FrameBgHovered] = ImVec4(0.22f, 0.22f, 0.27f, 1.00f);
  colors[ImGuiCol_FrameBgActive] = ImVec4(0.25f, 0.25f, 0.30f, 1.00f);

  // Tabs
  colors[ImGuiCol_Tab] = ImVec4(0.18f, 0.18f, 0.22f, 1.00f);
  colors[ImGuiCol_TabHovered] = ImVec4(0.35f, 0.35f, 0.50f, 1.00f);
  colors[ImGuiCol_TabActive] = ImVec4(0.25f, 0.25f, 0.38f, 1.00f);
  colors[ImGuiCol_TabUnfocused] = ImVec4(0.13f, 0.13f, 0.17f, 1.00f);
  colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.20f, 0.20f, 0.25f, 1.00f);

  // Title
  colors[ImGuiCol_TitleBg] = ImVec4(0.12f, 0.12f, 0.15f, 1.00f);
  colors[ImGuiCol_TitleBgActive] = ImVec4(0.15f, 0.15f, 0.20f, 1.00f);
  colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.10f, 0.10f, 0.12f, 1.00f);

  // Borders
  colors[ImGuiCol_Border] = ImVec4(0.20f, 0.20f, 0.25f, 0.50f);
  colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);

  // Text
  colors[ImGuiCol_Text] = ImVec4(0.90f, 0.90f, 0.95f, 1.00f);
  colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.55f, 1.00f);

  // Highlights
  colors[ImGuiCol_CheckMark] = ImVec4(0.50f, 0.70f, 1.00f, 1.00f);
  colors[ImGuiCol_SliderGrab] = ImVec4(0.50f, 0.70f, 1.00f, 1.00f);
  colors[ImGuiCol_SliderGrabActive] = ImVec4(0.60f, 0.80f, 1.00f, 1.00f);
  colors[ImGuiCol_ResizeGrip] = ImVec4(0.50f, 0.70f, 1.00f, 0.50f);
  colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.60f, 0.80f, 1.00f, 0.75f);
  colors[ImGuiCol_ResizeGripActive] = ImVec4(0.70f, 0.90f, 1.00f, 1.00f);

  // Scrollbar
  colors[ImGuiCol_ScrollbarBg] = ImVec4(0.10f, 0.10f, 0.12f, 1.00f);
  colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.30f, 0.30f, 0.35f, 1.00f);
  colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.40f, 0.40f, 0.50f, 1.00f);
  colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.45f, 0.45f, 0.55f, 1.00f);

  // Style tweaks
  style.WindowRounding = 5.0f;
  style.FrameRounding = 5.0f;
  style.GrabRounding = 5.0f;
  style.TabRounding = 5.0f;
  style.PopupRounding = 5.0f;
  style.ScrollbarRounding = 5.0f;
  style.WindowPadding = ImVec2(10, 10);
  style.FramePadding = ImVec2(6, 4);
  style.ItemSpacing = ImVec2(8, 6);
  style.PopupBorderSize = 0.f;

  ImGui_ImplGlfw_InitForOpenGL(m_Window, true);
  ImGui_ImplOpenGL3_Init(version);

  ImFontConfig font_config;
  font_config.FontDataOwnedByAtlas = false;
  ImFont *roboto = io.Fonts->AddFontFromMemoryTTF(g_RobotoRegular, sizeof(g_RobotoRegular), 20.0f, &font_config);

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
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);  // Required on Mac
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

    ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDocking;

    const ImGuiViewport *viewport = ImGui::GetMainViewport();
    const ImVec2 windowPos = viewport->Pos;
    ImGui::SetNextWindowPos(ImVec2(windowPos.x - 1, windowPos.y));
    ImGui::SetNextWindowSize(ImVec2(viewport->Size.x + 1, viewport->Size.y));
    ImGui::SetNextWindowViewport(viewport->ID);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(1.0f, 1.0f));
    window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBackground;
    window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

    ImGui::Begin("DockSpaceWindow", nullptr, window_flags);

    m_Layer->OnUIRender();

    for (auto &gauge: GaugeLoader::GetInstance()->GetAllRenderers()) {
      gauge.CreateImGuiWindow();
    }

    ImGui::PopStyleVar(3);
    ImGui::End();

    ImGui::Render();

    int display_w, display_h;
    glfwGetFramebufferSize(m_Window, &display_w, &display_h);
    glViewport(0, 0, display_w, display_h);
    glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
    glClear(GL_COLOR_BUFFER_BIT);

    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    for (auto &gauge: GaugeLoader::GetInstance()->GetAllRenderers()) {
      gauge.RenderContents();
    }

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

    if (const double frameTime = endTime - start_time; frameTime < FRAME_DURATION) {
      std::this_thread::sleep_for(std::chrono::duration<double>(FRAME_DURATION - frameTime));
    }
  }
  return {};
}

std::unique_ptr<Application> Application::CreateApplication(int argc, char **argv, std::unique_ptr<Layer> layer) {
  const auto specifications = ApplicationSpecifications{
      "WASM Emulator", std::make_pair(1440, 1026), std::make_pair(3840, 2160), std::make_pair(1240, 680), true, false};
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
