#pragma once
#include <string.h>
#include "esp_http_client.h"
#include "esp_log.h"

#ifndef MIN
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#endif

#define MAX_HTTP_RECV_BUFFER 1024 
#define MAX_HTTP_OUTPUT_BUFFER 2048

static const char *TAG = "HTTP_CLIENT";


// Structure to maintain state across callbacks
typedef struct {
    FILE* file;
    size_t total_bytes;
    size_t current_offset;
} download_state_t;


esp_err_t http_event_handler(esp_http_client_event_t *evt) {
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
                int percentage = (state->current_offset * 100) / state->total_bytes;
                ESP_LOGI(TAG, "Progress: %d%% (%d/%d bytes)", percentage, (int)state->current_offset, (int)state->total_bytes);
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

esp_err_t download_file_to_sd(const char* url, const char* file_path) {
    // Initialize our state
    download_state_t state = {
        .file = fopen(file_path, "wb"),
        .total_bytes = 0,
        .current_offset = 0
    };
    
    if (!state.file) {
        ESP_LOGE(TAG, "Failed to open file for writing");
        return ESP_FAIL;
    }

    // Pass our state structure via user_data
    esp_http_client_config_t config = {
        .url = url,
        .event_handler = http_event_handler,
        .user_data = &state  // This is key!
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_err_t ret = esp_http_client_perform(client);
    
    fclose(state.file);
    esp_http_client_cleanup(client);
    
    return ret;
}

esp_err_t upload_file_from_sd(const char* url, const char* file_path) {
    // Open the file
    FILE* f = fopen(file_path, "rb");
    if (f == NULL) {
        ESP_LOGE(TAG, "Failed to open file for reading");
        return ESP_FAIL;
    }

    // Get file size
    fseek(f, 0, SEEK_END);
    size_t file_size = ftell(f);
    fseek(f, 0, SEEK_SET);

    esp_http_client_config_t config = {
        .url = url,
        .method = HTTP_METHOD_PUT,
    };
    
    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (client == NULL) {
        ESP_LOGE(TAG, "Failed to initialize HTTP client");
        fclose(f);
        return ESP_FAIL;
    }

    // Buffer for reading file chunks
    uint8_t *buffer = malloc(MAX_HTTP_RECV_BUFFER);
    if (buffer == NULL) {
        ESP_LOGE(TAG, "Failed to allocate buffer");
        esp_http_client_cleanup(client);
        fclose(f);
        return ESP_FAIL;
    }

    size_t uploaded = 0;
    esp_err_t ret = ESP_OK;
    
    while (uploaded < file_size) {
        // Read chunk from file
        size_t chunk_size = MIN(MAX_HTTP_RECV_BUFFER, file_size - uploaded);
        size_t bytes_read = fread(buffer, 1, chunk_size, f);
        
        if (bytes_read != chunk_size) {
            ESP_LOGE(TAG, "Failed to read from file");
            ret = ESP_FAIL;
            break;
        }

        // Set Content-Range header for this chunk
        char content_range[64];
        snprintf(content_range, sizeof(content_range), 
                "bytes %d-%d/%d", 
                (int) uploaded, 
                (int) (uploaded + chunk_size - 1),
                (int) file_size);
                
        esp_http_client_set_header(client, "Content-Range", content_range);
        
        // Set Content-Length for this chunk
        char chunk_length[32];
        snprintf(chunk_length, sizeof(chunk_length), "%d", (int) chunk_size);
        esp_http_client_set_header(client, "Content-Length", chunk_length);
        
        // Upload the chunk
        esp_http_client_set_post_field(client, (const char*)buffer, chunk_size);
        esp_err_t err = esp_http_client_perform(client);
        
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Failed to upload chunk: %s", esp_err_to_name(err));
            ret = err;
            break;
        }
        
        int status = esp_http_client_get_status_code(client);
        if (status < 200 || status >= 300) {
            ESP_LOGE(TAG, "Bad status code: %d", status);
            ret = ESP_FAIL;
            break;
        }
        
        uploaded += chunk_size;
        ESP_LOGI(TAG, "Uploaded %d/%d bytes", (int) uploaded, (int) file_size);
    }
    
    // Clean up
    free(buffer);
    esp_http_client_cleanup(client);
    fclose(f);
    
    return ret;
}
