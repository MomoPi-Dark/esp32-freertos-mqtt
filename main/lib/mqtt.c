#include "mqtt_client.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

static const char *MQTT_TAG = "ESP32_MQTT";

static esp_mqtt_client_handle_t mqtt_client = NULL;

// 🔹 User-defined callback untuk data handler
typedef void (*mqtt_data_callback_t)(const char *topic, const char *data);

static mqtt_data_callback_t user_mqtt_callback = NULL;

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    esp_mqtt_event_handle_t event = event_data;
    switch (event->event_id)
    {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(MQTT_TAG, "✅ MQTT connected!");
        esp_mqtt_client_subscribe(mqtt_client, "esp32/led", 1);
        break;

    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGW(MQTT_TAG, "⚠️ MQTT disconnected!");
        break;

    case MQTT_EVENT_DATA:
        ESP_LOGI(MQTT_TAG, "📩 Data masuk: TOPIC=%.*s DATA=%.*s",
                 event->topic_len, event->topic,
                 event->data_len, event->data);

        if (user_mqtt_callback)
        {
            char topic[event->topic_len + 1];
            char data[event->data_len + 1];

            memcpy(topic, event->topic, event->topic_len);
            topic[event->topic_len] = '\0';

            memcpy(data, event->data, event->data_len);
            data[event->data_len] = '\0';

            user_mqtt_callback(topic, data);
        }
        break;

    default:
        break;
    }
}

// 🔹 Init MQTT + set callback
void mqtt_init(esp_mqtt_client_config_t mqtt_cfg, mqtt_data_callback_t cb)
{
    mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
    user_mqtt_callback = cb;
    esp_mqtt_client_register_event(mqtt_client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(mqtt_client);
}

void mqtt_publish(const char *topic, const char *data)
{
    if (mqtt_client)
    {
        esp_mqtt_client_publish(mqtt_client, topic, data, 0, 1, 0);
    }
}
