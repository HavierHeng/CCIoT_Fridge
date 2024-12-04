set(REQUIRE_COMPONENTS
                    "esp_wifi"
                    "esp_event"
                    "sdmmc"
                    "esp_http_client"
                    "nvs_flash"
                    "json"
                    "esp32-HD44780"
                    "fatfs"
)

idf_component_register(SRCS "s3-bucket-upload-download-test.c"
                    REQUIRES "${REQUIRE_COMPONENTS}"
                    INCLUDE_DIRS ".")


