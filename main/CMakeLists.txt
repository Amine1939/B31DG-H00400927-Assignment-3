idf_component_register(SRCS "workkernel.c" "gpio.c" "edge_counter.c" "tasks.c" "main.c" "monitor.c"
                    INCLUDE_DIRS "."
                    REQUIRES driver esp_timer)
