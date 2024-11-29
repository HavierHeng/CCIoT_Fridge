/* MQTT Mutual Authentication Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "esp_wifi.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "protocol_examples_common.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"

#include "lwip/sockets.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"

#include "esp_log.h"
#include "mqtt_client.h"
#include "fridge_mqtt_config.h"


#ifndef CLIENT_IDENTIFIER
    #error "Please define a unique client identifier, CLIENT_IDENTIFIER, in menuconfig"
#endif

extern const char client_cert_start[] asm("_binary_client_crt_start");
extern const char client_cert_end[] asm("_binary_client_crt_end");
extern const char client_key_start[] asm("_binary_client_key_start");
extern const char client_key_end[] asm("_binary_client_key_end");
extern const char root_cert_auth_start[] asm("_binary_root_cert_auth_crt_start");
extern const char root_cert_auth_end[] asm("_binary_root_cert_auth_crt_end");



#ifdef MQTT_AUDIO_TRIGGER_TOPIC
    #define MQTT_AUDIO_TRIGGER_TOPIC_CLIENT CLIENT_IDENTIFIER MQTT_AUDIO_TRIGGER_TOPIC
#else
    #error "Please define a topic to publish data to get a signed URL to."
#endif

#ifdef MQTT_AUDIO_SIGNED_URL_TOPIC
    #define MQTT_AUDIO_SIGNED_URL_TOPIC_CLIENT CLIENT_IDENTIFIER MQTT_AUDIO_SIGNED_URL_TOPIC
#else
    #error "Please define a topic to get signed URL to send audio to."
#endif 

#ifdef MQTT_TTS_AUDIO_SUB_TOPIC
    #define MQTT_TTS_AUDIO_SUB_TOPIC_CLIENT CLIENT_IDENTIFIER MQTT_TTS_AUDIO_SUB_TOPIC
#else
    #error "Please define a topic to GET audio files from."
#endif 

#ifdef MQTT_STT_SUB_TOPIC
    #define MQTT_STT_SUB_TOPIC_CLIENT CLIENT_IDENTIFIER MQTT_STT_SUB_TOPIC
#else 
    #error "Please define a topic to get detected user text from."
#endif

// Helper macro to stringify non-string values (for example, MQTT_BROKER_PORT)
#define STRINGIFY(x) #x

// Create the MQTT broker address macro
#define MQTT_BROKER_ADDRESS ("mqtts://" STRINGIFY(MQTT_IOT_ENDPOINT) ":" STRINGIFY(MQTT_BROKER_PORT))

static const char *TAG = "MQTT_EXAMPLE";

static void log_error_if_nonzero(const char *message, int error_code)
{
    if (error_code != 0) {
        ESP_LOGE(TAG, "Last error %s: 0x%x", message, error_code);
    }
}

/*
 * @brief Event handler registered to receive MQTT events
 *
 *  This function is called by the MQTT client event loop.
 *
 * @param handler_args user data registered to the event.
 * @param base Event base for the handler(always MQTT Base in this example).
 * @param event_id The id for the received event.
 * @param event_data The data for the event, esp_mqtt_event_handle_t.
 */
static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%" PRIi32, base, event_id);
    esp_mqtt_event_handle_t event = event_data;
    esp_mqtt_client_handle_t client = event->client;
    int msg_id;
    switch ((esp_mqtt_event_id_t)event_id) {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
        msg_id = esp_mqtt_client_subscribe(client, MQTT_AUDIO_SIGNED_URL_TOPIC_CLIENT, 1);
        ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);

        msg_id = esp_mqtt_client_subscribe(client, MQTT_TTS_AUDIO_SUB_TOPIC_CLIENT, 1);
        ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);

        msg_id = esp_mqtt_client_subscribe(client, MQTT_STT_SUB_TOPIC_CLIENT, 1);
        ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);

        break;
    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
        break;

    case MQTT_EVENT_SUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
        msg_id = esp_mqtt_client_publish(client, MQTT_AUDIO_TRIGGER_TOPIC_CLIENT, "fuck off", 0, 1, 0);
        ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);
        break;
    case MQTT_EVENT_UNSUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_PUBLISHED:
        ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_DATA:
        ESP_LOGI(TAG, "MQTT_EVENT_DATA");
        printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
        printf("DATA=%.*s\r\n", event->data_len, event->data);
        break;
    case MQTT_EVENT_ERROR:
        ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
        if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT) {
            log_error_if_nonzero("reported from esp-tls", event->error_handle->esp_tls_last_esp_err);
            log_error_if_nonzero("reported from tls stack", event->error_handle->esp_tls_stack_err);
            log_error_if_nonzero("captured as transport's socket errno",  event->error_handle->esp_transport_sock_errno);
            ESP_LOGI(TAG, "Last errno string (%s)", strerror(event->error_handle->esp_transport_sock_errno));
        }
        break;
    default:
        ESP_LOGI(TAG, "Other event id:%d", event->event_id);
        break;
    }
}

void mqtt_app_start(void)
{
  const esp_mqtt_client_config_t mqtt_cfg = {
    .broker.address.uri = MQTT_BROKER_ADDRESS,
    .broker.verification.certificate = (const char *)root_cert_auth_start,
    .credentials = {
      .authentication = {
        .certificate = (const char *)client_cert_start,
        .key = (const char *)client_key_start,
      },
    }
  };

    ESP_LOGI(TAG, "[APP] Free memory: %" PRIu32 " bytes", esp_get_free_heap_size());
    esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_cfg);
    /* The last argument may be used to pass data to the event handler, in this example mqtt_event_handler */
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(client);
}

