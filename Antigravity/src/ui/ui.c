#include "ui.h"
#include <stdio.h>

static lv_obj_t *chart_front;
static lv_obj_t *chart_rear;
static lv_chart_series_t *ser_front;
static lv_chart_series_t *ser_rear;
static lv_chart_series_t *ser_front_trend;
static lv_chart_series_t *ser_rear_trend;
static lv_obj_t *label_avg_front;
static lv_obj_t *label_avg_rear;

// Circular buffers for calculating 2-minute moving average (120 seconds)
#define AVG_WINDOW_SIZE 120
static float front_history[AVG_WINDOW_SIZE];
static float rear_history[AVG_WINDOW_SIZE];
static int history_idx = 0;
static int history_count = 0;

// Declare the font if not already available via header
LV_FONT_DECLARE(lv_font_montserrat_48);

void ui_init(void) {
  // Set black background for the screen
  lv_obj_set_style_bg_color(lv_scr_act(), lv_color_black(), 0);

  // --- FRONT TIRE SECTION (TOP) ---

  // Create FRONT tire chart (top left, wider)
  // Create FRONT tire chart (top left, wider)
  chart_front = lv_chart_create(lv_scr_act());
  lv_obj_set_size(chart_front, 315, 120);             // 320 - 5 = 315 width
  lv_obj_align(chart_front, LV_ALIGN_TOP_LEFT, 5, 0); // 5px margin left

  // Style: Black bg, gray border
  lv_obj_set_style_bg_color(chart_front, lv_color_black(), 0);
  lv_obj_set_style_border_color(chart_front, lv_color_hex(0x404040), 0);
  lv_obj_set_style_border_width(chart_front, 2, 0);

  // Chart config
  lv_chart_set_type(chart_front, LV_CHART_TYPE_LINE);
  lv_chart_set_point_count(chart_front,
                           320); // Upscale resolution to match width
  lv_chart_set_update_mode(chart_front, LV_CHART_UPDATE_MODE_SHIFT);
  lv_chart_set_range(chart_front, LV_CHART_AXIS_PRIMARY_Y, 15,
                     60); // Zoom in: 15° - 60°

  // Grid lines
  lv_obj_set_style_line_color(chart_front, lv_color_hex(0x303030),
                              LV_PART_MAIN);
  lv_obj_set_style_text_color(chart_front, lv_color_hex(0x808080),
                              LV_PART_TICKS);

  // Trend Series: White initially, will be colored dynamically
  ser_front_trend = lv_chart_add_series(chart_front, lv_color_white(),
                                        LV_CHART_AXIS_PRIMARY_Y);

  // Main Series: White
  ser_front = lv_chart_add_series(chart_front, lv_color_white(),
                                  LV_CHART_AXIS_PRIMARY_Y);
  lv_obj_set_style_line_width(chart_front, 9, LV_PART_ITEMS);

  // Label for Front Average (Left Side of Chart, Bottom Aligned with Padding)
  label_avg_front = lv_label_create(lv_scr_act());
  lv_label_set_text(label_avg_front, "--.-°");
  lv_obj_set_style_text_color(label_avg_front, lv_color_white(),
                              0); // White initially
  lv_obj_set_style_text_font(label_avg_front, &lv_font_montserrat_48, 0);

  // Set semi-transparent black background
  lv_obj_set_style_bg_color(label_avg_front, lv_color_black(), 0);
  lv_obj_set_style_bg_opa(label_avg_front, LV_OPA_60, 0); // 60% opacity

  // Align bottom-left of chart area
  lv_obj_align_to(label_avg_front, chart_front, LV_ALIGN_BOTTOM_LEFT, 5, -5);
  lv_obj_set_style_text_align(label_avg_front, LV_TEXT_ALIGN_CENTER, 0);

  // Bring label to top so it's over the chart
  lv_obj_move_foreground(label_avg_front);

  // --- REAR TIRE SECTION (BOTTOM) ---

  // Create REAR tire chart (bottom left)
  chart_rear = lv_chart_create(lv_scr_act());
  lv_obj_set_size(chart_rear, 315, 120); // 320 - 5 = 315 width
  lv_obj_align(chart_rear, LV_ALIGN_BOTTOM_LEFT, 5,
               0); // 5px margin left

  // Style
  lv_obj_set_style_bg_color(chart_rear, lv_color_black(), 0);
  lv_obj_set_style_border_color(chart_rear, lv_color_hex(0x404040), 0);
  lv_obj_set_style_border_width(chart_rear, 2, 0);

  // Chart config
  lv_chart_set_type(chart_rear, LV_CHART_TYPE_LINE);
  lv_chart_set_point_count(chart_rear, 320); // Upscale resolution to 320
  lv_chart_set_update_mode(chart_rear, LV_CHART_UPDATE_MODE_SHIFT);
  lv_chart_set_range(chart_rear, LV_CHART_AXIS_PRIMARY_Y, 15,
                     60); // Zoom in: 15° - 60°

  // Grid lines
  lv_obj_set_style_line_color(chart_rear, lv_color_hex(0x303030), LV_PART_MAIN);
  lv_obj_set_style_text_color(chart_rear, lv_color_hex(0x808080),
                              LV_PART_TICKS);

  // Trend Series: White initially, will be colored dynamically
  ser_rear_trend = lv_chart_add_series(chart_rear, lv_color_white(),
                                       LV_CHART_AXIS_PRIMARY_Y);

  // Main Series: White
  ser_rear = lv_chart_add_series(chart_rear, lv_color_white(),
                                 LV_CHART_AXIS_PRIMARY_Y);
  lv_obj_set_style_line_width(chart_rear, 9, LV_PART_ITEMS);

  // Label for Rear Average (Left Side of Chart, Top Aligned with Padding)
  label_avg_rear = lv_label_create(lv_scr_act());
  lv_label_set_text(label_avg_rear, "--.-°");
  lv_obj_set_style_text_color(label_avg_rear, lv_color_white(),
                              0); // White initially
  lv_obj_set_style_text_font(label_avg_rear, &lv_font_montserrat_48, 0);

  // Set semi-transparent black background
  lv_obj_set_style_bg_color(label_avg_rear, lv_color_black(), 0);
  lv_obj_set_style_bg_opa(label_avg_rear, LV_OPA_60, 0); // 60% opacity

  // Align top-left of chart area
  lv_obj_align_to(label_avg_rear, chart_rear, LV_ALIGN_TOP_LEFT, 5, 5);
  lv_obj_set_style_text_align(label_avg_rear, LV_TEXT_ALIGN_CENTER, 0);

  // Bring label to top
  lv_obj_move_foreground(label_avg_rear);
}

