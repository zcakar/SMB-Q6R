# CMake toolchain file for cross-compiling SMB-Q6R from x86_64 Ubuntu host
# to aarch64 target (HN00-09Q6 teach pendant, Rockchip RK3568).
#
# Requires:
#   * Ubuntu host with `arm64` architecture enabled via dpkg --add-architecture
#   * Packages: crossbuild-essential-arm64, qtbase5-dev:arm64, qtdeclarative5-dev:arm64
#   * Run scripts/setup-host-cross-toolchain.sh once to install the above.

set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR aarch64)

set(CMAKE_C_COMPILER   aarch64-linux-gnu-gcc)
set(CMAKE_CXX_COMPILER aarch64-linux-gnu-g++)

# Multiarch layout: arm64 libs live alongside amd64 under /usr/lib/aarch64-linux-gnu/.
# No traditional sysroot needed; just point CMake at the multiarch directories.
set(CMAKE_FIND_ROOT_PATH /usr/lib/aarch64-linux-gnu /usr/aarch64-linux-gnu /usr)

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)   # use host's binaries (qmake-tools, moc generator)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

# Qt5 CMake config for arm64 lives here on Noble.
list(APPEND CMAKE_PREFIX_PATH
    /usr/lib/aarch64-linux-gnu/cmake
    /usr/lib/aarch64-linux-gnu/cmake/Qt5
)

# Help pkg-config find arm64 .pc files.
set(ENV{PKG_CONFIG_LIBDIR} /usr/lib/aarch64-linux-gnu/pkgconfig:/usr/share/pkgconfig)
set(ENV{PKG_CONFIG_SYSROOT_DIR} "")

# Hardening / ABI flags matched to Cortex-A53.
set(CMAKE_C_FLAGS_INIT   "-march=armv8-a -mtune=cortex-a53")
set(CMAKE_CXX_FLAGS_INIT "-march=armv8-a -mtune=cortex-a53")
