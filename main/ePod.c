#include <stdio.h>
#include <esp_log.h>
#include <freertos/freertos.h>
#include <freertos/task.h>
#include "jpeg.h"
#include "timeProbe.h"


void app_main(void) {
    uint8_t fps_count = 0;
    timeProbe_t fps;
    timeProbe_start(&fps);
    while (1) {
        fps_count++;
        if (fps_count == 0) {
            ESP_LOGI("fps", "fps: %f", 256.0f * 1000.0f / (timeProbe_stop(&fps) / 1000.0));
            timeProbe_start(&fps);
        }
        jpeg_decode();
        while (1){
            vTaskDelay(1);
        }
    }


}