cmake_minimum_required(VERSION 3.16)
project(screen_light_adjust VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

set(CMAKE_PREFIX_PATH "/home/azzato/Programs/Qt/6.10.2/gcc_64")

find_package(Qt6 REQUIRED COMPONENTS Widgets)

add_executable(screen_light_adjust
    main.cpp
    color_temp.h color_temp.cpp
    monitor_ctrl.h monitor_ctrl.cpp
    mainwindow.h mainwindow.cpp
)

target_link_libraries(screen_light_adjust PRIVATE Qt6::Widgets)
