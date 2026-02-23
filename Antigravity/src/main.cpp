#include "display/LGFX_ESP32C6_ST7789.hpp"
#include "esp_log.h"
#include "esp_random.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lvgl.h"
#include "ui/ui.h"
#include <stdio.h>

static const char *TAG = "MAIN";
static LGFX_ESP32C6_ST7789 tft;

/* Change to your screen resolution */
static const uint32_t screenWidth = 320;
static const uint32_t screenHeight = 240;

static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf[screenWidth * 10];

/* Display flushing */
void my_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area,
                   lv_color_t *color_p) {
  uint32_t w = (area->x2 - area->x1 + 1);
  uint32_t h = (area->y2 - area->y1 + 1);

  tft.startWrite();
  tft.setAddrWindow(area->x1, area->y1, w, h);
  tft.writePixels((lgfx::rgb565_t *)&color_p->full, w * h);
  tft.endWrite();

  lv_disp_flush_ready(disp);
}

/* LVGL tick callback */
static void lv_tick_task(void *arg) {
  lv_tick_inc(10); // 10ms tick
}

extern "C" void app_main(void) {
  ESP_LOGI(TAG, "Starting Tire Temp Monitor");

  tft.begin();
  tft.setRotation(1); // 90 degrees clockwise
  tft.setBrightness(128);

  lv_init();

  // Setup LVGL tick timer (10ms periodic)
  const esp_timer_create_args_t lvgl_tick_timer_args = {
      .callback = &lv_tick_task,
      .arg = NULL,
      .dispatch_method = ESP_TIMER_TASK,
      .name = "lvgl_tick",
      .skip_unhandled_events = false};
  esp_timer_handle_t lvgl_tick_timer;
  esp_timer_create(&lvgl_tick_timer_args, &lvgl_tick_timer);
  esp_timer_start_periodic(lvgl_tick_timer, 10000); // 10ms in microseconds

  lv_disp_draw_buf_init(&draw_buf, buf, NULL, screenWidth * 10);

  static lv_disp_drv_t disp_drv;
  lv_disp_drv_init(&disp_drv);
  disp_drv.hor_res = screenWidth;
  disp_drv.ver_res = screenHeight;
  disp_drv.flush_cb = my_disp_flush;
  disp_drv.draw_buf = &draw_buf;
  lv_disp_drv_register(&disp_drv);

  ui_init();

  // Pre-fill simulation data (300 points = 5 minutes of history)
  // To avoid waiting for the graph to fill up
  {
    float time_s = -300.0; // Start 5 minutes ago
    const float PI = 3.14159265359;
    const float oscillation_period = 120.0;
    const float trend_period = 600.0;
    const float min_temp = 20.0;
    const float max_temp = 50.0;
    const float trend_amplitude = (max_temp - min_temp) / 2.0;
    const float trend_center = (max_temp + min_temp) / 2.0;
    const float oscillation_amplitude = 3.0;

    for (int i = 0; i < 300; i++) {
      time_s += 1.0;

      float trend = trend_center +
                    trend_amplitude * sin(2.0 * PI * time_s / trend_period);
      float sim_front = trend + oscillation_amplitude *
                                    sin(2.0 * PI * time_s / oscillation_period);
      float sim_rear =
          trend + oscillation_amplitude *
                      sin(2.0 * PI * time_s / oscillation_period + PI / 3.0);

      ui_update_data(sim_front, sim_rear);
    }
  }

  uint32_t last_update = 0;
  while (1) {
    lv_timer_handler();
    vTaskDelay(pdMS_TO_TICKS(5));

    uint32_t now = esp_timer_get_time() / 1000; // Convert to ms
    if (now - last_update > 1000) {
      last_update = now;

      // Simulate sensor data with trending sinusoidal curves
      // Oscillation period: 120 seconds (2 minutes)
      // Trend period: 600 seconds (10 minutes) - 20°C -> 50°C -> 20°C
      static float time_seconds = 0.0;
      time_seconds += 1.0; // Increment by 1 second

      const float PI = 3.14159265359;
      const float oscillation_period = 120.0; // Fast oscillation: 2 minutes
      const float trend_period = 600.0; // Slow trend: 10 minutes (full cycle)
      const float min_temp = 20.0;
      const float max_temp = 50.0;
      const float trend_amplitude = (max_temp - min_temp) / 2.0; // 15°C
      const float trend_center = (max_temp + min_temp) / 2.0;    // 35°C
      const float oscillation_amplitude = 3.0; // ±3°C oscillation

      // Trending baseline: goes from 20°C to 50°C and back to 20°C
      float trend =
          trend_center +
          trend_amplitude * sin(2.0 * PI * time_seconds / trend_period);

      // Front tire: oscillating sine wave on top of trend
      float sim_front =
          trend + oscillation_amplitude *
                      sin(2.0 * PI * time_seconds / oscillation_period);

      // Rear tire: oscillating sine wave with phase shift on top of trend
      float sim_rear =
          trend +
          oscillation_amplitude *
              sin(2.0 * PI * time_seconds / oscillation_period + PI / 3.0);

      ui_update_data(sim_front, sim_rear);
    }
  }
}
