#include <pebble.h>
#include <pebble-fctx/fctx.h>
#include <pebble-fctx/ffont.h>
#include "igitimer-graphic-big.h"

#ifndef INT_TO_FIXED
#define INT_TO_FIXED(a) ((int32_t)(a) << 16)
#endif

// Timer state
static Layer* timer_layer;
static FFont* timer_font;

static uint32_t timer_remaining_seconds;
static uint32_t timer_duration_seconds;
static bool timer_is_paused;
static bool timer_running;
static AppTimer *timer_app_timer;
static AppTimer *anim_timer;
static time_t last_tick_time;
static uint16_t last_tick_ms;

// Timer display strings
static char timer_seconds_str[8];

// Forward declarations
static void timer_tick_callback(void *data);

// Update the timer display strings
static void update_timer_strings(void) {
  snprintf(timer_seconds_str, sizeof(timer_seconds_str), "%lu", timer_remaining_seconds);
}

// Timer tick - decrease time every second
static void timer_tick_callback(void *data) {
  if (timer_running && !timer_is_paused) {
    if (timer_remaining_seconds > 0) {
      timer_remaining_seconds--;
      time_ms(&last_tick_time, &last_tick_ms);
      update_timer_strings();
      Timer_redraw();
      
      if (timer_remaining_seconds > 0) {
        timer_app_timer = app_timer_register(1000, timer_tick_callback, NULL);
      } else {
        // Timer finished - vibrate
        vibes_double_pulse();
        timer_running = false;
      }
    }
  }
}

static void anim_timer_callback(void *data) {
  if (timer_running && !timer_is_paused) {
    Timer_redraw();
    anim_timer = app_timer_register(50, anim_timer_callback, NULL);
  } else {
    anim_timer = NULL;
  }
}

