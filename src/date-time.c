#include "pebble.h"

Window *window;
TextLayer *text_date_layer;
TextLayer *battery_layer;
TextLayer *text_time_layer;
Layer *line_layer;
TextLayer *connection_layer;
TextLayer *bar_layer;
AppTimer *timer;

void line_layer_update_callback(Layer *layer, GContext* ctx) {
  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_fill_rect(ctx, layer_get_bounds(layer), 0, GCornerNone);
}

void update_bluetooth(void *data){
  text_layer_set_text(connection_layer, "");
}

void handle_back_click(ClickRecognizerRef recognizer, void *context) {
  vibes_short_pulse();
}

void click_config_provider(void *context) {
  const uint16_t repeat_interval_ms = 1000;
  window_single_repeating_click_subscribe(BUTTON_ID_BACK, repeat_interval_ms, handle_back_click);
}

void handle_bluetooth(bool connected){
  text_layer_set_text(connection_layer, connected ? "BT: connected" : "BT: disconnected");
  vibes_double_pulse();
  if (connected)
    timer = app_timer_register(5000, update_bluetooth, NULL);
}

void handle_battery(BatteryChargeState charge_state) {
  static char battery_text[] = "100% charged";

  if (charge_state.is_charging) {
    snprintf(battery_text, sizeof(battery_text), "charging");
  } else {
    snprintf(battery_text, sizeof(battery_text), "%d%% charged", charge_state.charge_percent);
  }
  text_layer_set_text(battery_layer, battery_text);
}

void handle_minute_tick(struct tm *tick_time, TimeUnits units_changed) {
  // Need to be static because they're used by the system later.
  static char time_text[] = "00:00:00";
  static char date_text[] = "Xxxx 00/00/00";

  char *time_format;

  handle_battery(battery_state_service_peek());

  // TODO: Only update the date when it's changed.
  strftime(date_text, sizeof(date_text), "%a. %D", tick_time);
  text_layer_set_text(text_date_layer, date_text);


  if (clock_is_24h_style()) {
    time_format = "%R";
  } else {
    time_format = "%I:%M";
  }

  strftime(time_text, sizeof(time_text), time_format, tick_time);

  // Kludge to handle lack of non-padded hour format string
  // for twelve hour clock.
  if (!clock_is_24h_style() && (time_text[0] == '0')) {
    memmove(time_text, &time_text[1], sizeof(time_text) - 1);
  }

  text_layer_set_text(text_time_layer, time_text);
}

void handle_deinit(void) {
  tick_timer_service_unsubscribe();
  battery_state_service_unsubscribe();
  bluetooth_connection_service_unsubscribe();
}

void handle_init(void) {
  window = window_create();
  window_set_fullscreen(window, true);
  window_stack_push(window, true /* Animated */);
  window_set_background_color(window, GColorBlack);
  window_set_click_config_provider(window, click_config_provider);

  Layer *window_layer = window_get_root_layer(window);
  // Date: wkday. mm/dd/yy
  text_date_layer = text_layer_create(GRect(0, 92, 145, 100));
  text_layer_set_text_color(text_date_layer, GColorBlack);
  text_layer_set_background_color(text_date_layer, GColorWhite);
  text_layer_set_font(text_date_layer, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_DROID_SERIF_20)));
  text_layer_set_text_alignment(text_date_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(text_date_layer));

  // Time - 12hr
  text_time_layer = text_layer_create(GRect(0, 117, 145, 50));
  text_layer_set_text_color(text_time_layer, GColorBlack);
  text_layer_set_background_color(text_time_layer, GColorClear);
  text_layer_set_font(text_time_layer, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_DROID_SERIF_42)));
  text_layer_set_text_alignment(text_time_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(text_time_layer));

  // Percent charged battery display
  battery_layer = text_layer_create(GRect(0, 0, 144-0, 168-68));
  text_layer_set_text_color(battery_layer, GColorWhite);
  text_layer_set_background_color(battery_layer, GColorClear);
  text_layer_set_font(battery_layer, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_DROID_SERIF_BOLD_15)));           //FONT_ROBOTO_CONDENSED_18)));
  text_layer_set_text_alignment(battery_layer, GTextAlignmentCenter);
  text_layer_set_text(battery_layer, "... charged");
  layer_add_child(window_layer, text_layer_get_layer(battery_layer));

  // Bluetooth connection
  connection_layer = text_layer_create(GRect(0, 22, 145, 20));
  text_layer_set_text_color(connection_layer, GColorWhite);
  text_layer_set_background_color(connection_layer, GColorClear);
  text_layer_set_font(connection_layer, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_DROID_SERIF_BOLD_15))); //FONT_ROBOTO_CONDENSED_18)));
  text_layer_set_text_alignment(connection_layer, GTextAlignmentCenter);
  text_layer_set_text(connection_layer, "");
  layer_add_child(window_layer, text_layer_get_layer(connection_layer));

  // black seperator bar
  bar_layer = text_layer_create(GRect(0, 118, 145, 3));
  text_layer_set_background_color(bar_layer, GColorBlack);
  layer_add_child(window_layer, text_layer_get_layer(bar_layer));

// White line from old UI
/*
  GRect line_frame = GRect(8, 117, 130, 2);
  line_layer = layer_create(line_frame);
  layer_set_color(line_layer, GColorBlack);
  layer_set_update_proc(line_layer, line_layer_update_callback);
  layer_add_child(window_layer, line_layer);
*/

  tick_timer_service_subscribe(MINUTE_UNIT, handle_minute_tick);
  battery_state_service_subscribe(&handle_battery);
  bluetooth_connection_service_subscribe(&handle_bluetooth);

  // If bluetooth is disconnected, it will say so even if you leave watchface
  bool connected = bluetooth_connection_service_peek();
  if (!connected)
    text_layer_set_text(connection_layer, "BT: disconnected");
    
  // TODO: Update display here to avoid blank display on launch?
}


int main(void) {
  handle_init();

  app_event_loop();

  handle_deinit();
}
