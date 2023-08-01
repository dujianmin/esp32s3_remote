#include "my_mqtt.h"
#include "my_motor.h"
#include "config.h"
#
static const char *TAG = "MY_MQTT";

esp_mqtt_client_handle_t client;
MotorParams *params;
static void log_error_if_nonzero(const char *message, int error_code)
{
    if (error_code != 0) {
        ESP_LOGE(TAG, "Last error %s: 0x%x", message, error_code);
    }
}

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    // ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%ld", base, event_id);
    esp_mqtt_event_handle_t event = event_data;
    esp_mqtt_client_handle_t client = event->client;
    int msg_id;
    switch ((esp_mqtt_event_id_t)event_id) {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");

        msg_id = esp_mqtt_client_subscribe(client, "/topic/ps4_1", 0);
        ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);

        msg_id = esp_mqtt_client_subscribe(client, "/topic/ps4_2", 1);
        ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);
        break;
    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
        break;

    case MQTT_EVENT_SUBSCRIBED:

        break;
    case MQTT_EVENT_UNSUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_PUBLISHED:
        ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_DATA:
        ESP_LOGI(TAG, "MQTT_EVENT_DATA");
        if (event->data_len > 0)
        {
            ESP_LOGI(TAG, "Received MQTT message: %.*s", event->data_len, event->data);

            cJSON *root = cJSON_Parse(event->data);
        
            if (root != NULL)
            {
                cJSON *x = cJSON_GetObjectItem(root, "X");
                cJSON *y = cJSON_GetObjectItem(root, "Y");
                cJSON *a = cJSON_GetObjectItem(root, "A");                
                if (x != NULL && y != NULL && a != NULL )
                {
                    int x_value = x->valueint;
                    int y_value = y->valueint;  
                    int a_value = a->valueint;                     
                    params->left_speed = x_value;
                    params->right_speed = y_value;  
                    params->angle = a_value;                        
                    ESP_LOGI(TAG, "Parsed data - x: %d, y: %d, a: %d", x_value, y_value, a_value);
                }

                cJSON_Delete(root);
            }
            else
            {
                const char *error_ptr = cJSON_GetErrorPtr();
                if (error_ptr != NULL)
                {
                    ESP_LOGE(TAG, "Failed to parse JSON: %s", error_ptr);
                }
            }
        }

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

void mqtt_task(void *pvParameters)
{
    params = (MotorParams *)pvParameters;

    // motor_init();
    esp_mqtt_client_config_t mqtt_cfg = {
        // .broker.address.uri = CONFIG_BROKER_URL,
        .uri = CONFIG_BROKER_URL,
    };

    client = esp_mqtt_client_init(&mqtt_cfg);
    /* The last argument may be used to pass data to the event handler, in this example mqtt_event_handler */
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(client);

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));  // 延时1秒，可以根据需要调整延时时间
    }
}