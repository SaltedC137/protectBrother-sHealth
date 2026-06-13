cmake_minimum_required(VERSION 3.16)
project(screen-light-adjust VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

set(CMAKE_PREFIX_PATH "/home/azzato/Programs/Qt/6.10.2/gcc_64")

find_package(Qt6 REQUIRED COMPONENTS Widgets)

# -------------------- Binary --------------------

set(QT_LIB_DIR "/home/azzato/Programs/Qt/6.10.2/gcc_64/lib")
set(QT_PLUGIN_DIR "/home/azzato/Programs/Qt/6.10.2/gcc_64/plugins")

add_executable(screen-light-adjust
    main.cpp
    color_temp.h color_temp.cpp
    monitor_ctrl.h monitor_ctrl.cpp
    mainwindow.h mainwindow.cpp
)

target_link_libraries(screen-light-adjust PRIVATE Qt6::Widgets)
set_target_properties(screen-light-adjust PROPERTIES
    INSTALL_RPATH "${QT_LIB_DIR}"
    BUILD_RPATH "${QT_LIB_DIR}"
)

# qt.conf tells Qt where to find plugins at runtime
configure_file(
    "${CMAKE_SOURCE_DIR}/resources/qt.conf"
    "${CMAKE_CURRENT_BINARY_DIR}/qt.conf" COPYONLY)

# -------------------- Install rules --------------------

set(CPACK_PACKAGE_NAME "screen-light-adjust")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Multi-monitor brightness and color temperature control via xrandr")

set(CPACK_DEBIAN_PACKAGE_MAINTAINER "user")
set(CPACK_DEBIAN_PACKAGE_SECTION "utils")
set(CPACK_DEBIAN_PACKAGE_PRIORITY "optional")
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS OFF)
set(CPACK_DEBIAN_PACKAGE_DEPENDS "x11-xserver-utils")

install(TARGETS screen-light-adjust RUNTIME DESTINATION /usr/local/bin)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/qt.conf"
        DESTINATION /usr/local/bin)
install(PROGRAMS "resources/screen-light-adjust.desktop"
        DESTINATION /usr/local/share/applications)
install(FILES "resources/screen-light-adjust.svg"
        DESTINATION /usr/local/share/icons/hicolor/scalable/apps)

configure_file("${CMAKE_SOURCE_DIR}/cmake/postinst.in"
    "${CMAKE_CURRENT_BINARY_DIR}/postinst" @ONLY)
configure_file("${CMAKE_SOURCE_DIR}/cmake/prerm.in"
    "${CMAKE_CURRENT_BINARY_DIR}/prerm" @ONLY)

set(CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA
    "${CMAKE_CURRENT_BINARY_DIR}/postinst;${CMAKE_CURRENT_BINARY_DIR}/prerm")

include(CPack)

# Convenience target: cmake --build --preset <name> --target deb
add_custom_target(deb
    COMMAND ${CMAKE_CPACK_COMMAND} -G DEB
    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
    COMMENT "Generating .deb package…")
