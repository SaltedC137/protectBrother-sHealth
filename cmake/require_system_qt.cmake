if(NOT SCREEN_LIGHT_ADJUST_USE_SYSTEM_QT)
    message(FATAL_ERROR
        "DEB/RPM packages must be built with the distribution's Qt libraries. "
        "Configure a fresh build directory with -DSCREEN_LIGHT_ADJUST_USE_SYSTEM_QT=ON.")
endif()
