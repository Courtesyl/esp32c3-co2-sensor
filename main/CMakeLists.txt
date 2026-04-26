idf_component_register(SRCS "scd41_drv.c" "main.c"
                    INCLUDE_DIRS "."
                    REQUIRES driver nvs_flash esp_lcd esp_lcd_st7735)