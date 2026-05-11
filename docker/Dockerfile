# SMB-Q6R cross-compile builder image.
#
# Provides Ubuntu 20.04 Focal (glibc 2.31, matches teach pendant device)
# with arm64 multiarch + Qt 5.12.8 dev headers, so host-side cross-compile
# produces binaries that load cleanly against the device's system Qt 5.12.8
# without sourcing the vendor's qt_env.sh override.

FROM ubuntu:20.04

ARG DEBIAN_FRONTEND=noninteractive

# Replace stock sources.list with explicit per-arch entries. Focal main
# archive does not host arm64 — those come from ports.ubuntu.com.
RUN dpkg --add-architecture arm64 && \
    rm /etc/apt/sources.list && \
    printf '%s\n' \
        'deb [arch=amd64] http://archive.ubuntu.com/ubuntu/ focal main restricted universe multiverse' \
        'deb [arch=amd64] http://archive.ubuntu.com/ubuntu/ focal-updates main restricted universe multiverse' \
        'deb [arch=amd64] http://archive.ubuntu.com/ubuntu/ focal-backports main restricted universe multiverse' \
        'deb [arch=amd64] http://security.ubuntu.com/ubuntu focal-security main restricted universe multiverse' \
        '' \
        'deb [arch=arm64] http://ports.ubuntu.com/ubuntu-ports/ focal main restricted universe multiverse' \
        'deb [arch=arm64] http://ports.ubuntu.com/ubuntu-ports/ focal-updates main restricted universe multiverse' \
        'deb [arch=arm64] http://ports.ubuntu.com/ubuntu-ports/ focal-backports main restricted universe multiverse' \
        'deb [arch=arm64] http://ports.ubuntu.com/ubuntu-ports/ focal-security main restricted universe multiverse' \
        > /etc/apt/sources.list && \
    apt-get update && \
    apt-get install -y --no-install-recommends \
        crossbuild-essential-arm64 \
        cmake \
        ninja-build \
        pkg-config \
        qtbase5-dev:arm64 \
        qtbase5-dev-tools \
        qtdeclarative5-dev:arm64 \
        qtquickcontrols2-5-dev:arm64 \
        qml-module-qtquick-controls2:arm64 \
        qml-module-qtquick-window2:arm64 \
        qml-module-qtquick-layouts:arm64 \
        qml-module-qtquick2:arm64 \
        libqt5quickcontrols2-5:arm64 \
        ca-certificates && \
    rm -rf /var/lib/apt/lists/*

WORKDIR /src
ENTRYPOINT ["/bin/bash", "-l", "-c"]
CMD ["echo 'smb-q6r builder image ready. Mount source at /src and invoke cmake/ninja.'"]
