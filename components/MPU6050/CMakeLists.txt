idf_component_register(SRCS "MPU6050.c"
                    INCLUDE_DIRS "include"
                    REQUIRES mySerial driver esp_driver_i2c)
