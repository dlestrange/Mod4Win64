cmake_minimum_required(VERSION 3.25)
project(Mod4Win64 VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 6.5 REQUIRED COMPONENTS Widgets)

qt_add_executable(Mod4Win64 WIN32
    src/main.cpp
    src/PlayerWindow.cpp
    src/PlayerWindow.h
)

target_link_libraries(Mod4Win64 PRIVATE Qt6::Widgets)
set_target_properties(Mod4Win64 PROPERTIES
    WIN32_EXECUTABLE TRUE
)
