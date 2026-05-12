# Vendor HWInterfaceDemo applications wrapped in our CMake pipeline.
#
# Each demo is a tiny Qt 5 Widgets app that exercises one peripheral. We
# cross-compile them with the same Focal-arm64 toolchain as smb_q6r so the
# resulting binaries can run beside ours on the pendant and act as oracles
# during hardware bring-up (see .ai/PHASE1_TEST_PLAN.md).
#
# Layout per demo (vendor original):
#   vendor-demos/<NAME>/<NAME>/{main.cpp, mainwindow.cpp, mainwindow.h,
#                               mainwindow.ui, icon.qrc, <NAME>.pro}
#
# We ignore .pro (qmake) and replicate the build with this loop.

find_package(Qt5 5.12 REQUIRED COMPONENTS Core Gui Widgets)

# Vendor demos and where each one lives under vendor-demos/.
set(VENDOR_DEMO_NAMES
    led
    button
    matrixKeys
    pwm
    rotaryEncoder
    wheel
    backlight
)

set(VENDOR_OUT_DIR "${CMAKE_BINARY_DIR}/vendor")

foreach(demo IN LISTS VENDOR_DEMO_NAMES)
    set(src_dir "${CMAKE_CURRENT_SOURCE_DIR}/${demo}/${demo}")
    string(TOLOWER "${demo}" target)
    set(target "vendor_${target}")

    add_executable(${target}
        ${src_dir}/main.cpp
        ${src_dir}/mainwindow.cpp
        ${src_dir}/mainwindow.h
        ${src_dir}/mainwindow.ui
        ${src_dir}/icon.qrc
    )
    target_link_libraries(${target} PRIVATE Qt5::Core Qt5::Gui Qt5::Widgets)
    target_include_directories(${target} PRIVATE ${src_dir})

    set_target_properties(${target} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${VENDOR_OUT_DIR}"
        AUTOMOC ON
        AUTOUIC ON
        AUTORCC ON
    )
    target_compile_options(${target} PRIVATE
        -Wno-unused-parameter -Wno-shadow -Wno-conversion -Wno-unused-result
        # Vendor's backlight demo calls open(path, O_RDWR|O_CREAT) without a
        # mode argument; FORTIFY_SOURCE=2 in modern glibc catches this as a
        # hard error. Disable FORTIFY for vendor sources only — we don't want
        # to patch upstream so traceability stays intact.
        -U_FORTIFY_SOURCE
    )

    # Post-build: produce a stripped copy ready for scp.
    add_custom_command(TARGET ${target} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
            $<TARGET_FILE:${target}> $<TARGET_FILE:${target}>.stripped
        COMMAND ${CMAKE_STRIP} --strip-all $<TARGET_FILE:${target}>.stripped
        VERBATIM
    )
endforeach()

message(STATUS "Vendor demos: ${VENDOR_DEMO_NAMES}")
message(STATUS "  output dir: ${VENDOR_OUT_DIR}")
