idf_component_register(SRCS "cmd_system.c"
                    INCLUDE_DIRS .
                    REQUIRES console spi_flash)

list(APPEND compile_options "-Wno-error=implicit-function-declaration")