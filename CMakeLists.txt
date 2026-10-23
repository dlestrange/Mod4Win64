cmake_minimum_required(VERSION 3.25)
project(Mod4Win64 VERSION 0.2.0 LANGUAGES CXX)

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
    src/ModuleAudio.cpp
    src/ModuleAudio.h
)

target_link_libraries(Mod4Win64 PRIVATE Qt6::Widgets Qt6::Multimedia)
set_target_properties(Mod4Win64 PROPERTIES WIN32_EXECUTABLE TRUE)
