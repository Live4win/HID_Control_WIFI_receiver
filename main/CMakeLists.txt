idf_component_register(SRCS "esp32_nat_router.c"
                            "http_server.c"
                            "main.c"
                            "ble_hidd_demo_main.c"
                            "esp_hidd_prf_api.c"
                            "hid_device_le_prf"
                            "hid_dev.c"
                            "hid_app_control.c"
                            "io_hardware.c"
                            "ws2812.c"
                        INCLUDE_DIRS "."
                        EMBED_TXTFILES ${project_dir}/server_certs/certs.pem)

if(GCC_NOT_5_2_0)
    target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-unused-const-variable)
endif()
