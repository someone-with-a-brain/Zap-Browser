cmake_minimum_required(VERSION 3.22)
project(ZapBrowser VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(WIN32)
    # Prevent windows.h from defining min/max macros that clash with STL
    add_definitions(-DNOMINMAX)
endif()

# ─── Output Directories ──────────────────────────────────────────────────────
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
# ─── Qt6 ─────────────────────────────────────────────────────────────────────
find_package(Qt6 REQUIRED COMPONENTS Core Widgets Network)

# ─── CEF ─────────────────────────────────────────────────────────────────────
# CEF's cmake files have a guard: cef_variables.cmake must be loaded via
# find_package(CEF), NOT via include() directly.
#
# CEF cmake file lookup order:
#   1. CMake searches CMAKE_MODULE_PATH for FindCEF.cmake
#   2. FindCEF.cmake sets _CEF_ROOT_EXPLICIT then includes cef_variables.cmake
#   3. cef_variables.cmake sets CEF_FOUND, CEF_BINARY_DIR, etc.
#
set(CEF_ROOT "${CMAKE_SOURCE_DIR}/third_party/cef" CACHE PATH "CEF root directory")

if(EXISTS "${CEF_ROOT}/cmake/FindCEF.cmake")
    # Add CEF's cmake/ to module path so find_package can find FindCEF.cmake
    list(APPEND CMAKE_MODULE_PATH "${CEF_ROOT}/cmake")
    find_package(CEF REQUIRED)

    # Build the CEF C++ wrapper library (required by all CEF apps)
    # CEF_LIBCEF_DLL_WRAPPER_PATH is set by FindCEF / cef_variables.cmake
    add_subdirectory("${CEF_LIBCEF_DLL_WRAPPER_PATH}" libcef_dll_wrapper)

    message(STATUS "CEF found: ${CEF_VERSION}")
    set(CEF_FOUND TRUE)
else()
    message(WARNING
        "CEF not found at ${CEF_ROOT}. "
        "Run scripts/download_cef.ps1 or place the extracted CEF SDK at ${CEF_ROOT}. "
        "Compiling in stub mode (no web rendering).")
    set(CEF_FOUND FALSE)
endif()

# ─── SQLite3 ─────────────────────────────────────────────────────────────────
find_package(SQLite3 QUIET)
if(NOT SQLite3_FOUND)
    # Fallback: compile from source in third_party
    if(EXISTS "${CMAKE_SOURCE_DIR}/third_party/sqlite/sqlite3.c")
        add_library(sqlite3_local STATIC
            third_party/sqlite/sqlite3.c
        )
        target_include_directories(sqlite3_local PUBLIC third_party/sqlite)
        set(SQLITE3_LIB sqlite3_local)
    else()
        message(STATUS "SQLite3 not found — history features disabled")
        set(SQLITE3_LIB "")
    endif()
else()
    set(SQLITE3_LIB SQLite::SQLite3)
endif()

# ─── nlohmann/json ───────────────────────────────────────────────────────────
if(EXISTS "${CMAKE_SOURCE_DIR}/third_party/nlohmann_json/json.hpp")
    set(JSON_INCLUDE "${CMAKE_SOURCE_DIR}/third_party/nlohmann_json")
else()
    find_package(nlohmann_json QUIET)
    if(nlohmann_json_FOUND)
        set(JSON_INCLUDE "")
    else()
        message(STATUS "nlohmann/json not found — downloading...")
        file(DOWNLOAD
            "https://github.com/nlohmann/json/releases/download/v3.11.3/json.hpp"
            "${CMAKE_SOURCE_DIR}/third_party/nlohmann_json/json.hpp"
        )
        set(JSON_INCLUDE "${CMAKE_SOURCE_DIR}/third_party/nlohmann_json")
    endif()
endif()

# ─── Source Files ────────────────────────────────────────────────────────────
set(ZAP_SOURCES
    src/main.cpp

    src/app/ZapApp.h
    src/app/ZapApp.cpp
    src/app/ZapClient.h
    src/app/ZapClient.cpp

    src/ui/MainWindow.h
    src/ui/MainWindow.cpp
    src/ui/TabBar.h
    src/ui/TabBar.cpp
    src/ui/AddressBar.h
    src/ui/AddressBar.cpp
    src/ui/AISidebar.h
    src/ui/AISidebar.cpp
    src/ui/SettingsPage.h
    src/ui/SettingsPage.cpp

    src/privacy/TorManager.h
    src/privacy/TorManager.cpp
    src/privacy/DohResolver.h
    src/privacy/DohResolver.cpp
    src/privacy/BlockList.h
    src/privacy/BlockList.cpp
    src/privacy/CookieIsolator.h
    src/privacy/CookieIsolator.cpp

    src/ai/OllamaBridge.h
    src/ai/OllamaBridge.cpp
    src/ai/PageSummarizer.h
    src/ai/PageSummarizer.cpp

    src/data/HistoryDB.h
    src/data/HistoryDB.cpp
    src/data/BookmarkStore.h
    src/data/BookmarkStore.cpp

    src/update/AutoUpdater.h
    src/update/AutoUpdater.cpp
)

# ─── Executable ──────────────────────────────────────────────────────────────
add_executable(zap WIN32 ${ZAP_SOURCES})

target_include_directories(zap PRIVATE
    src/
    ${JSON_INCLUDE}
)

target_link_libraries(zap PRIVATE
    Qt6::Core
    Qt6::Widgets
    Qt6::Network
    ${SQLITE3_LIB}
)

# ─── CEF Linking (when available) ────────────────────────────────────────────
if(CEF_FOUND)
    target_compile_definitions(zap PRIVATE ZAP_CEF_ENABLED)

    # CEF include root (for #include "include/cef_app.h" etc.)
    # We apply this to both zap and the generated autogen files
    target_include_directories(zap PRIVATE "${CEF_ROOT}")
    set_target_properties(zap PROPERTIES AUTOMOC_MOC_OPTIONS "-D_UNICODE;-DUNICODE")

    # Link the wrapper (builds from source) + the main CEF shared library
    # CEF_STANDARD_LIBS is set by cef_variables.cmake (e.g. libcef.lib on Win)
    target_link_libraries(zap PRIVATE
        libcef_dll_wrapper
        ${CEF_STANDARD_LIBS}
    )

    # Copy CEF runtime binaries (libcef.dll, etc.) to output directory.
    # COPY_FILES is a macro from cef_macros.cmake loaded by find_package(CEF).
    COPY_FILES(zap "${CEF_BINARY_FILES}"   "${CEF_BINARY_DIR}"   "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
    COPY_FILES(zap "${CEF_RESOURCE_FILES}" "${CEF_RESOURCE_DIR}" "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
endif()

# ─── Copy Assets ─────────────────────────────────────────────────────────────
add_custom_command(TARGET zap POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/assets"
        "$<TARGET_FILE_DIR:zap>/assets"
    COMMENT "Copying ZAP assets..."
)

# ─── Windows: Copy Qt DLLs ───────────────────────────────────────────────────
if(WIN32)
    add_custom_command(TARGET zap POST_BUILD
        COMMAND Qt6::windeployqt "$<TARGET_FILE:zap>"
        COMMENT "Running windeployqt..."
    )
endif()

# ─── Compiler Flags ──────────────────────────────────────────────────────────
if(MSVC)
    target_compile_options(zap PRIVATE /W3 /utf-8)
else()
    target_compile_options(zap PRIVATE -Wall -Wextra -Wpedantic)
endif()

message(STATUS "=== ZAP Browser Build Configuration ===")
message(STATUS "  C++ Standard : ${CMAKE_CXX_STANDARD}")
message(STATUS "  CEF Found    : ${CEF_FOUND}")
message(STATUS "  Qt6 Found    : ${Qt6_FOUND}")
message(STATUS "  Output Dir   : ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
message(STATUS "========================================")
