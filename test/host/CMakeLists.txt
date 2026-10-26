# Host unit tests for the pure-C parts of the firmware.
#
#   cmake -S test/host -B test/host/build -G "MinGW Makefiles"
#   cmake --build test/host/build
#   ctest --test-dir test/host/build --output-on-failure
#
# Unity is taken from ESP-IDF ($IDF_PATH) unless -DUNITY_DIR=... is given.
cmake_minimum_required(VERSION 3.16)
project(ignacc_host_tests C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)  # for clangd code navigation

get_filename_component(ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../.." ABSOLUTE)
set(COMP "${ROOT}/components")

if(NOT UNITY_DIR)
    if(NOT DEFINED ENV{IDF_PATH})
        message(FATAL_ERROR "Set IDF_PATH or pass -DUNITY_DIR=<path to unity/src>")
    endif()
    set(UNITY_DIR "$ENV{IDF_PATH}/components/unity/unity/src")
endif()

add_library(unity STATIC "${UNITY_DIR}/unity.c")
target_include_directories(unity PUBLIC "${UNITY_DIR}")

enable_testing()

function(add_unit_test name)
    add_executable(${name} ${ARGN})
    target_include_directories(${name} PRIVATE
        "${COMP}/protocol/include"
        "${COMP}/link_if/include"
        "${COMP}/relay/include"
        "${COMP}/uart_link/include"
        "${COMP}/app_fsm/include")
    target_compile_options(${name} PRIVATE -Wall -Wextra -Werror)
    target_link_libraries(${name} PRIVATE unity)
    add_test(NAME ${name} COMMAND ${name})
endfunction()

add_unit_test(test_protocol       test_protocol.c       "${COMP}/protocol/protocol.c")
add_unit_test(test_line_assembler test_line_assembler.c "${COMP}/uart_link/line_assembler.c")
add_unit_test(test_relay_gpio     test_relay_gpio.c     "${COMP}/relay/relay_gpio.c")
add_unit_test(test_app_fsm        test_app_fsm.c        "${COMP}/app_fsm/app_fsm.c" "${COMP}/protocol/protocol.c")
