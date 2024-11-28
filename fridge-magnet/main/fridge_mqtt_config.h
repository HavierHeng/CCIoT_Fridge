#ifndef DEMO_CONFIG_H_  // Header guard
#define DEMO_CONFIG_H_

#ifndef MQTT_IOT_ENDPOINT
    #define MQTT_IOT_ENDPOINT CONFIG_MQTT_BROKER_ENDPOINT
#endif

    
#ifndef MQTT_BROKER_PORT
    #define MQTT_BROKER_PORT ( CONFIG_MQTT_BROKER_PORT )
#endif

// Username value for auth client to MQTT broker when username/password based client auth is used. For this project, will be using client cert/private key though.
#ifndef CLIENT_IDENTIFER
    #define CLIENT_IDENTIFER CONFIG_MQTT_CLIENT_IDENTIFIER
#endif

#define NETWORK_BUFFER_SIZE       ( CONFIG_MQTT_NETWORK_BUFFER_SIZE )

#ifndef MQTT_AUDIO_TRIGGER_TOPIC
    #define MQTT_AUDIO_TRIGGER_TOPIC CONFIG_MQTT_AUDIO_TRIGGER_TOPIC
#endif

#ifndef MQTT_AUDIO_SIGNED_URL_TOPIC
    #define MQTT_AUDIO_SIGNED_URL_TOPIC CONFIG_MQTT_AUDIO_SIGNED_URL_TOPIC
#endif 

#ifndef MQTT_TTS_AUDIO_SUB_TOPIC
    #define MQTT_TTS_AUDIO_SUB_TOPIC CONFIG_MQTT_TTS_AUDIO_SUB_TOPIC 
#endif 

#ifndef MQTT_STT_SUB_TOPIC
    #define MQTT_STT_SUB_TOPIC CONFIG_MQTT_STT_SUB_TOPIC
#endif


/**
 * @brief The name of the operating system that the application is running on.
 * The current value is given as an example. Please update for your specific
 * operating system.
 */
#define OS_NAME                   "FreeRTOS"

/**
 * @brief The version of the operating system that the application is running
 * on. The current value is given as an example. Please update for your specific
 * operating system version.
 */
#define OS_VERSION                tskKERNEL_VERSION_NUMBER

/**
 * @brief The name of the hardware platform the application is running on. The
 * current value is given as an example. Please update for your specific
 * hardware platform.
 */
#define HARDWARE_PLATFORM_NAME    CONFIG_HARDWARE_PLATFORM_NAME

#endif
