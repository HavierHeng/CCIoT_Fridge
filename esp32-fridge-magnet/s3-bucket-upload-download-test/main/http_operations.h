#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "esp_err.h"
#include "esp_http_client.h"
#include "esp_log.h"
#include "cJSON.h"
#include "lwip/def.h"
#include "freertos/FreeRTOS.h"

#define MAX_HTTP_RECV_BUFFER 1024 
#define MAX_HTTP_OUTPUT_BUFFER 1024

static const char *TAG = "HTTP_CLIENT";

// Structure to maintain state across callbacks
typedef struct {
    FILE* file;
    size_t total_bytes;
    size_t current_offset;
} download_state_t;


SemaphoreHandle_t download_semaphore;

esp_err_t init_semaphore() {
        // Create a semaphore for synchronization
    download_semaphore = xSemaphoreCreateBinary();
    if (download_semaphore == NULL) {
        ESP_LOGE(TAG, "Failed to create semaphore");
        return ESP_FAIL;
    }
    xSemaphoreGive(download_semaphore);
    return ESP_OK;
}

// Function to parse MQTT URL and return protocol, hostname, and path as a heap-allocated char**
   /*
    {
      "protocol": "https",
      "hostname": "fridge-bucket-a8e757a7-74fc-48fd-9391-ee3bd3ebf30e.s3.amazonaws.com",
      "path": "/audio/expiry/95491898-a0e3-4857-8b9f-79bedc06f217.wav"
    }
    */
  
char **parse_mqtt_url(const char* json_info) {
    ESP_LOGI(TAG, "Parsing Json_info %s", json_info);
    const cJSON *protocol = NULL;
    const cJSON *hostname = NULL;
    const cJSON *path = NULL;
    
    char **result = NULL;
    
    // Parse the JSON string
    cJSON *mqtt_json = cJSON_Parse(json_info);
    if (mqtt_json == NULL) {
        const char *error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL) {
            ESP_LOGE(TAG, "Error parsing JSON before: %s\n", error_ptr);
        }
        goto end;
    }

    // Extract the protocol, hostname, and path from the JSON
    protocol = cJSON_GetObjectItemCaseSensitive(mqtt_json, "protocol");
    hostname = cJSON_GetObjectItemCaseSensitive(mqtt_json, "hostname");
    path = cJSON_GetObjectItemCaseSensitive(mqtt_json, "path");

    // Allocate memory for the result array (char**)
    result = (char **)malloc(3 * sizeof(char *));
    if (result == NULL) {
        ESP_LOGE(TAG, "Memory allocation for result array failed");
        goto end;
    }

    // Initialize the result array to NULL
    result[0] = result[1] = result[2] = NULL;

    // Allocate and copy strings to heap if they exist
    if (protocol && cJSON_IsString(protocol)) {
        result[0] = strdup(protocol->valuestring);  // heap allocated strings, store into heap allocated pointers
        ESP_LOGI(TAG, "protocol: %s", protocol->valuestring);
        if (result[0] == NULL) {
            ESP_LOGE(TAG, "Memory allocation for protocol failed");
            goto end;
        }
    }

    if (hostname && cJSON_IsString(hostname)) {
        result[1] = strdup(hostname->valuestring);
        ESP_LOGI(TAG, "hostname: %s", hostname->valuestring);
        if (result[1] == NULL) {
            ESP_LOGE(TAG, "Memory allocation for hostname failed");
            goto end;
        }
    }

    if (path && cJSON_IsString(path)) {
        result[2] = strdup(path->valuestring);
        ESP_LOGI(TAG, "path: %s", path->valuestring);
        if (result[2] == NULL) {
            ESP_LOGE(TAG, "Memory allocation for path failed");
            goto end;
        }
    }

end:
    // Clean up the cJSON object
    if (mqtt_json != NULL) {
        cJSON_Delete(mqtt_json);
    }

    // In case of failure, free the allocated memory
    if (result == NULL || result[0] == NULL || result[1] == NULL || result[2] == NULL) {
        if (result) {
            for (int i = 0; i < 3; i++) {
                if (result[i] != NULL) {
                    free(result[i]);
                }
            }
            free(result);
        }
        result = NULL;
    }

    return result;
}

void free_mqtt_url(char **result) {
    if (result != NULL) {
        // Free the individual strings first
        if (result[0] != NULL) free(result[0]);  // Free the protocol string
        if (result[1] != NULL) free(result[1]);  // Free the hostname string
        if (result[2] != NULL) free(result[2]);  // Free the path string

        // Finally, free the array of pointers itself
        free(result);
    }
}



