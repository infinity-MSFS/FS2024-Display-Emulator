cmake_minimum_required(VERSION 3.30)
include(FetchContent)

if (WIN32)
    set(VCPKG_TRIPLET "x64-windows-static")
else ()
    set(VCPKG_TRIPLET "x64-linux-static")
endif ()
if (NOT VCPKG_ROOT)
    set(VCPKG_ROOT $ENV{VCPKG_ROOT})
endif ()
if (VCPKG_ROOT)
    set(CMAKE_TOOLCHAIN_FILE "${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")
    message("${Blue}Using Toolchain file: ${Green}${CMAKE_TOOLCHAIN_FILE}${ColorReset}")
else ()
    message(FATAL_ERROR "VCPKG_ROOT is not set in the environment!")
endif ()


project(FS2024_WASM_Emulator)

FetchContent_Declare(Infinity GIT_REPOSITORY https://github.com/infinity-MSFS/InfinityLauncherDependencies.git)
FetchContent_MakeAvailable(Infinity)

set(CMAKE_CXX_STANDARD 26)

find_package(OpenGL REQUIRED)
find_package(CURL REQUIRED)
find_package(GLEW REQUIRED)
find_package(nanovg CONFIG REQUIRED)
set(GLFW_BUILD_WAYLAND ON)
find_package(glfw3 CONFIG REQUIRED)


add_executable(FS2024_WASM_Emulator src/main.cpp
        src/FsShims/FsVars.cpp
        src/FsShims/FsVars.hpp
        src/FsShims/FsCore.hpp
        src/Application/Application.cpp
        src/Application/Application.hpp
        src/Application/Layer.hpp
        src/GaugeLoader/GaugeLoader.cpp
        src/GaugeLoader/GaugeLoader.hpp
        src/FileDialog/FileDialog.hpp
        src/FsShims/FsStructs.hpp)

file(GLOB IMGUI_SOURCES
        ${infinity_SOURCE_DIR}/src/imgui/
        ${infinity_SOURCE_DIR}/src/imgui/imgui.cpp
        ${infinity_SOURCE_DIR}/src/imgui/imgui_draw.cpp
        ${infinity_SOURCE_DIR}/src/imgui/imgui_widgets.cpp
        ${infinity_SOURCE_DIR}/src/imgui/imgui_tables.cpp
        ${infinity_SOURCE_DIR}/src/imgui/misc/cpp/imgui_stdlib.cpp
        ${infinity_SOURCE_DIR}/src/imgui/backends/imgui_impl_glfw.cpp
        ${infinity_SOURCE_DIR}/src/imgui/backends/imgui_impl_opengl3.cpp
)
target_sources(FS2024_WASM_Emulator PRIVATE ${IMGUI_SOURCES})

target_include_directories(FS2024_WASM_Emulator PRIVATE ${infinity_SOURCE_DIR}/src/imgui src)
target_link_options(FS2024_WASM_Emulator PRIVATE -rdynamic)

target_link_libraries(FS2024_WASM_Emulator PRIVATE OpenGL::GL glfw CURL::libcurl nanovg::nanovg GLEW::GLEW)