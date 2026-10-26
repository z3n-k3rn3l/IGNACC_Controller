# relay_gpio.c is pure C; pin_hal_esp.c is the only file touching the GPIO driver.
idf_component_register(SRCS "relay_gpio.c" "pin_hal_esp.c"
                       INCLUDE_DIRS "include"
                       PRIV_REQUIRES esp_driver_gpio)