esp_err_t download_event_handler(esp_http_client_event_t *evt) {
    // Get our state structure from user_data
    download_state_t* state = (download_state_t*)evt->user_data;
    
    switch(evt->event_id) {
        case HTTP_EVENT_ON_HEADER:
            // Check content length when headers arrive
            if (strcmp(evt->header_key, "Content-Length") == 0) {
                state->total_bytes = atoi(evt->header_value);
                ESP_LOGI(TAG, "File size will be: %d bytes", (int) state->total_bytes);
            }
            break;
            
        case HTTP_EVENT_ON_DATA:
            // Write data and update our offset
            if (state->file) {
                fwrite(evt->data, 1, evt->data_len, state->file);
                state->current_offset += evt->data_len;
                
                // Calculate percentage
                // int percentage = (state->current_offset * 100) / state->total_bytes;
                ESP_LOGI(TAG, "Progress: %d/%d bytes", (int)state->current_offset, (int)state->total_bytes);
            }
            break;

        case HTTP_EVENT_ERROR:
            ESP_LOGD(TAG, "HTTP_EVENT_ERROR");
            break;
        case HTTP_EVENT_ON_CONNECTED:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_CONNECTED");
            break;
        case HTTP_EVENT_HEADER_SENT:
            ESP_LOGD(TAG, "HTTP_EVENT_HEADER_SENT");
            break;
        case HTTP_EVENT_ON_FINISH:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_FINISH");
            xSemaphoreGive(download_semaphore);
            break;
        case HTTP_EVENT_DISCONNECTED:
            ESP_LOGD(TAG, "HTTP_EVENT_DISCONNECTED");
            break;
        case HTTP_EVENT_REDIRECT:
            ESP_LOGD(TAG, "HTTP_EVENT_REDIRECT");
            esp_http_client_set_header(evt->client, "From", "user@example.com");
            esp_http_client_set_header(evt->client, "Accept", "text/html");
            esp_http_client_set_redirection(evt->client);
            break;
    }
    return ESP_OK;
}

esp_err_t download_file(const char* json_info, const char* file_path) {
    download_state_t state = {
        .file = fopen(file_path, "wb"),
        .total_bytes = 0,
        .current_offset = 0
    };
    
    if (!state.file) {
        ESP_LOGE(TAG, "Failed to open file for writing");
        return ESP_FAIL;
    }

    // Allocate heap memory to copy json_info
    size_t json_len = strlen(json_info) + 1; // +1 for null terminator
    char *json_copy = (char*)malloc(json_len);

    if (json_copy == NULL) {
        ESP_LOGE(TAG, "Memory allocation failed for json_info copy");
        fclose(state.file);  // Don't forget to close the file if malloc fails
        return ESP_ERR_NO_MEM;
    }

    strcpy(json_copy, json_info); // Copy the string to heap memory

    char **mqtt_data = parse_mqtt_url(json_copy);
    if (mqtt_data == NULL) {
        ESP_LOGE(TAG, "Failed to parse MQTT URL");
        free(json_copy);
        fclose(state.file);
        return ESP_ERR_INVALID_ARG;
    }

    // Allocate heap for combining protocol and url
    char* http_delim = "://";  
    size_t total_length = strlen(mqtt_data[0]) + strlen(http_delim) + strlen(mqtt_data[1]) + 1; // +1 for '\0'
    char *full_host = (char *)malloc(total_length);

    if (full_host == NULL) {
        ESP_LOGE(TAG, "Memory allocation failed for full_host");
        free_mqtt_url(mqtt_data);
        free(json_copy);
        fclose(state.file);
        return ESP_ERR_NO_MEM;
    }

    // Copy the protocol string into the allocated memory
    strcpy(full_host, mqtt_data[0]);
    // Concat the ://
    strcat(full_host, http_delim);
    // Concatenate the hostname string onto the protocol
    strcat(full_host, mqtt_data[1]);
    //

    xSemaphoreTake(download_semaphore, portMAX_DELAY);
    // Pass our state structure via user_data
    // Path has to be split out from JSON with key path as well
    esp_http_client_config_t config = {
        .url = full_host,
        .path = mqtt_data[2],
        .event_handler = download_event_handler,
        .user_data = &state 
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);

    if (client == NULL) {
        ESP_LOGE(TAG, "Failed to initialize HTTP client");
        free(full_host);
        free_mqtt_url(mqtt_data);
        free(json_copy);
        fclose(state.file);
        return ESP_ERR_NO_MEM;
    }

    esp_err_t err = esp_http_client_perform(client);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "HTTP request failed: %s", esp_err_to_name(err));
    }

    xSemaphoreTake(download_semaphore, portMAX_DELAY);

    esp_http_client_cleanup(client);
    
    // Free resources
    free(full_host);
    free(json_copy);
    free_mqtt_url(mqtt_data);  // Make sure to free the mqtt_data array
    fclose(state.file);
    
    return ESP_OK;
}