void ui_update_data(float front_temp, float rear_temp) {
  // Update Moving Averages first so we can plot them
  front_history[history_idx] = front_temp;
  rear_history[history_idx] = rear_temp;

  history_idx = (history_idx + 1) % AVG_WINDOW_SIZE;
  if (history_count < AVG_WINDOW_SIZE)
    history_count++;

  // Calculate Averages
  float sum_front = 0;
  float sum_rear = 0;
  for (int i = 0; i < history_count; i++) {
    sum_front += front_history[i];
    sum_rear += rear_history[i];
  }
  float avg_front =
      (history_count > 0) ? (sum_front / history_count) : front_temp;
  float avg_rear = (history_count > 0) ? (sum_rear / history_count) : rear_temp;

  // -- Dynamic Trend Coloring Logic --
  static float prev_avg_front = 0;
  static float prev_avg_rear = 0;

  // Front Trend: Red (Ascending) / Blue (Descending)
  lv_color_t color_front;
  if (avg_front > prev_avg_front) {
    color_front = lv_palette_main(LV_PALETTE_RED);
  } else {
    color_front = lv_palette_main(LV_PALETTE_BLUE);
  }
  ser_front_trend->color = color_front;
  lv_obj_set_style_text_color(label_avg_front, color_front, 0);
  prev_avg_front = avg_front;

  // Rear Trend: Red (Ascending) / Blue (Descending)
  lv_color_t color_rear;
  if (avg_rear > prev_avg_rear) {
    color_rear = lv_palette_main(LV_PALETTE_RED);
  } else {
    color_rear = lv_palette_main(LV_PALETTE_BLUE);
  }
  ser_rear_trend->color = color_rear;
  lv_obj_set_style_text_color(label_avg_rear, color_rear, 0);
  prev_avg_rear = avg_rear;

  // Update Charts (Main Data + Trendline)
  // We plot the CURRENT moving average as the trend point
  lv_chart_set_next_value(chart_front, ser_front, (lv_coord_t)front_temp);
  lv_chart_set_next_value(chart_front, ser_front_trend, (lv_coord_t)avg_front);

  lv_chart_set_next_value(chart_rear, ser_rear, (lv_coord_t)rear_temp);
  lv_chart_set_next_value(chart_rear, ser_rear_trend, (lv_coord_t)avg_rear);

  lv_chart_refresh(chart_front);
  lv_chart_refresh(chart_rear);

  // Update Labels
  char buf[32];
  snprintf(buf, sizeof(buf), "%.1f°", avg_front);
  lv_label_set_text(label_avg_front, buf);

  snprintf(buf, sizeof(buf), "%.1f°", avg_rear);
  lv_label_set_text(label_avg_rear, buf);
}
