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
    # Default to Release for size-constrained target device (HN00-09Q6: 1.4 GB free disk).
    # Use -DCMAKE_BUILD_TYPE=Debug or RelWithDebInfo for host-side debugging.
    set(CMAKE_BUILD_TYPE Release)
endif()

# Size optimisation flags for Release; debug info kept in object files (host)
# but stripped from the deployed binary via the 'strip-deploy' target below.
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    add_compile_options(-Os -ffunction-sections -fdata-sections)
    add_link_options(-Wl,--gc-sections -Wl,--as-needed)
endif()

# Warnings as errors only for our own sources (suppressed for Qt-generated MOC).
add_compile_options(-Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wno-unused-parameter)

# ---------- Dependencies ----------
# Linked against device-side packages: libqt5{core,gui,qml,quick}5 only.
# We intentionally do NOT pull in QuickControls2 / Layouts / Window QML modules
# — those .deb's are not installed on the HN00-09Q6 system Qt 5.12.8. We build
# every UI primitive from QtQuick 2 (Item / Rectangle / Text / MouseArea).
find_package(Qt5 5.12 REQUIRED COMPONENTS Core Qml Quick Gui)

# ---------- Sources ----------
set(SMB_Q6R_SOURCES
    src/main.cpp
    src/hwio.cpp
    src/led_controller.cpp
    src/diagnostics_model.cpp
)

# Phase 1 (iter B+) will add: buzzer_controller.cpp, switch_monitor.cpp,
# backlight_controller.cpp, system_info.cpp.

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
)

target_include_directories(smb_q6r PRIVATE src)

# ---------- Install ----------
install(TARGETS smb_q6r RUNTIME DESTINATION bin)

# Produce a stripped copy ready for scp to the device (debug info stays in
# the original build-arm64/smb_q6r for host-side gdb/addr2line).
if(CMAKE_STRIP)
    set(STRIP_TOOL "${CMAKE_STRIP}")
else()
    set(STRIP_TOOL "${CMAKE_CXX_COMPILER_TARGET}-strip")
endif()
add_custom_command(TARGET smb_q6r POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
        $<TARGET_FILE:smb_q6r> $<TARGET_FILE:smb_q6r>.stripped
    COMMAND ${STRIP_TOOL} --strip-all $<TARGET_FILE:smb_q6r>.stripped
    COMMAND ${CMAKE_COMMAND} -E echo "Stripped deploy artifact -> $<TARGET_FILE:smb_q6r>.stripped"
    VERBATIM
)

# ---------- Diagnostics ----------
message(STATUS "SMB-Q6R build:")
message(STATUS "  Build type   : ${CMAKE_BUILD_TYPE}")
message(STATUS "  C++ compiler : ${CMAKE_CXX_COMPILER} (${CMAKE_CXX_COMPILER_VERSION})")
message(STATUS "  Target arch  : ${CMAKE_SYSTEM_PROCESSOR}")
message(STATUS "  Qt version   : ${Qt5Core_VERSION}")
