#include "ui.h"
#include <stdint.h>
#include <stdio.h>

static lv_obj_t *chart_front;
static lv_obj_t *chart_rear;

// Main data series — kept white
static lv_chart_series_t *ser_front_data;
static lv_chart_series_t *ser_rear_data;

// Trend (moving average) series: red (ascending) and blue (descending).
// Only the active direction gets values; the other gets LV_CHART_POINT_NONE.
// At direction transitions both get the current value so segments connect.
static lv_chart_series_t *ser_front_trend_red;
static lv_chart_series_t *ser_front_trend_blue;
static lv_chart_series_t *ser_rear_trend_red;
static lv_chart_series_t *ser_rear_trend_blue;

static lv_obj_t *label_avg_front;
static lv_obj_t *label_avg_rear;

// Chart line thickness (pixels)
#define CHART_LINE_WIDTH 3

// Circular buffers for calculating 2-minute moving average (120 seconds)
#define AVG_WINDOW_SIZE 120
static float front_history[AVG_WINDOW_SIZE];
static float rear_history[AVG_WINDOW_SIZE];
static int history_idx = 0;
static int history_count = 0;

static lv_obj_t *label_age;

// Declare the font if not already available via header
LV_FONT_DECLARE(lv_font_montserrat_48);

void ui_init(void) {
  // Set black background for the screen
  lv_obj_set_style_bg_color(lv_scr_act(), lv_color_black(), 0);

  // --- FRONT TIRE SECTION (TOP) ---

  chart_front = lv_chart_create(lv_scr_act());
  lv_obj_set_size(chart_front, 300, 120);
  lv_obj_align(chart_front, LV_ALIGN_TOP_LEFT, 20, 0);

  lv_obj_set_style_bg_color(chart_front, lv_color_black(), 0);
  lv_obj_set_style_border_width(chart_front, 0, 0);
  lv_obj_set_style_pad_all(chart_front, 0, LV_PART_MAIN);

  lv_chart_set_type(chart_front, LV_CHART_TYPE_LINE);
  lv_chart_set_point_count(chart_front, 200);
  lv_chart_set_update_mode(chart_front, LV_CHART_UPDATE_MODE_SHIFT);
  lv_chart_set_range(chart_front, LV_CHART_AXIS_PRIMARY_Y, 150, 600);
  lv_chart_set_div_line_count(chart_front, 0, 0);
  lv_obj_set_style_size(chart_front, 0, LV_PART_INDICATOR);

  // Main data series (white, drawn first so trend series renders on top)
  ser_front_data = lv_chart_add_series(chart_front, lv_color_white(),
                                       LV_CHART_AXIS_PRIMARY_Y);

  // Red series: ascending trend
  ser_front_trend_red = lv_chart_add_series(
      chart_front, lv_palette_main(LV_PALETTE_RED), LV_CHART_AXIS_PRIMARY_Y);

  lv_obj_set_style_line_width(chart_front, CHART_LINE_WIDTH, LV_PART_ITEMS);
  // Blue series: descending trend
  ser_front_trend_blue = lv_chart_add_series(
      chart_front, lv_palette_main(LV_PALETTE_BLUE), LV_CHART_AXIS_PRIMARY_Y);

  // Label for Front Average
  label_avg_front = lv_label_create(lv_scr_act());
  lv_label_set_text(label_avg_front, "--.-°");
  lv_obj_set_style_text_color(label_avg_front, lv_color_white(), 0);
  lv_obj_set_style_text_font(label_avg_front, &lv_font_montserrat_48, 0);
  lv_obj_set_style_bg_color(label_avg_front, lv_color_black(), 0);
  lv_obj_set_style_bg_opa(label_avg_front, LV_OPA_60, 0);
  lv_obj_align_to(label_avg_front, chart_front, LV_ALIGN_TOP_LEFT, 5, 5);
  lv_obj_set_style_text_align(label_avg_front, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_move_foreground(label_avg_front);

  // --- REAR TIRE SECTION (BOTTOM) ---

  chart_rear = lv_chart_create(lv_scr_act());
  lv_obj_set_size(chart_rear, 300, 120);
  lv_obj_align(chart_rear, LV_ALIGN_BOTTOM_LEFT, 20, 0);

  lv_obj_set_style_bg_color(chart_rear, lv_color_black(), 0);
  lv_obj_set_style_border_width(chart_rear, 0, 0);
  lv_obj_set_style_pad_all(chart_rear, 0, LV_PART_MAIN);

  lv_chart_set_type(chart_rear, LV_CHART_TYPE_LINE);
  lv_chart_set_point_count(chart_rear, 200);
  lv_chart_set_update_mode(chart_rear, LV_CHART_UPDATE_MODE_SHIFT);
  lv_chart_set_range(chart_rear, LV_CHART_AXIS_PRIMARY_Y, 150, 600);
  lv_chart_set_div_line_count(chart_rear, 0, 0);
  lv_obj_set_style_size(chart_rear, 0, LV_PART_INDICATOR);
  lv_obj_set_style_line_width(chart_rear, CHART_LINE_WIDTH, LV_PART_ITEMS);

  // Main data series (white)
  ser_rear_data = lv_chart_add_series(chart_rear, lv_color_white(),
                                      LV_CHART_AXIS_PRIMARY_Y);

  // Red series: ascending trend
  ser_rear_trend_red = lv_chart_add_series(
      chart_rear, lv_palette_main(LV_PALETTE_RED), LV_CHART_AXIS_PRIMARY_Y);

  // Blue series: descending trend
  ser_rear_trend_blue = lv_chart_add_series(
      chart_rear, lv_palette_main(LV_PALETTE_BLUE), LV_CHART_AXIS_PRIMARY_Y);

  // Label for Rear Average
  label_avg_rear = lv_label_create(lv_scr_act());
  lv_label_set_text(label_avg_rear, "--.-°");
  lv_obj_set_style_text_color(label_avg_rear, lv_color_white(), 0);
  lv_obj_set_style_text_font(label_avg_rear, &lv_font_montserrat_48, 0);
  lv_obj_set_style_bg_color(label_avg_rear, lv_color_black(), 0);
  lv_obj_set_style_bg_opa(label_avg_rear, LV_OPA_60, 0);
  lv_obj_align_to(label_avg_rear, chart_rear, LV_ALIGN_TOP_LEFT, 5, 5);
  lv_obj_set_style_text_align(label_avg_rear, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_move_foreground(label_avg_rear);

  // Label for Chart Timespan
  label_age = lv_label_create(lv_scr_act());
  lv_label_set_text(label_age, "-200s");
  lv_obj_set_style_text_color(label_age, lv_color_white(), 0);
  lv_obj_set_style_bg_color(label_age, lv_color_black(), 0);
  lv_obj_set_style_bg_opa(label_age, LV_OPA_60, 0);
  lv_obj_align_to(label_age, chart_rear, LV_ALIGN_BOTTOM_LEFT, 15, -5);
  lv_obj_move_foreground(label_age);
}

void ui_update_data(float front_temp, float rear_temp) {
  // --- Moving Average ---
  front_history[history_idx] = front_temp;
  rear_history[history_idx] = rear_temp;
  history_idx = (history_idx + 1) % AVG_WINDOW_SIZE;
  if (history_count < AVG_WINDOW_SIZE)
    history_count++;

  float sum_front = 0, sum_rear = 0;
  for (int i = 0; i < history_count; i++) {
    sum_front += front_history[i];
    sum_rear += rear_history[i];
  }
  float avg_front =
      (history_count > 0) ? (sum_front / history_count) : front_temp;
  float avg_rear = (history_count > 0) ? (sum_rear / history_count) : rear_temp;

  // --- Direction tracking (for trend label color and per-segment trend series)
  // ---
  static lv_coord_t prev_scaled_avg_front = LV_CHART_POINT_NONE;
  static lv_coord_t prev_scaled_avg_rear = LV_CHART_POINT_NONE;
  static float prev_avg_front = 0;
  static float prev_avg_rear = 0;

  lv_coord_t scaled_front = (lv_coord_t)(front_temp * 10.0f);
  lv_coord_t scaled_rear = (lv_coord_t)(rear_temp * 10.0f);
  lv_coord_t scaled_avg_front = (lv_coord_t)(avg_front * 10.0f);
  lv_coord_t scaled_avg_rear = (lv_coord_t)(avg_rear * 10.0f);

  // Ascending if current trend value is >= previous
  bool front_asc = (prev_scaled_avg_front == LV_CHART_POINT_NONE) ||
                   (scaled_avg_front >= prev_scaled_avg_front);
  bool rear_asc = (prev_scaled_avg_rear == LV_CHART_POINT_NONE) ||
                  (scaled_avg_rear >= prev_scaled_avg_rear);

  // Detect transition by comparing previous direction flag
  static bool prev_front_asc = true;
  static bool prev_rear_asc = true;
  bool front_transition = (front_asc != prev_front_asc) &&
                          (prev_scaled_avg_front != LV_CHART_POINT_NONE);
  bool rear_transition = (rear_asc != prev_rear_asc) &&
                         (prev_scaled_avg_rear != LV_CHART_POINT_NONE);

  // Main data lines (white)
  lv_chart_set_next_value(chart_front, ser_front_data, scaled_front);
  lv_chart_set_next_value(chart_rear, ser_rear_data, scaled_rear);

  // Push trend values to the correct red/blue series
  if (front_asc) {
    lv_chart_set_next_value(chart_front, ser_front_trend_red, scaled_avg_front);
    lv_chart_set_next_value(chart_front, ser_front_trend_blue,
                            front_transition ? scaled_avg_front
                                             : LV_CHART_POINT_NONE);
  } else {
    lv_chart_set_next_value(chart_front, ser_front_trend_blue,
                            scaled_avg_front);
    lv_chart_set_next_value(chart_front, ser_front_trend_red,
                            front_transition ? scaled_avg_front
                                             : LV_CHART_POINT_NONE);
  }

  if (rear_asc) {
    lv_chart_set_next_value(chart_rear, ser_rear_trend_red, scaled_avg_rear);
    lv_chart_set_next_value(chart_rear, ser_rear_trend_blue,
                            rear_transition ? scaled_avg_rear
                                            : LV_CHART_POINT_NONE);
  } else {
    lv_chart_set_next_value(chart_rear, ser_rear_trend_blue, scaled_avg_rear);
    lv_chart_set_next_value(chart_rear, ser_rear_trend_red,
                            rear_transition ? scaled_avg_rear
                                            : LV_CHART_POINT_NONE);
  }

  // Label coloring: red if average is rising, blue if falling
  lv_color_t color_front = front_asc ? lv_palette_main(LV_PALETTE_RED)
                                     : lv_palette_main(LV_PALETTE_BLUE);
  lv_color_t color_rear = rear_asc ? lv_palette_main(LV_PALETTE_RED)
                                   : lv_palette_main(LV_PALETTE_BLUE);
  lv_obj_set_style_text_color(label_avg_front, color_front, 0);
  lv_obj_set_style_text_color(label_avg_rear, color_rear, 0);

  lv_chart_refresh(chart_front);
  lv_chart_refresh(chart_rear);

  // Update state
  prev_scaled_avg_front = scaled_avg_front;
  prev_scaled_avg_rear = scaled_avg_rear;
  prev_front_asc = front_asc;
  prev_rear_asc = rear_asc;
  prev_avg_front = avg_front;
  prev_avg_rear = avg_rear;

  // Update labels
  char buf[32];
  snprintf(buf, sizeof(buf), "%.1f°", avg_front);
  lv_label_set_text(label_avg_front, buf);
  snprintf(buf, sizeof(buf), "%.1f°", avg_rear);
  lv_label_set_text(label_avg_rear, buf);
}
