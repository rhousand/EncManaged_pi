if(APPLE)
    set(OPENCPN_PLUGIN_DIR
        "$ENV{HOME}/Library/Application Support/OpenCPN/Contents/PlugIns"
        CACHE PATH "")
    set(OPENCPN_DATA_DIR
        "$ENV{HOME}/Library/Application Support/OpenCPN/Contents/SharedSupport/encmanaged_pi"
        CACHE PATH "")
    set(OPENCPN_INSTDATA
        "$ENV{HOME}/Library/Preferences/opencpn/plugins/install_data")
elseif(UNIX)
    set(OPENCPN_PLUGIN_DIR
        "$ENV{HOME}/.local/share/opencpn/Contents/PlugIns"
        CACHE PATH "")
    set(OPENCPN_DATA_DIR
        "$ENV{HOME}/.local/share/opencpn/Contents/SharedSupport/encmanaged_pi"
        CACHE PATH "")
    set(OPENCPN_INSTDATA "$ENV{HOME}/.opencpn/plugins/install_data")
endif()

install(TARGETS encmanaged_pi
    LIBRARY DESTINATION "${OPENCPN_PLUGIN_DIR}")

install(DIRECTORY data/
    DESTINATION "${OPENCPN_DATA_DIR}")

# Register with OpenCPN's plugin manager so the plugin appears in Options > Plugins.
# OpenCPN's UpdateManagedPlugins() removes any unmanaged plugin (no catalog entry,
# not active) from its internal array before building the Plugins tab UI. Without
# these two files the plugin loads successfully but is silently dropped and never
# shown to the user.
install(FILES "${CMAKE_CURRENT_LIST_DIR}/encmanaged_import.xml"
    DESTINATION "${OPENCPN_INSTDATA}/imports"
    RENAME "encmanaged.xml")

install(CODE "
    file(WRITE
        \"${OPENCPN_INSTDATA}/encmanaged.files\"
        \"${OPENCPN_PLUGIN_DIR}/\n${OPENCPN_PLUGIN_DIR}/encmanaged_pi.dylib\n\")
    file(WRITE
        \"${OPENCPN_INSTDATA}/encmanaged.version\"
        \"${PROJECT_VERSION}\")
")
