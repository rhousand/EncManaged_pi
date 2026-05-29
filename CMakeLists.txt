cmake_minimum_required(VERSION 3.18)
project(encmanaged_pi VERSION 0.1.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(wxWidgets REQUIRED COMPONENTS core base net)
find_package(CURL REQUIRED)

# Sets wx include dirs + platform defines (__WXMAC__, WXUSINGDLL, etc.)
# needed for compilation. wx libs are NOT linked into the plugin — OpenCPN
# provides all wx symbols at runtime from its bundled copy. Linking the nix
# wx alongside OpenCPN's bundled wx loads two wx instances → crash on init.
include(${wxWidgets_USE_FILE})

include_directories(
    include
    third_party
    ${CURL_INCLUDE_DIRS}
)

set(PLUGIN_SOURCES
    src/enc_managed_pi.cpp
    src/pref_dialog.cpp
    src/sync_engine.cpp
    src/catalog.cpp
    src/state.cpp
    src/downloader.cpp
    third_party/tinyxml2/tinyxml2.cpp
)

add_library(encmanaged_pi SHARED ${PLUGIN_SOURCES})

# OpenCPN's bundled wx is a Release build — no wxTheAssertHandler exported.
# Suppress wx assertion handler references so flat-namespace dlopen succeeds.
target_compile_definitions(encmanaged_pi PRIVATE wxDEBUG_LEVEL=0)

target_link_libraries(encmanaged_pi PRIVATE
    ${CURL_LIBRARIES}
)

set_target_properties(encmanaged_pi PROPERTIES
    PREFIX ""
    OUTPUT_NAME "encmanaged_pi"
)

# OpenCPN host resolves these symbols at load time, not link time
if(APPLE)
    target_link_options(encmanaged_pi PRIVATE -undefined dynamic_lookup)
elseif(UNIX)
    target_link_options(encmanaged_pi PRIVATE -Wl,--allow-shlib-undefined)
endif()

include(cmake/PluginInstall.cmake)

# --- Standalone test drivers (no OpenCPN needed) ---
foreach(test catalog state downloader)
    add_executable(test_${test} tests/test_${test}.cpp)
    if(test STREQUAL "catalog")
        target_sources(test_${test} PRIVATE src/catalog.cpp third_party/tinyxml2/tinyxml2.cpp)
    elseif(test STREQUAL "state")
        target_sources(test_${test} PRIVATE src/state.cpp)
    elseif(test STREQUAL "downloader")
        target_sources(test_${test} PRIVATE
            src/catalog.cpp src/downloader.cpp src/state.cpp
            third_party/tinyxml2/tinyxml2.cpp)
        target_link_libraries(test_${test} PRIVATE ${wxWidgets_LIBRARIES})
    endif()
    target_include_directories(test_${test} PRIVATE include third_party)
    target_link_libraries(test_${test} PRIVATE ${CURL_LIBRARIES})
    target_compile_features(test_${test} PRIVATE cxx_std_17)
endforeach()
