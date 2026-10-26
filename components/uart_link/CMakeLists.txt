# line_assembler.c is pure C; uart_link.c wraps the IDF UART driver.
idf_component_register(SRCS "line_assembler.c" "uart_link.c"
                       INCLUDE_DIRS "include"
                       REQUIRES link_if freertos esp_common
                       PRIV_REQUIRES esp_driver_uart)
