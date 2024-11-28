set(COMPONENT_SRCS
                    "fridge-magnet.c"
                    "mqtt_auth.c"
)

set(COMPONENT_ADD_INCLUDEDIRS
                    "."
                    "${CMAKE_CURRENT_LIST_DIR}"
)

idf_component_register(SRCS "${COMPONENTS_SRCS}"
                    INCLUDE_DIRS ${COMPONENT_ADD_INCLUDEDIRS}
                    )
