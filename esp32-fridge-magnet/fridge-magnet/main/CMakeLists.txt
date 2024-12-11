set(COMPONENT_SRCS
                    "fridge_magnet.c"
                    "mqtt/mqtt_auth.c"
                    "sd_card/sd_card.c"
                    "speaker/speaker.c"
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
                    "fatfs"
                    "driver"
                    "sdmmc"
)

idf_component_register(SRCS "${COMPONENT_SRCS}"
                    INCLUDE_DIRS "${COMPONENT_ADD_INCLUDEDIRS}"
                    REQUIRES "${REQUIRE_COMPONENTS}"
                    PRIV_REQUIRES "${PRIV_REQUIRE_COMPONENTS}"
)
