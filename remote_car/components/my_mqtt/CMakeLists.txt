idf_component_register(SRCS "my_mqtt.c"
                    INCLUDE_DIRS "include"
                    REQUIRES  "mqtt"  "json" "my_motor" "main")
