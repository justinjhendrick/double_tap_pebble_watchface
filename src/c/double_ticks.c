#include <pebble.h>
#include "utils.h"

#define DEBUG_TIME (true)
#define BUFFER_LEN (10)
#define COL_BG (GColorBlack)
#define COL_TK (GColorWhite)
#define COL_HR (GColorPictonBlue)
#define COL_MN (GColorOrange)

static Window* s_window;
static Layer* s_layer;
static GPath* s_tri;
static char s_buffer[BUFFER_LEN];

static const GPathInfo TRI_POINTS = {
  .num_points = 3,
  .points = (GPoint []) {{0, 0}, {0, 0}, {0, 0}}
};

static void draw_tri(GContext* ctx, GPoint center, int radius, int height, int angle, int width_angle) {
  int hh = height / 2;
  int ha = width_angle / 2;
  TRI_POINTS.points[0] = cartesian_from_polar(center, radius - hh, angle     );
  TRI_POINTS.points[1] = cartesian_from_polar(center, radius     , angle - ha);
  TRI_POINTS.points[2] = cartesian_from_polar(center, radius     , angle + ha);
  gpath_draw_filled(ctx, s_tri);
}

static void draw_minute_ticks(GContext* ctx, GPoint center, int radius) {
  graphics_context_set_stroke_width(ctx, 1);
  graphics_context_set_fill_color(ctx, COL_TK);
  graphics_context_set_stroke_color(ctx, COL_TK);
  int MAX = 60;
  for (int m = 0; m < MAX; m++) {
    int angle = m * TRIG_MAX_RATIO / MAX;
    if (m % 5 == 0) {
      draw_tri(ctx, center, radius, 14, angle, DEG_TO_TRIGANGLE(6));
    } else {
      graphics_draw_pixel(ctx, cartesian_from_polar(center, radius, angle));
    }
  }
}

static void draw_hour_ticks(GContext* ctx, GPoint center, int radius) {
  graphics_context_set_stroke_width(ctx, 1);
  graphics_context_set_fill_color(ctx, COL_TK);
  graphics_context_set_stroke_color(ctx, COL_TK);
  int MAX = 12 * 60;
  for (int m = 0; m < MAX; m += 30) {
    int angle = m * TRIG_MAX_RATIO / MAX;
    if (m % 60 == 0) {
      draw_tri(ctx, center, radius, 14, angle, DEG_TO_TRIGANGLE(6));
    } else {
      graphics_draw_pixel(ctx, cartesian_from_polar(center, radius, angle));
    }
  }
}

static void draw_minute_hand(GContext* ctx, GPoint center, int radius, struct tm* now) {
  graphics_context_set_text_color(ctx, COL_MN);
  int total_mins = 60;
  int current_mins = now->tm_min;
  int angle = current_mins * TRIG_MAX_ANGLE / total_mins;
  GPoint tip = cartesian_from_polar(center, radius - 6, angle);
  graphics_context_set_stroke_width(ctx, 5);
  graphics_context_set_stroke_color(ctx, COL_MN);
  graphics_draw_line(ctx, center, tip);
  snprintf(s_buffer, BUFFER_LEN, "%d", now->tm_min);
  GPoint text_center = cartesian_from_polar(center, radius + 10, angle);
  GRect bbox = rect_from_midpoint(text_center, GSize(40, 40));
  draw_text_midalign(ctx, s_buffer, bbox, GTextAlignmentCenter, true);
}

static void draw_hour_hand(GContext* ctx, GPoint center, int radius, struct tm* now) {
  graphics_context_set_text_color(ctx, COL_HR);
  int total_mins = 12 * 60;
  int current_mins = now->tm_hour * 60 + now->tm_min;
  int angle = current_mins * TRIG_MAX_ANGLE / total_mins;
  GPoint tip = cartesian_from_polar(center, radius - 6, angle);
  graphics_context_set_stroke_width(ctx, 5);
  graphics_context_set_stroke_color(ctx, COL_HR);
  graphics_draw_line(ctx, center, tip);
  format_hour(s_buffer, BUFFER_LEN, now, true);
  GPoint text_center = cartesian_from_polar(center, radius + 10, angle);
  GRect bbox = rect_from_midpoint(text_center, GSize(40, 40));
  draw_text_midalign(ctx, s_buffer, bbox, GTextAlignmentCenter, true);
}

static void update_layer(Layer* layer, GContext* ctx) {
  time_t temp = time(NULL);
  struct tm* now = localtime(&temp);
  if (DEBUG_TIME) {
    fast_forward_time(now);
  }

  GRect bounds = layer_get_bounds(layer);
  int vcr = min(bounds.size.h, bounds.size.w) / 2 - PBL_IF_ROUND_ELSE(4, 2);
  int minute_tip = vcr - 20;
  int hour_tip = vcr / 2;
  GPoint center = grect_center_point(&bounds);
  draw_minute_ticks(ctx, center, minute_tip);
  draw_hour_ticks(ctx, center, hour_tip);

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
  s_tri = gpath_create(&TRI_POINTS);
  tick_timer_service_subscribe(DEBUG_TIME ? SECOND_UNIT : MINUTE_UNIT, tick_handler);
}

static void deinit(void) {
  gpath_destroy(s_tri);
  window_destroy(s_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}