esp_err_t upload_file_to_s3(const char* json_info, const char* file_path) {
    // Allocate heap memory to copy json_info
    size_t json_len = strlen(json_info) + 1; // +1 for null terminator
    char *json_copy = (char*)malloc(json_len);

    if (json_copy == NULL) {
        ESP_LOGE(TAG, "Memory allocation failed for json_info copy");
        return ESP_ERR_NO_MEM;
    }

    strcpy(json_copy, json_info); // Copy the string to heap memory

    // Parse the MQTT URL into its components
    char **mqtt_data = parse_mqtt_url(json_copy);

    if (mqtt_data == NULL) {
        ESP_LOGE(TAG, "Failed to parse MQTT URL");
        free(json_copy);
        return ESP_ERR_INVALID_ARG;
    }

    // Open the file for reading
    FILE* file = fopen(file_path, "rb");
    char header_file_length[16]; 

    if (!file) {
        ESP_LOGE(TAG, "Failed to open file for reading");
        free(json_copy);
        free_mqtt_url(mqtt_data);
        return ESP_ERR_INVALID_ARG;
    }

    if (fseek(file, 0, SEEK_END) != 0) {
        ESP_LOGE(TAG, "Error seeking to the end of the file");
        fclose(file);
        free(json_copy);
        free_mqtt_url(mqtt_data);
        return ESP_ERR_INVALID_ARG;
    }

    int file_size = ftell(file);
    fseek(file, 0, SEEK_SET); // Rewind to the front of the file

    // Allocate heap memory for the full URL
    char* http_delim = "://";  
    size_t total_length = strlen(mqtt_data[0]) + strlen(http_delim) + strlen(mqtt_data[1]) + 1; // +1 for '\0'
    char *full_host = (char *)malloc(total_length);

    if (full_host == NULL) {
        ESP_LOGE(TAG, "Memory allocation failed for full_host");
        fclose(file);
        free(json_copy);
        free_mqtt_url(mqtt_data);
        return ESP_ERR_NO_MEM;
    }

    // Build the full host URL
    strcpy(full_host, mqtt_data[0]);
    strcat(full_host, http_delim);
    strcat(full_host, mqtt_data[1]);

    // Configure the HTTP client
    esp_http_client_config_t config = {
        .host = full_host,
        .path = mqtt_data[2], 
        .method = HTTP_METHOD_PUT,
        .keep_alive_enable = true,
    };

    // Free the http_delim (string literal) is not needed for freeing
    // free(http_delim);  // Do not free string literals!

    // Initialize HTTP client
    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (!client) {
        ESP_LOGE(TAG, "Failed to initialize HTTP client");
        fclose(file);
        free(full_host);
        free(json_copy);
        free_mqtt_url(mqtt_data);
        return ESP_ERR_NO_MEM;
    }

    // Set headers for the HTTP PUT request
    esp_http_client_set_header(client, "Host", mqtt_data[1]);  // Set as domain
    esp_http_client_set_header(client, "Content-Type", "audio/wav");  // For wav
    lwip_itoa(header_file_length, sizeof(header_file_length), file_size);  // For text version of the header file length
    esp_http_client_set_header(client, "Content-Length", header_file_length);  // Set header for length to expect

    // Open the HTTP connection
    esp_err_t err = esp_http_client_open(client, file_size);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "HTTP PUT request failed: %s", esp_err_to_name(err));
        esp_http_client_cleanup(client);
        fclose(file);
        free(full_host);
        free(json_copy);
        free_mqtt_url(mqtt_data);
        return err;
    }

    // Send file in chunks
    size_t total_sent = 0;
    char data[MAX_HTTP_OUTPUT_BUFFER];

    while (total_sent < file_size) {
        size_t to_send = MAX_HTTP_OUTPUT_BUFFER;
        if (total_sent + to_send > file_size) {
            to_send = file_size - total_sent;
        }

        // Read the chunk from the file
        size_t bytes_read = fread(data, 1, to_send, file);
        if (bytes_read != to_send) {
            ESP_LOGE(TAG, "Error reading from file");
            esp_http_client_cleanup(client);
            fclose(file);
            free(full_host);
            free(json_copy);
            free_mqtt_url(mqtt_data);
            return ESP_ERR_INVALID_ARG;
        }

        ESP_LOGI(TAG, "Read %zu bytes, sending now", to_send);

        // Send the chunk over HTTP
        int write_len = esp_http_client_write(client, &data[0], to_send);
        if (write_len <= 0) {
            ESP_LOGE(TAG, "Error occurred during file write: %s", esp_err_to_name(write_len));
            esp_http_client_cleanup(client);
            fclose(file);
            free(full_host);
            free(json_copy);
            free_mqtt_url(mqtt_data);
            return write_len;
        }

        // Update the total number of bytes sent
        total_sent += to_send;
    }

    // Handle the response
    char output_buffer[MAX_HTTP_OUTPUT_BUFFER + 1] = {0};
    int content_length = esp_http_client_fetch_headers(client);

    if (content_length < 0) {
        ESP_LOGE(TAG, "HTTP client fetch headers failed");
    } else {
        int data_read = esp_http_client_read_response(client, output_buffer, MAX_HTTP_OUTPUT_BUFFER);
        if (data_read >= 0) {
            ESP_LOGI(TAG, "HTTP PUT Status = %d, content_length = %"PRId64,
                     esp_http_client_get_status_code(client),
                     esp_http_client_get_content_length(client));
        }
    }

    // Clean up the HTTP client
    esp_http_client_close(client);
    esp_http_client_cleanup(client);

    // Free all heap-allocated memory and close the file
    fclose(file);
    free(full_host);
    free(json_copy);
    free_mqtt_url(mqtt_data);

    return ESP_OK;
}

