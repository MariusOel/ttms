#include "display/LGFX_ESP32C6_ST7789.hpp"
#include "esp_log.h"
#include "esp_random.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lvgl.h"
#include "ota_manager.hpp"
#include "ui/ui.h"
#include <math.h>
#include <stdio.h>

#define FIRMWARE_VERSION "0.1.7"
static const char *TAG = "MAIN";
static LGFX_ESP32C6_ST7789 tft;

/* Screen resolution */
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
  ESP_LOGI(TAG, "Starting Tire Temp Monitor v%s", FIRMWARE_VERSION);

  // 1. Hardware & Services Init
  ota_manager_start();

  tft.begin();
  tft.setRotation(1); // Landscape
  tft.setBrightness(128);

  lv_init();

  // 2. LVGL Tick Timer
  const esp_timer_create_args_t lvgl_tick_timer_args = {
      .callback = &lv_tick_task,
      .arg = NULL,
      .dispatch_method = ESP_TIMER_TASK,
      .name = "lvgl_tick",
      .skip_unhandled_events = false};
  esp_timer_handle_t lvgl_tick_timer;
  esp_timer_create(&lvgl_tick_timer_args, &lvgl_tick_timer);
  esp_timer_start_periodic(lvgl_tick_timer, 10000); // 10ms

  // 3. Display Driver Setup
  lv_disp_draw_buf_init(&draw_buf, buf, NULL, screenWidth * 10);

  static lv_disp_drv_t disp_drv;
  lv_disp_drv_init(&disp_drv);
  disp_drv.hor_res = screenWidth;
  disp_drv.ver_res = screenHeight;
  disp_drv.flush_cb = my_disp_flush;
  disp_drv.draw_buf = &draw_buf;
  lv_disp_drv_register(&disp_drv);

  // 4. SHOW SPLASH SCREEN (3 seconds)
  ui_show_splash(FIRMWARE_VERSION, "IP: 192.168.4.1");

  uint32_t splash_start = esp_timer_get_time() / 1000;
  while ((esp_timer_get_time() / 1000) - splash_start < 3000) {
    lv_timer_handler();
    vTaskDelay(pdMS_TO_TICKS(50)); // Yield to IDLE and other tasks
  }

  // 5. INITIALIZE MAIN UI
  lv_obj_clean(lv_scr_act());
  ui_init();

  // 6. PRE-FILL DATA (Optimized)
  ESP_LOGI(TAG, "Pre-filling simulation data...");
  {
    float time_s = -300.0;
    const float PI = 3.14159265;
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

      // Add data without refreshing the chart for speed
      bool is_last = (i == 299);
      ui_update_data(sim_front, sim_rear, is_last);

      // Yield every iteration during pre-fill to ensure IDLE runs
      vTaskDelay(1);
    }
  }
  ESP_LOGI(TAG, "Startup complete.");

  // 7. MAIN LOOP
  uint32_t last_update = 0;
  while (1) {
    lv_timer_handler();
    vTaskDelay(pdMS_TO_TICKS(10));

    uint32_t now = esp_timer_get_time() / 1000;
    if (now - last_update >= 1000) {
      last_update = now;

      static float time_seconds = 0.0;
      time_seconds += 1.0;

      const float PI = 3.14159265;
      float trend = 35.0 + 15.0 * sin(2.0 * PI * time_seconds / 600.0);
      float sim_front = trend + 3.0 * sin(2.0 * PI * time_seconds / 120.0);
      float sim_rear =
          trend + 3.0 * sin(2.0 * PI * time_seconds / 120.0 + PI / 3.0);

      ui_update_data(sim_front, sim_rear, true);
    }
  }
}
