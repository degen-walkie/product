# Degen Walkie firmware (ESP32-S3). Build: idf.py set-target esp32s3 && idf.py build
# The browser simulator builds the same core with tools/build_sim.py.
cmake_minimum_required(VERSION 3.16)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
# LVGL reads our lv_conf.h (CONFIG_LV_CONF_SKIP is off in sdkconfig.defaults).
idf_build_set_property(COMPILE_OPTIONS "-I${CMAKE_CURRENT_LIST_DIR}/core" APPEND)
project(degen_walkie)
