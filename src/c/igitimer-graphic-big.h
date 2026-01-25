#pragma once

#include <pebble.h>

// Initialize the timer
void Timer_init(Window *window);

// Deinitialize the timer
void Timer_deinit(void);

// Start the timer with duration in seconds
void Timer_start(uint32_t duration_seconds);

// Stop the timer
void Timer_stop(void);

// Toggle pause/resume
void Timer_toggle_pause(void);

// Check if timer is running
bool Timer_is_running(void);

// Get remaining time in seconds
uint32_t Timer_get_remaining(void);

// Redraw the timer display
void Timer_redraw(void);

// Add one minute to the timer
void Timer_plus_minute(void);

// Subtract one minute from the timer
void Timer_minus_minute(void);

// Add five minutes to the timer
void Timer_plus_5_minutes(void);

// Subtract five minutes from the timer
void Timer_minus_5_minutes(void);

// Restart the timer with the last set duration
void Timer_restart(void);