// Draw the timer on screen
static void timer_layer_update_proc(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);
  
  // Set text color to white (or configured color if available)
  graphics_context_set_text_color(ctx, GColorBlack);
  
  // Draw a background rectangle
  graphics_context_set_fill_color(ctx, GColorYellow);
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);
  
  // Set fill color to black for text
  graphics_context_set_fill_color(ctx, GColorBlack);
  
  // Create FContext for drawing with FFont
  FContext fctx;
  fctx_init_context(&fctx, ctx);
  fctx_set_fill_color(&fctx, GColorBlack);
  
  // Load the font (using a large FFont similar to the clock display)
  if (!timer_font) {
    timer_font = ffont_create_from_resource(RESOURCE_ID_AVENIR_NEXT_REGULAR);
  }
  
  // Calculate font size to fill most of the screen
  int font_size = bounds.size.h / 2;
  
  // Draw progress bar (Left side)
  // Green fill (Decreases as timer counts down)
  if (timer_duration_seconds > 0) {
    int fill_height = (int)((uint64_t)timer_remaining_seconds * bounds.size.h / timer_duration_seconds);
    graphics_context_set_fill_color(ctx, GColorGreen);
    graphics_fill_rect(ctx, GRect(0, bounds.size.h - fill_height, bounds.size.w, fill_height), 0, GCornerNone);
  }
  
  // Calculate positions
  // Center of the screen
  int center_x = bounds.size.w / 2;
  int center_y = bounds.size.h / 2;
  
  // Draw seconds
  FPoint seconds_pos;
  seconds_pos.x = INT_TO_FIXED(center_x);
  seconds_pos.y = INT_TO_FIXED(center_y);

  fctx_begin_fill(&fctx);
  fctx_set_offset(&fctx, seconds_pos);
  fctx_set_text_em_height(&fctx, timer_font, font_size);
  fctx_draw_string(&fctx, timer_seconds_str, timer_font, GTextAlignmentCenter, FTextAnchorMiddle);
  fctx_end_fill(&fctx);
  
  int radius = (bounds.size.w < bounds.size.h ? bounds.size.w : bounds.size.h) / 2 - 5;
  int black_radius = radius - 10;
  time_t temp_time;
  uint16_t ms;
  time_ms(&temp_time, &ms);

  // Draw rotating black dot (smooth countdown, 1 rotation per 60s)
  long elapsed_ms = (temp_time - last_tick_time) * 1000 + (ms - last_tick_ms);
  if (elapsed_ms < 0) elapsed_ms = 0;
  if (elapsed_ms > 1000) elapsed_ms = 1000;

  // Draw rotating black dot (smooth countdown, starts from top)
  int32_t angle = 0;
  if (timer_duration_seconds > 0) {
    int64_t total_ms_remaining = (int64_t)timer_remaining_seconds * 1000 - elapsed_ms;
    int64_t total_ms_duration = (int64_t)timer_duration_seconds * 1000;
    angle = (int32_t)(TRIG_MAX_ANGLE * total_ms_remaining / total_ms_duration);

    // Draw the trail (arc)
    int arc_width = 6;
    int arc_outer_radius = black_radius + arc_width / 2;
    GRect arc_rect = GRect(
      center_x - arc_outer_radius, 
      center_y - arc_outer_radius, 
      arc_outer_radius * 2, 
      arc_outer_radius * 2
    );
    graphics_context_set_fill_color(ctx, GColorBlack);
    graphics_fill_radial(ctx, arc_rect, GOvalScaleModeFitCircle, arc_width, angle, TRIG_MAX_ANGLE);
  }
  
  GPoint dot_pos = GPoint(
    center_x + (sin_lookup(angle) * black_radius / TRIG_MAX_RATIO),
    center_y - (cos_lookup(angle) * black_radius / TRIG_MAX_RATIO)
  );
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_circle(ctx, dot_pos, 5);
  
  // Draw red rotating dot (1 rotation per second)
  int32_t red_angle = TRIG_MAX_ANGLE * elapsed_ms / 1000;
  
  // Draw red trail (sweeping arc)
  int red_radius = radius;
  int red_arc_width = 3;
  int red_arc_outer_radius = red_radius + red_arc_width / 2;
  GRect red_arc_rect = GRect(
    center_x - red_arc_outer_radius, 
    center_y - red_arc_outer_radius, 
    red_arc_outer_radius * 2, 
    red_arc_outer_radius * 2
  );
  graphics_context_set_fill_color(ctx, GColorRed);
  graphics_fill_radial(ctx, red_arc_rect, GOvalScaleModeFitCircle, red_arc_width, 0, red_angle);
  
  GPoint red_dot_pos = GPoint(
    center_x + (sin_lookup(red_angle) * red_radius / TRIG_MAX_RATIO),
    center_y - (cos_lookup(red_angle) * red_radius / TRIG_MAX_RATIO)
  );
  graphics_context_set_fill_color(ctx, GColorRed);
  graphics_fill_circle(ctx, red_dot_pos, 5);
  
  // Draw pause icon if paused
  if (timer_is_paused && timer_running) {
    const int bar_width = 6;
    const int bar_height = 20;
    const int gap = 6;
    const int total_width = (2 * bar_width) + gap;
    
    int start_x = center_x - (total_width / 2);
    int start_y = center_y - (bar_height / 2);
    
    graphics_context_set_fill_color(ctx, GColorBlack);
    graphics_fill_rect(ctx, GRect(start_x, start_y, bar_width, bar_height), 2, GCornersAll);
    graphics_fill_rect(ctx, GRect(start_x + bar_width + gap, start_y, bar_width, bar_height), 2, GCornersAll);
  }
  
  fctx_deinit_context(&fctx);
}

// Initialize timer
void Timer_init(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);
  
  // Create the timer display layer
  timer_layer = layer_create(bounds);
  layer_set_update_proc(timer_layer, timer_layer_update_proc);
  layer_add_child(window_layer, timer_layer);
  
  // Initialize timer state
  timer_remaining_seconds = 0;
  timer_duration_seconds = 0;
  timer_is_paused = false;
  timer_running = false;
  timer_app_timer = NULL;
  anim_timer = NULL;
  
  // Initialize display strings
  snprintf(timer_seconds_str, sizeof(timer_seconds_str), "0");
}

// Deinitialize timer
void Timer_deinit(void) {
  if (timer_app_timer) {
    app_timer_cancel(timer_app_timer);
  }
  
  if (anim_timer) {
    app_timer_cancel(anim_timer);
  }
  
  if (timer_font) {
    ffont_destroy(timer_font);
  }
  
  if (timer_layer) {
    layer_destroy(timer_layer);
  }
}

