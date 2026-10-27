"""General Example for reciving byte data from CAN and translating into Celsius""" 


#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_err.h"
#include "esp_log.h"
#include "driver/twai.h"

#define CAN_TX_GPIO GPIO_NUM_4
#define CAN_RX_GPIO GPIO_NUM_5

// Example sensor CAN ID and conversion scale
#define TEMP_SENSOR_ID 0x123
#define TEMP_SCALE_C   0.01f

static const char *TAG = "CAN_TEMP";

static void can_receive_task(void *arg)
{
    twai_message_t msg;

    while (1) {
        if (twai_receive(&msg, pdMS_TO_TICKS(1000)) != ESP_OK) {
            continue;
        }

        // Match the example standard CAN data frame
        if (msg.identifier != TEMP_SENSOR_ID ||
            msg.extd ||
            msg.rtr ||
            msg.data_length_code < 2) {
            continue;
        }

        // Combine two bytes into a signed 16-bit value
        uint16_t raw_bits =
            ((uint16_t)msg.data[0] << 8) |
            (uint16_t)msg.data[1];

        int16_t raw_temperature = (int16_t)raw_bits;

        // Convert raw bits to Celsius, then Fahrenheit
        float temperature_c =
            raw_temperature * TEMP_SCALE_C;

        float temperature_f =
            (temperature_c * 9.0f / 5.0f) + 32.0f;

        // Output only; no reference-temperature variable
        ESP_LOGI(TAG, "Temperature: %.2f F", temperature_f);
    }
}

void app_main(void)
{
    twai_general_config_t g_config =
        TWAI_GENERAL_CONFIG_DEFAULT(
            CAN_TX_GPIO,
            CAN_RX_GPIO,
            TWAI_MODE_LISTEN_ONLY
        );

    // Example bitrate only; match your CAN bus.
    twai_timing_config_t t_config =
        TWAI_TIMING_CONFIG_500KBITS();

    twai_filter_config_t f_config =
        TWAI_FILTER_CONFIG_ACCEPT_ALL();

    ESP_ERROR_CHECK(
        twai_driver_install(&g_config, &t_config, &f_config)
    );

    ESP_ERROR_CHECK(twai_start());

    ESP_LOGI(TAG, "Listening for temperature frames");

    xTaskCreate(
        can_receive_task,
        "can_receive",
        4096,
        NULL,
        5,
        NULL
    );
}