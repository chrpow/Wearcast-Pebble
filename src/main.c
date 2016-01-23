#include <pebble.h>
#include <stdio.h>

#define KEY_TEMPERATURE 0
#define KEY_CONDITIONS 1
#define KEY_HEAD 2
#define KEY_CHEST 3
#define KEY_LEGS 4
#define KEY_UMBRELLA 5

// main window
static Window *s_main_window;

// text layers
static TextLayer *s_logo_layer;
static TextLayer *s_time_layer;
static TextLayer *s_weather_layer;

// fonts
static GFont s_logo_font;
static GFont s_time_font;
static GFont s_weather_font;

// clothes layers
static BitmapLayer *s_chest_layer;
static BitmapLayer *s_legs_layer;
static BitmapLayer *s_feet_layer;
static BitmapLayer *s_face_layer;
static BitmapLayer *s_head_layer;
static BitmapLayer *s_accessory_layer;

// clothes bitmaps
static GBitmap *s_chest_bitmap;
static GBitmap *s_legs_bitmap;
static GBitmap *s_feet_bitmap;
static GBitmap *s_face_bitmap;
static GBitmap *s_head_bitmap;
static GBitmap *s_accessory_bitmap;

static void inbox_received_callback(DictionaryIterator *iterator, void *context) {
  // Store incoming information
  static char head_buffer[8];
  static char chest_buffer[8];
  static char legs_buffer[8];
  static char umbrella_buffer[8];
  static char temperature_buffer[8];
  static char conditions_buffer[32];
  static char weather_layer_buffer[32];

  // Read tuples for data
  Tuple *temp_tuple = dict_find(iterator, KEY_TEMPERATURE);
  Tuple *conditions_tuple = dict_find(iterator, KEY_CONDITIONS);
  Tuple *head_tuple = dict_find(iterator, KEY_HEAD);
  Tuple *chest_tuple = dict_find(iterator, KEY_CHEST);
  Tuple *legs_tuple = dict_find(iterator, KEY_LEGS);
  Tuple *umbrella_tuple = dict_find(iterator, KEY_UMBRELLA);
  
  // If all data is available, use it
  if(temp_tuple && conditions_tuple && head_tuple && chest_tuple && legs_tuple && umbrella_tuple) {
    snprintf(temperature_buffer, sizeof(temperature_buffer), "%dF", (int)temp_tuple->value->int32);
    snprintf(conditions_buffer, sizeof(conditions_buffer), "%s", conditions_tuple->value->cstring);
    //snprintf(head_buffer, sizeof(head_buffer), "%dF", (int)temp_tuple->value->int32);
    

    // Assemble full string and display
    snprintf(weather_layer_buffer, sizeof(weather_layer_buffer), "%s-%s", temperature_buffer, conditions_buffer);
    text_layer_set_text(s_weather_layer, weather_layer_buffer);
  }
}

static void inbox_dropped_callback(AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Message dropped!");
}

static void outbox_failed_callback(DictionaryIterator *iterator, AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Outbox send failed!");
}

static void outbox_sent_callback(DictionaryIterator *iterator, void *context) {
  APP_LOG(APP_LOG_LEVEL_INFO, "Outbox send success!");
  
}

static void update_time() {
  // Get a tm structure
  time_t temp = time(NULL); 
  struct tm *tick_time = localtime(&temp);

  // Write the current hours and minutes into a buffer
  static char s_buffer[8];
  strftime(s_buffer, sizeof(s_buffer), clock_is_24h_style() ?
                                          "%H:%M" : "%I:%M", tick_time);

  // Display this time on the TextLayer
  text_layer_set_text(s_time_layer, s_buffer);
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  update_time();

  // Get weather update every 30 minutes
  if(tick_time->tm_min % 30 == 0) {
    // Begin dictionary
    DictionaryIterator *iter;
    app_message_outbox_begin(&iter);

    // Add a key-value pair
    dict_write_uint8(iter, 0, 0);

    // Send the message!
    app_message_outbox_send();
  }
}

static void main_window_load(Window *window) {
  // Get information about the Window
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);
  
  
  // Create BitmapLayer to display the GBitmap
  s_chest_layer = bitmap_layer_create(
  GRect(0, 20, bounds.size.w, 40));

  // Set the bitmap onto the layer and add to the window
  bitmap_layer_set_bitmap(s_chest_layer, s_chest_bitmap);
  layer_add_child(window_layer, bitmap_layer_get_layer(s_chest_layer));
  
  // Create logo layer
  s_logo_layer = text_layer_create(
  GRect(0, 146, bounds.size.w, 20));
  
  // Fill logo
  text_layer_set_text_color(s_logo_layer, GColorBlack);
  text_layer_set_text(s_logo_layer, "WEARCAST");
  text_layer_set_text_alignment(s_logo_layer, GTextAlignmentCenter);
  s_logo_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_TXT_12));
  text_layer_set_font(s_logo_layer, s_logo_font);
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_logo_layer));
  
  // Create the TextLayer with specific bounds
  s_time_layer = text_layer_create(
      GRect(0, 94, bounds.size.w, 50));

  // Improve the layout to be more like a watchface
  //text_layer_set_background_color(s_time_layer, GColorClear);
  text_layer_set_text_color(s_time_layer, GColorBlack);
  text_layer_set_text(s_time_layer, "00:00");
  text_layer_set_text_alignment(s_time_layer, GTextAlignmentCenter);

  // Create GFont
  s_time_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_BLACK_FOREST_48));

  // Apply to TextLayer
  text_layer_set_font(s_time_layer, s_time_font);

  // Add it as a child layer to the Window's root layer
  layer_add_child(window_layer, text_layer_get_layer(s_time_layer));

  // Create temperature Layer
  s_weather_layer = text_layer_create(
      GRect(0, 86, bounds.size.w, 18));

  // Style the text
  //text_layer_set_background_color(s_weather_layer, GColorClear);
  text_layer_set_text_color(s_weather_layer, GColorBlack);
  text_layer_set_text_alignment(s_weather_layer, GTextAlignmentCenter);
  text_layer_set_text(s_weather_layer, "Loading...");

  // Create second custom font, apply it and add to Window
  s_weather_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_TXT_18));
  text_layer_set_font(s_weather_layer, s_weather_font);
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_weather_layer));
}

static void main_window_unload(Window *window) {
  // Destroy TextLayer
  text_layer_destroy(s_time_layer);

  // Unload GFont
  fonts_unload_custom_font(s_time_font);
  
  // Destroy weather elements
  text_layer_destroy(s_weather_layer);
  fonts_unload_custom_font(s_weather_font);
}


static void init() {
  // Create main Window element and assign to pointer
  s_main_window = window_create();

  // Set handlers to manage the elements inside the Window
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload
  });

  // Show the Window on the watch, with animated=true
  window_stack_push(s_main_window, true);

  // Make sure the time is displayed from the start
  update_time();

  // Register with TickTimerService
  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);

  // Register callbacks
  app_message_register_inbox_received(inbox_received_callback);
  app_message_register_inbox_dropped(inbox_dropped_callback);
  app_message_register_outbox_failed(outbox_failed_callback);
  app_message_register_outbox_sent(outbox_sent_callback);

  // Open AppMessage
  app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());
}

static void deinit() {
  // Destroy GBitmap
  gbitmap_destroy(s_chest_bitmap);

  // Destroy BitmapLayer
  bitmap_layer_destroy(s_chest_layer);
  
  // Destroy Window
  window_destroy(s_main_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}

