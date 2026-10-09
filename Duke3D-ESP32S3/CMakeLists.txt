# Duke Nukem 3D for ESP32-S3 boards (ESP-IDF project).
# Board: -DDUKE_BOARD=BOARD_TDECK (default) or -DDUKE_BOARD=BOARD_WIO_TRACKER_L2,
# e.g. idf.py -B build-l2 -DDUKE_BOARD=BOARD_WIO_TRACKER_L2 build
cmake_minimum_required(VERSION 3.16.0)
if(NOT DEFINED DUKE_BOARD)
    set(DUKE_BOARD BOARD_TDECK)
endif()
set(DUKE_BOARD ${DUKE_BOARD} CACHE STRING "Target board")
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(duke3d_esp32s3)
