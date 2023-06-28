#include <stdio.h>
#include <esp_log.h>
#include <freertos/freertos.h>
#include <freertos/task.h>
#include "jpeg.h"
#include "timeProbe.h"
#include "lcd.h"

uint16_t frameBuffer[240 * 240];


void app_main(void) {
    init_lcd();

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

        esp_lcd_panel_draw_bitmap(lcd_panel_handle,0,0,240,240,frameBuffer);

        vTaskDelay(pdMS_TO_TICKS(1));

    }


}