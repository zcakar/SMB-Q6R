cmake_minimum_required(VERSION 3.16)
project(smb_q6r
    VERSION 0.1.0
    DESCRIPTION "SMB-Q6R Teach Pendant UI for 6-axis Fanuc 6R robot"
    LANGUAGES CXX
)

# ---------- Build options ----------
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE RelWithDebInfo)
endif()

# Warnings as errors only for our own sources (suppressed for Qt-generated MOC).
add_compile_options(-Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wno-unused-parameter)

# ---------- Dependencies ----------
find_package(Qt5 5.12 REQUIRED COMPONENTS Core Quick QuickControls2 Qml Gui)

# ---------- Sources ----------
set(SMB_Q6R_SOURCES
    src/main.cpp
)

# Phase 1 will add: hwio.cpp, led_controller.cpp, buzzer_controller.cpp,
# switch_monitor.cpp, backlight_controller.cpp, system_info.cpp,
# diagnostics_model.cpp.

set(SMB_Q6R_RESOURCES
    qml/qml.qrc
)

add_executable(smb_q6r
    ${SMB_Q6R_SOURCES}
    ${SMB_Q6R_RESOURCES}
)

target_link_libraries(smb_q6r
    PRIVATE
        Qt5::Core
        Qt5::Gui
        Qt5::Qml
        Qt5::Quick
        Qt5::QuickControls2
)

target_include_directories(smb_q6r PRIVATE src)

# ---------- Install ----------
install(TARGETS smb_q6r RUNTIME DESTINATION bin)

# ---------- Diagnostics ----------
message(STATUS "SMB-Q6R build:")
message(STATUS "  Build type   : ${CMAKE_BUILD_TYPE}")
message(STATUS "  C++ compiler : ${CMAKE_CXX_COMPILER} (${CMAKE_CXX_COMPILER_VERSION})")
message(STATUS "  Target arch  : ${CMAKE_SYSTEM_PROCESSOR}")
message(STATUS "  Qt version   : ${Qt5Core_VERSION}")
