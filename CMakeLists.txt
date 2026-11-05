cmake_minimum_required(VERSION 3.25)
project(Mod4Win64 VERSION 0.37.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 6.5 REQUIRED COMPONENTS Widgets Multimedia)

qt_add_executable(Mod4Win64 WIN32
    src/main.cpp
    src/ClassicWindow.cpp
    src/ClassicWindow.h
    src/ClassicDisplay.cpp
    src/ClassicDisplay.h
    src/SetupDialog.cpp
    src/SetupDialog.h
    src/EffectsCanvas.cpp
    src/EffectsCanvas.h
    src/EffectsWindow.cpp
    src/EffectsWindow.h
    src/ModuleAudio.cpp
    src/ModuleAudio.h
)

target_link_libraries(Mod4Win64 PRIVATE Qt6::Widgets Qt6::Multimedia)
set_target_properties(Mod4Win64 PROPERTIES WIN32_EXECUTABLE TRUE)

# Windows executable icon: generated from our own retro green 64 design.
if(WIN32)
    enable_language(RC)
    find_program(MOD4WIN64_POWERSHELL NAMES powershell.exe REQUIRED)
    set(APP_ICON_PATH "${CMAKE_CURRENT_BINARY_DIR}/Mod4Win64.ico")
    add_custom_command(
        OUTPUT "${APP_ICON_PATH}"
        COMMAND "${MOD4WIN64_POWERSHELL}" -NoProfile -ExecutionPolicy Bypass
                -File "${CMAKE_CURRENT_SOURCE_DIR}/scripts/generate-icon.ps1"
                -Output "${APP_ICON_PATH}"
        DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/scripts/generate-icon.ps1"
        VERBATIM
        COMMENT "Generating Mod4Win64 icon")
    add_custom_target(mod4win64_icon DEPENDS "${APP_ICON_PATH}")
    add_dependencies(Mod4Win64 mod4win64_icon)
    configure_file("${CMAKE_CURRENT_SOURCE_DIR}/resources/app.rc.in"
                   "${CMAKE_CURRENT_BINARY_DIR}/Mod4Win64.rc" @ONLY)
    target_sources(Mod4Win64 PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/Mod4Win64.rc")
endif()
