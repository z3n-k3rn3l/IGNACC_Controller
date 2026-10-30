# IGNACC_Controller - ESP32 UART-controlled dual relay firmware
cmake_minimum_required(VERSION 3.22)

# Firmware version: the ONLY place it is defined.
# Read at runtime via esp_app_get_description()->version.
set(PROJECT_VER "1.1.0")

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(IGNACC_Controller)
