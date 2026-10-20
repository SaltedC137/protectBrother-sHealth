cmake_minimum_required(VERSION 3.16)
project(screen-light-adjust VERSION 1.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT "$ENV{QT_HOME_PATH}" STREQUAL "")
    list(APPEND CMAKE_PREFIX_PATH "$ENV{QT_HOME_PATH}")
    set(QT_PLUGIN_DIR "$ENV{QT_HOME_PATH}/plugins")
endif()

find_package(Qt6 REQUIRED COMPONENTS Widgets)
include(CTest)

if(BUILD_TESTING)
    find_package(Qt6 REQUIRED COMPONENTS Test)
endif()

# Keep the build tree usable with a non-system Qt installation. Installed
# packages use the target system's normal Qt plugin search path.
if(QT_PLUGIN_DIR)
    configure_file(resources/qt.conf "${CMAKE_CURRENT_BINARY_DIR}/qt.conf" @ONLY)
endif()

# -------------------- Binary --------------------

set(QT_LIB_DIR "$ENV{QT_HOME_PATH}/lib")

add_library(screen-light-adjust-core STATIC
    include/break_reminder.h
    include/break_timer.h
    include/color_temp.h
    include/mainwindow.h
    include/monitor_ctrl.h
    include/preset_manager.h
    include/settings_manager.h
    src/break_reminder.cpp
    src/break_timer.cpp
    src/color_temp.cpp
    src/mainwindow.cpp
    src/monitor_ctrl.cpp
    src/preset_manager.cpp
    src/settings_manager.cpp
)
target_include_directories(screen-light-adjust-core
    PUBLIC "${PROJECT_SOURCE_DIR}/include")
target_link_libraries(screen-light-adjust-core PUBLIC Qt6::Widgets)

add_executable(screen-light-adjust src/main.cpp)
target_link_libraries(screen-light-adjust PRIVATE screen-light-adjust-core)

set_target_properties(screen-light-adjust PROPERTIES
    INSTALL_RPATH ""
    BUILD_RPATH "${QT_LIB_DIR}"
)

# -------------------- Install rules --------------------

set(CPACK_PACKAGE_NAME "screen-light-adjust")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Multi-monitor display adjustment and scheduled break reminders")
set(CPACK_GENERATOR "DEB;RPM")
set(CPACK_PACKAGING_INSTALL_PREFIX "/usr")

set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Screen Light Adjust contributors")
set(CPACK_DEBIAN_PACKAGE_SECTION "utils")
set(CPACK_DEBIAN_PACKAGE_PRIORITY "optional")
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
set(CPACK_DEBIAN_PACKAGE_DEPENDS "x11-xserver-utils, redshift, gammastep, qt6-qpa-plugins, qt6-wayland")
set(CPACK_RPM_PACKAGE_REQUIRES "xrandr, redshift, gammastep, qt6-qtbase-gui, qt6-qtwayland")
set(CPACK_RPM_PACKAGE_AUTOREQ ON)

include(GNUInstallDirs)
install(TARGETS screen-light-adjust RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}")
install(FILES "resources/screen-light-adjust.desktop"
        DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/applications"
        PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ)
install(FILES "resources/screen-light-adjust.svg"
        DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/scalable/apps"
        PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ)

configure_file("${CMAKE_SOURCE_DIR}/cmake/postinst.in"
    "${CMAKE_CURRENT_BINARY_DIR}/postinst" @ONLY)
configure_file("${CMAKE_SOURCE_DIR}/cmake/prerm.in"
    "${CMAKE_CURRENT_BINARY_DIR}/prerm" @ONLY)

set(CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA
    "${CMAKE_CURRENT_BINARY_DIR}/postinst;${CMAKE_CURRENT_BINARY_DIR}/prerm")

include(CPack)

if(BUILD_TESTING)
    add_executable(screen-light-adjust-tests
        tests/screen_light_adjust_test.cpp
    )
    target_link_libraries(screen-light-adjust-tests PRIVATE screen-light-adjust-core Qt6::Test)
    add_test(NAME screen-light-adjust-tests COMMAND screen-light-adjust-tests)
    set_tests_properties(screen-light-adjust-tests PROPERTIES
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
endif()

# Convenience target: cmake --build --preset <name> --target deb
add_custom_target(deb
    COMMAND ${CMAKE_CPACK_COMMAND} -G DEB
    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
    COMMENT "Generating .deb package…")

add_custom_target(rpm
    COMMAND ${CMAKE_CPACK_COMMAND} -G RPM
    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
    COMMENT "Generating .rpm package…")