// Start the timer
void Timer_start(uint32_t duration_seconds) {
  timer_remaining_seconds = duration_seconds;
  timer_is_paused = false;
  timer_running = true;
  
  update_timer_strings();
  Timer_redraw();
  
  // Start the tick callback
  if (timer_app_timer) {
    app_timer_cancel(timer_app_timer);
  }
  timer_app_timer = app_timer_register(1000, timer_tick_callback, NULL);
  time_ms(&last_tick_time, &last_tick_ms);
  
  if (anim_timer) {
    app_timer_cancel(anim_timer);
  }
  anim_timer = app_timer_register(50, anim_timer_callback, NULL);
}

// Stop the timer
void Timer_stop(void) {
  timer_running = false;
  
  if (timer_app_timer) {
    app_timer_cancel(timer_app_timer);
    timer_app_timer = NULL;
  }
  
  if (anim_timer) {
    app_timer_cancel(anim_timer);
    anim_timer = NULL;
  }
  
  Timer_redraw();
}

// Toggle pause/resume
void Timer_toggle_pause(void) {
  if (timer_running) {
    // Timer is running, so toggle pause state
    timer_is_paused = !timer_is_paused;
    
    if (!timer_is_paused && timer_app_timer == NULL) {
      // Resume the timer
      timer_app_timer = app_timer_register(1000, timer_tick_callback, NULL);
      time_ms(&last_tick_time, &last_tick_ms);
      if (anim_timer == NULL) {
        anim_timer = app_timer_register(50, anim_timer_callback, NULL);
      }
    } else if (timer_is_paused && timer_app_timer) {
      // Pause the timer
      app_timer_cancel(timer_app_timer);
      timer_app_timer = NULL;
      if (anim_timer) {
        app_timer_cancel(anim_timer);
        anim_timer = NULL;
      }
    }
  } else {
    // Timer is not running, so start it if a duration is set
    if (timer_duration_seconds > 0) {
      Timer_start(timer_duration_seconds);
    }
  }
}

// Check if timer is running
bool Timer_is_running(void) {
  return timer_running;
}

// Get remaining time
uint32_t Timer_get_remaining(void) {
  return timer_remaining_seconds;
}

// Add one minute
void Timer_plus_minute(void) {
  uint32_t minutes = timer_remaining_seconds / 60;
  timer_remaining_seconds = (minutes + 1) * 60;
  
  // Aggiorniamo la durata predefinita
  timer_duration_seconds = timer_remaining_seconds;
  
  update_timer_strings();
  Timer_redraw();
}

// Subtract one minute
void Timer_minus_minute(void) {
  uint32_t minutes = timer_remaining_seconds / 60;
  if (minutes > 0) {
    timer_remaining_seconds = (minutes - 1) * 60;
  } else {
    timer_remaining_seconds = 0;
  }

  // Se scendiamo a 0, fermiamo il timer
  if (timer_remaining_seconds == 0 && timer_running) {
    Timer_stop();
  }

  timer_duration_seconds = timer_remaining_seconds;
  
  update_timer_strings();
  Timer_redraw();
}

// Add five minutes
void Timer_plus_5_minutes(void) {
  uint32_t minutes = timer_remaining_seconds / 60;
  timer_remaining_seconds = (minutes + 5) * 60;
  
  // Aggiorniamo la durata predefinita
  timer_duration_seconds = timer_remaining_seconds;
  
  update_timer_strings();
  Timer_redraw();
}

// Subtract five minutes
void Timer_minus_5_minutes(void) {
  uint32_t minutes = timer_remaining_seconds / 60;
  if (minutes >= 5) {
    timer_remaining_seconds = (minutes - 5) * 60;
  } else {
    timer_remaining_seconds = 0;
  }

  // Se scendiamo a 0, fermiamo il timer
  if (timer_remaining_seconds == 0 && timer_running) {
    Timer_stop();
  }

  timer_duration_seconds = timer_remaining_seconds;
  
  update_timer_strings();
  Timer_redraw();
}

// Restart the timer
void Timer_restart(void) {
  Timer_stop();
  Timer_start(timer_duration_seconds);
}

// Redraw the timer display
void Timer_redraw(void) {
  layer_mark_dirty(timer_layer);
}
