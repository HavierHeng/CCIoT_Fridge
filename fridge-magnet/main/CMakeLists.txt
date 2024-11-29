set(COMPONENT_SRCS
                    "fridge_magnet.c"
                    "mqtt_auth.c"
)

set(COMPONENT_ADD_INCLUDEDIRS
                    "."
                    "${CMAKE_CURRENT_LIST_DIR}"
)

set(REQUIRE_COMPONENTS
                    "esp_wifi"
                    "esp_event"
)

set(PRIV_REQUIRE_COMPONENTS
                    "nvs_flash"
                    "mqtt"
)

idf_component_register(SRCS "${COMPONENT_SRCS}"
                    INCLUDE_DIRS "${COMPONENT_ADD_INCLUDEDIRS}"
                    REQUIRES "${REQUIRE_COMPONENTS}"
                    PRIV_REQUIRES "${PRIV_REQUIRE_COMPONENTS}"
)
