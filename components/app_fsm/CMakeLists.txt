# app_fsm.c is pure C (host-testable); app_fsm_task.c is the FreeRTOS runner.
# Depends only on interfaces: protocol, relay.h, link_if.h.
idf_component_register(SRCS "app_fsm.c" "app_fsm_task.c"
                       INCLUDE_DIRS "include"
                       REQUIRES protocol relay link_if freertos esp_common)
