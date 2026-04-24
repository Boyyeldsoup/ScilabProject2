idf_component_register(SRCS "AnalogWrite.c"
                    INCLUDE_DIRS "include"
                    REQUIRES mySerial driver esp_driver_ledc)
