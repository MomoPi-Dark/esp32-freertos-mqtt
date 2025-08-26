#include <stdio.h>
#include <stdbool.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "nvs_flash.h"
#include "driver/gpio.h"
#include "mqtt_client.h"

#include "lib/wifi.c"
#include "lib/mqtt.c"

#define LED_PIN GPIO_NUM_2

#define WIFI_SSID "your_ssid"
#define WIFI_PASS "your_password"

#define MQTT_BROKER_URI "mqtt://your_server:1883"

static const char *MAIN_TAG = "ESP32_APP";

void my_mqtt_handler(const char *topic, const char *data)
{
    if (strcmp(topic, "esp32/led") == 0)
    {
        if (strcmp(data, "ON") == 0)
        {
            gpio_set_level(LED_PIN, 1);
            ESP_LOGI("APP", "💡 LED ON via MQTT");
        }
        else if (strcmp(data, "OFF") == 0)
        {
            gpio_set_level(LED_PIN, 0);
            ESP_LOGI("APP", "💡 LED OFF via MQTT");
        }
    }
}

void app_init(void)
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_LOGI(MAIN_TAG, "✅ NVS initialized!");

    wifi_init_handler(
        (wifi_config_t){
            .sta = {
                .ssid = WIFI_SSID,
                .password = WIFI_PASS,
                .threshold.authmode = WIFI_AUTH_WPA2_PSK,
                .pmf_cfg = {
                    .capable = true,
                    .required = false},
            },
        });
    ESP_LOGI(MAIN_TAG, "✅ WiFi initialized!");

    gpio_reset_pin(LED_PIN);
    gpio_set_direction(LED_PIN, GPIO_MODE_OUTPUT);

    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = MQTT_BROKER_URI,
    };
    mqtt_init(mqtt_cfg, my_mqtt_handler);
}

static int counter = 0;

void app_loop(void)
{
    ESP_LOGI(MAIN_TAG, "LED TOGGLE");
    gpio_set_level(LED_PIN, counter % 2);
    counter++;

    if (mqtt_client != NULL)
    {
        const char *msg = (counter % 2) ? "LED=ON" : "LED=OFF";
        mqtt_publish("esp32/status", msg, 0, 1, 0);
    }

    vTaskDelay(pdMS_TO_TICKS(2000));
}

void app_main(void)
{
    app_init();
    while (true)
    {
        app_loop();
    }
}
