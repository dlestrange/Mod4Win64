cmake_minimum_required(VERSION 3.25)
project(Mod4Win64 VERSION 0.9.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 6.5 REQUIRED COMPONENTS Widgets Multimedia)

if(APPLE)
    set(MOD4_APP_NAME Mod4Mac64)
else()
    set(MOD4_APP_NAME Mod4Win64)
endif()

qt_add_executable(${MOD4_APP_NAME} WIN32 MACOSX_BUNDLE
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

target_link_libraries(${MOD4_APP_NAME} PRIVATE Qt6::Widgets Qt6::Multimedia)
if(WIN32)
    set_target_properties(${MOD4_APP_NAME} PROPERTIES WIN32_EXECUTABLE TRUE)
elseif(APPLE)
    set_target_properties(${MOD4_APP_NAME} PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_BUNDLE_NAME "Mod4Mac64"
        MACOSX_BUNDLE_GUI_IDENTIFIER "au.com.mod4win64.mod4mac64"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "0.9.0"
        MACOSX_BUNDLE_BUNDLE_VERSION "0.9.0"
    )
    target_compile_definitions(${MOD4_APP_NAME} PRIVATE MOD4MAC64=1)
endif()

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
    add_dependencies(${MOD4_APP_NAME} mod4win64_icon)
    configure_file("${CMAKE_CURRENT_SOURCE_DIR}/resources/app.rc.in"
                   "${CMAKE_CURRENT_BINARY_DIR}/Mod4Win64.rc" @ONLY)
    target_sources(${MOD4_APP_NAME} PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/Mod4Win64.rc")
endif()
