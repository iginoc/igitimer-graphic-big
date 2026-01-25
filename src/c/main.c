#include <pebble.h>
#include "igitimer-graphic-big.h"

static Window *s_window;

// Gestione pulsante SELEZIONA: Pausa/Riprendi
static void select_click_handler(ClickRecognizerRef recognizer, void *context) {
  Timer_toggle_pause();
}

// Gestione pulsante SELEZIONA (Pressione lunga): Riavvia il timer
static void select_long_click_handler(ClickRecognizerRef recognizer, void *context) {
  Timer_restart();
}

// Gestione pulsante SU: Aumenta minuti
static void up_click_handler(ClickRecognizerRef recognizer, void *context) {
  Timer_plus_minute();
}

// Gestione pulsante SU (Pressione lunga): Aumenta 5 minuti
static void up_long_click_handler(ClickRecognizerRef recognizer, void *context) {
  Timer_plus_5_minutes();
}

// Gestione pulsante GIÙ: Diminuisce minuti
static void down_click_handler(ClickRecognizerRef recognizer, void *context) {
  Timer_minus_minute();
}

// Gestione pulsante GIÙ (Pressione lunga): Diminuisce 5 minuti
static void down_long_click_handler(ClickRecognizerRef recognizer, void *context) {
  Timer_minus_5_minutes();
}

// Configurazione dei pulsanti
static void click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler);
  window_long_click_subscribe(BUTTON_ID_SELECT, 0, select_long_click_handler, NULL);
  window_single_click_subscribe(BUTTON_ID_UP, up_click_handler);
  window_long_click_subscribe(BUTTON_ID_UP, 0, up_long_click_handler, NULL);
  window_single_click_subscribe(BUTTON_ID_DOWN, down_click_handler);
  window_long_click_subscribe(BUTTON_ID_DOWN, 0, down_long_click_handler, NULL);
}

static void window_load(Window *window) {
  // Inizializza il timer passando la finestra principale
  Timer_init(window);
}

static void window_unload(Window *window) {
  Timer_deinit();
}

static void init(void) {
  s_window = window_create();
  window_set_click_config_provider(s_window, click_config_provider);
  window_set_window_handlers(s_window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload,
  });
  window_stack_push(s_window, true);
}

static void deinit(void) {
  window_destroy(s_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}