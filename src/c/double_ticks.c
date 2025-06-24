#include <pebble.h>
#include "utils.h"

#define DEBUG_TIME (false)
#define BUFFER_LEN (10)
#define COL_BG COLOR_FALLBACK(GColorOxfordBlue,               GColorBlack)
#define COL_LN COLOR_FALLBACK(GColorMidnightGreen,            GColorWhite)
#define COL_TK COLOR_FALLBACK(GColorLiberty,                  GColorWhite)
#define COL_HR COLOR_FALLBACK(GColorCeleste,                  GColorWhite)
#define COL_MN COLOR_FALLBACK(GColorRajah,                    GColorWhite)

static Window* s_window;
static Layer* s_layer;
static char s_buffer[BUFFER_LEN];

static void draw_ticks(GContext* ctx, GPoint center, int vcr, int minute_tip, int hour_tip, int text_size) {
  graphics_context_set_stroke_width(ctx, 1);
  int inf_radius = vcr * 2;
  int MAX = 12 * 60;
  for (int m = 0; m < MAX; m++) {
    int angle = m * TRIG_MAX_RATIO / MAX;
    if (m % 60 == 0) {
      // Hour lines
      graphics_context_set_stroke_color(ctx, COL_LN);
      graphics_draw_line(ctx, center, cartesian_from_polar(center, inf_radius, angle));
    } else if (m % 30 == 0) {
      // Half hour dots
      graphics_context_set_fill_color(ctx, COL_TK);
      graphics_fill_circle(ctx, cartesian_from_polar(center, hour_tip, angle), 1);
    } else if (m % 12 == 0) {
      // Minute ticks
      graphics_context_set_stroke_color(ctx, COL_TK);
      graphics_draw_line(ctx, cartesian_from_polar(center, minute_tip, angle), cartesian_from_polar(center, inf_radius, angle));
    }
  }

  // Hour numbers
  graphics_context_set_text_color(ctx, COL_HR);
  int hour_mid_radius = (hour_tip + minute_tip) / 2;
  MAX = 12;
  for (int h = 2; h <= MAX; h += 2) {
    int angle = h * TRIG_MAX_RATIO / MAX;
    snprintf(s_buffer, BUFFER_LEN, "%d", h);
    GPoint text_center = cartesian_from_polar(center, hour_mid_radius, angle);
    if (h < 20) {
      // GTextCenterAlignment is a liar
      text_center.x++;
    }
    GRect bbox = rect_from_midpoint(text_center, GSize(text_size, text_size));
    draw_text_midalign(ctx, s_buffer, bbox, GTextAlignmentCenter, true);
    // DEBUG code below
    //graphics_context_set_stroke_color(ctx, GColorRed);
    //graphics_draw_pixel(ctx, text_center);
  }

  // minute numbers
  graphics_context_set_text_color(ctx, COL_MN);
  int minute_mid_radius = (minute_tip + vcr) / 2;
  MAX = 60;
  for (int m = 5; m < MAX; m += 10) {
    int angle = m * TRIG_MAX_RATIO / MAX;
    snprintf(s_buffer, BUFFER_LEN, "%d", m);
    GPoint text_center = cartesian_from_polar(center, minute_mid_radius, angle);
    if (m < 20) {
      // GTextCenterAlignment is a liar
      text_center.x++;
    }
    GRect bbox = rect_from_midpoint(text_center, GSize(text_size, text_size));
    draw_text_midalign(ctx, s_buffer, bbox, GTextAlignmentCenter, true);
    // DEBUG code below
    //graphics_context_set_stroke_color(ctx, GColorRed);
    //graphics_draw_pixel(ctx, text_center);
  }
}

static void draw_minute_hand(GContext* ctx, GPoint center, int radius, struct tm* now) {
  int total_mins = 60;
  int current_mins = now->tm_min;
  int angle = current_mins * TRIG_MAX_ANGLE / total_mins;
  GPoint tip = cartesian_from_polar(center, radius - 4, angle);
  graphics_context_set_stroke_width(ctx, 5);
  graphics_context_set_stroke_color(ctx, COL_MN);
  graphics_draw_line(ctx, center, tip);
}

static void draw_hour_hand(GContext* ctx, GPoint center, int radius, struct tm* now) {
  int total_mins = 12 * 60;
  int current_mins = now->tm_hour * 60 + now->tm_min;
  int angle = current_mins * TRIG_MAX_ANGLE / total_mins;
  GPoint tip = cartesian_from_polar(center, radius - 4, angle);
  graphics_context_set_stroke_width(ctx, 5);
  graphics_context_set_stroke_color(ctx, COL_HR);
  graphics_draw_line(ctx, center, tip);
}

static void update_layer(Layer* layer, GContext* ctx) {
  time_t temp = time(NULL);
  struct tm* now = localtime(&temp);
  if (DEBUG_TIME) {
    fast_forward_time(now);
  }

  GRect bounds = layer_get_bounds(layer);
  int vcr = min(bounds.size.h, bounds.size.w) / 2 - PBL_IF_ROUND_ELSE(4, 2);
  int text_size = vcr * 7 / 20;
  int minute_tip = vcr - text_size * 2 / 3; 
  int hour_tip = minute_tip * 6 / 10;
  GPoint center = grect_center_point(&bounds);
  draw_ticks(ctx, center, vcr, minute_tip, hour_tip, text_size);
  draw_minute_hand(ctx, center, minute_tip, now);
  draw_hour_hand(ctx, center, hour_tip, now);
}

static void window_load(Window* window) {
  Layer* window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);
  window_set_background_color(s_window, COL_BG);
  s_layer = layer_create(bounds);
  layer_set_update_proc(s_layer, update_layer);
  layer_add_child(window_layer, s_layer);
}

static void window_unload(Window* window) {
  layer_destroy(s_layer);
}

static void tick_handler(struct tm* now, TimeUnits units_changed) {
  layer_mark_dirty(window_get_root_layer(s_window));
}

static void init(void) {
  s_window = window_create();
  window_set_window_handlers(s_window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload,
  });
  window_stack_push(s_window, true);
  tick_timer_service_subscribe(DEBUG_TIME ? SECOND_UNIT : MINUTE_UNIT, tick_handler);
}

static void deinit(void) {
  window_destroy(s_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}