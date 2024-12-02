set(REQUIRE_COMPONENTS
                    "esp-wifi"
)

idf_component_register(SRCS "mic-test-standalone.c"
                    INCLUDE_DIRS ".")
