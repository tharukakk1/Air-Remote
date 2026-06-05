#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

static const char *TAG = "MAIN";

void app_main(void)
{
    ESP_LOGI(TAG, "Hello from ESP32-C3!");
    
    int i = 0;
    while (1) {
        printf("Hello World! (%d)\n", i++);
        vTaskDelay(pdMS_TO_TICKS(1000)); // Delay for 1 second
    }
}