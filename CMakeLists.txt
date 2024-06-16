cmake_minimum_required(VERSION 3.20)
set(CMAKE_CXX_STANDARD 20)
project(nexus_hover_infos)

include_directories(include)
set(BUILD_SHARED_LIBS OFF)
set(BUILD_STATIC_LIBS ON)

include(FetchContent)
FetchContent_Declare(cpr GIT_REPOSITORY https://github.com/libcpr/cpr.git
        GIT_TAG 3b15fa82ea74739b574d705fea44959b58142eb8) # Replace with your desired git commit from: https://github.com/libcpr/cpr/releases
FetchContent_MakeAvailable(cpr)

add_library(imgui STATIC src/imgui/imgui.cpp src/imgui/imgui_demo.cpp src/imgui/imgui_draw.cpp src/imgui/imgui_tables.cpp src/imgui/imgui_widgets.cpp)

add_library(nexus_hover_infos SHARED src/main.cpp src/addon/Addon.cpp src/addon/Shared.cpp src/addon/Utils.cpp)
target_link_libraries(nexus_hover_infos imgui cpr::cpr)