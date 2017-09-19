
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event_loop.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "driver/i2s.h"

#include "vector.h"
#include "audio_renderer.h"
#include "playerconfig.h"
#include "wifi.h"
#include "app_main.h"
#include "bt_speaker.h"
#include "uart_rc.h"


#define WIFI_LIST_NUM   10


#define TAG "main"


//Priorities of the reader and the decoder thread. bigger number = higher prio
#define PRIO_READER configMAX_PRIORITIES -3
#define PRIO_MQTT configMAX_PRIORITIES - 3
#define PRIO_CONNECT configMAX_PRIORITIES -1

int volatile taskstatus = 0;
int volatile taskcounter = 0;


static void init_hardware()
{
    nvs_flash_init();

    ESP_LOGI(TAG, "hardware initialized");
}

static renderer_config_t *create_renderer_config()
{
    renderer_config_t *renderer_config = calloc(1, sizeof(renderer_config_t));

    renderer_config->bit_depth = I2S_BITS_PER_SAMPLE_16BIT;
    renderer_config->i2s_num = I2S_NUM_0;
    renderer_config->sample_rate = 44100;
    renderer_config->sample_rate_modifier = 1.0;
    renderer_config->output_mode = AUDIO_OUTPUT_MODE;

    if(renderer_config->output_mode == I2S_MERUS) {
        renderer_config->bit_depth = I2S_BITS_PER_SAMPLE_32BIT;
    }

    if(renderer_config->output_mode == DAC_BUILT_IN) {
        renderer_config->bit_depth = I2S_BITS_PER_SAMPLE_16BIT;
    }

    return renderer_config;
}


static void memory_logger(void* p) 
{
    while(1) {
        ESP_LOGI(TAG, "RAM left %d, %d, %d", esp_get_free_heap_size(), taskstatus, taskcounter);
        vTaskDelay(50);
    }
}

/**
 * entry point
 */
void app_main()
{
    ESP_LOGI(TAG, "starting app_main()");
    // ESP_LOGI(TAG, "RAM left: %u", esp_get_free_heap_size());

    init_hardware();
    uart_app_start();
    bt_speaker_start(create_renderer_config());

    // ESP_LOGI(TAG, "RAM left %d", esp_get_free_heap_size());
    // ESP_LOGI(TAG, "app_main stack: %d\n", uxTaskGetStackHighWaterMark(NULL));

    // xTaskCreate(memory_logger, "memory_logger", 2048, NULL, 0, NULL);
}
