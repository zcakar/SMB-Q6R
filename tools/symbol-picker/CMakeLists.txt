cmake_minimum_required(VERSION 3.16)
project(symbol_picker
        VERSION 0.2.0
        DESCRIPTION "SMB-Q6R PLC symbol picker (CodeSys-style tag selector)"
        LANGUAGES C CXX)

# Host-side dev tool. Never cross-compiled for the pendant.
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# Qt5 first (matches pendant target Qt 5.15), fall back to Qt6 on hosts
# that don't have Qt5.
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets Concurrent)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets Concurrent)

# open62541 — same version as the pendant. The picker uses it from the
# Live-PLC source to browse the server directly, mirroring doBrowse().
# Leave most features at their defaults; turning methodcalls off in
# v1.3.10 trips -Werror=unused-function on an internal helper, so we
# keep that on even though the picker doesn't issue Call requests.
include(FetchContent)
set(UA_NAMESPACE_ZERO   "REDUCED" CACHE STRING "" FORCE)
set(UA_BUILD_EXAMPLES    OFF      CACHE BOOL   "" FORCE)
set(UA_BUILD_TOOLS       OFF      CACHE BOOL   "" FORCE)
set(UA_FORCE_WERROR      OFF      CACHE BOOL   "" FORCE)
FetchContent_Declare(
    open62541
    GIT_REPOSITORY https://github.com/open62541/open62541.git
    GIT_TAG        v1.3.10
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(open62541)

add_executable(symbol_picker
    main.cpp
    PickerWindow.cpp
    PickerWindow.h
    BrowseTreeModel.cpp
    BrowseTreeModel.h
    BrowseJsonLoader.cpp
    BrowseJsonLoader.h
    LivePlcSource.cpp
    LivePlcSource.h
    CodesysXmlSource.cpp
    CodesysXmlSource.h
    SymbolsJsonWriter.cpp
    SymbolsJsonWriter.h
)

target_link_libraries(symbol_picker PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Concurrent
    open62541::open62541
)

# Default install / output location is the build dir; launcher script
# scripts/pick-symbols.sh invokes the binary from there.
