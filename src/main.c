#include <pebble.h>

#define KEY_TEMPERATURE 0
#define KEY_CONDITIONS 1

// info layers
static Window *s_main_window;

// text layers
static TextLayer *s_logo_layer;
static TextLayer *s_time_layer;
static TextLayer *s_temperature_layer;

// visual layers
static BitmapLayer *s_head_layer;
static BitmapLayer *s_chest_layer;
static BitmapLayer *s_legs_layer;
static BitmapLayer *s_umbrella_layer;
static BitmapLayer *s_weather_layer;

// bitmaps
static GBitmap *s_head_bitmap;
static GBitmap *s_chest_bitmap;
static GBitmap *s_legs_bitmap;
static GBitmap *s_umbrella_bitmap;
static GBitmap *s_weather_bitmap;

static const uint32_t WEATHER_ICONS[] = {
  RESOURCE_ID_IMAGE_SUNNY,  //0
  RESOURCE_ID_IMAGE_CLOUDY, //1
  RESOURCE_ID_IMAGE_RAIN,   //2
  RESOURCE_ID_IMAGE_SNOW    //3
};
static const uint32_t HEAD_ICONS[] = {
  RESOURCE_ID_IMAGE_EYES, // 0
  RESOURCE_ID_IMAGE_HAT   // 1
};

static const uint32_t CHEST_ICONS[] = {
  RESOURCE_ID_IMAGE_COAT,        // 0
  RESOURCE_ID_IMAGE_RAIN_JACKET, // 1
  RESOURCE_ID_IMAGE_SWEATER,     // 2
  RESOURCE_ID_IMAGE_LONG_SLEEVE, // 3
  RESOURCE_ID_IMAGE_SHORT_SLEEVE // 4
};

static const uint32_t LEGS_ICONS[] = {
  RESOURCE_ID_IMAGE_PANTS_BOOTS, // 0
  RESOURCE_ID_IMAGE_PANTS_SHOES, // 1
  RESOURCE_ID_IMAGE_SHORTS_SHOES // 2
};

static const uint32_t UMBRELLA_ICON =
  RESOURCE_ID_IMAGE_UMBRELLA;

static uint8_t current_weather = 0;
static int current_temp = 0;

static void update_time() {
  // get tm structure
  time_t temp = time(NULL);
  struct tm *tick_time = localtime(&temp);
  
  // write current time into buffer
  static char s_buffer[8];
  strftime(s_buffer, sizeof(s_buffer), clock_is_24h_style() ? "%H:%M" : "%I:%M", tick_time);
  
  //display time on text layer
  text_layer_set_text(s_time_layer, s_buffer);
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  update_time();
}

static int calculate_head() {
  if (current_temp >= 42 && (current_weather == 0 || current_weather == 1))
      return 0;
  else
    return 1;
}

static int calculate_chest() {
  if (current_weather == 3 || current_temp <= 42)
    return 0; // coat
  else if (current_weather == 2)
    return 1; // rain jacket
  else if (current_temp <= 50)
    return 2; // sweater
  else if (current_temp <= 60)
    return 3; // long sleeve
  else
    return 4; // short sleeve
}

static int calculate_legs() {
  if (current_weather == 2 || current_weather == 3)
    return 0; // pants boots
  else if (current_temp <= 60)
    return 1; // pants shoes
  else
    return 2; // shorts shoes
}

static Window *s_main_window;

static AppSync s_sync;
static uint8_t s_sync_buffer[64];


enum WeatherKey {
  WEATHER_ICON_KEY = 0x0,         // TUPLE_INT
  WEATHER_TEMPERATURE_KEY = 0x1,  // TUPLE_CSTRING
};

static void sync_error_callback(DictionaryResult dict_error, AppMessageResult app_message_error, void *context) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "App Message Sync Error: %d", app_message_error);
}

static void sync_tuple_changed_callback(const uint32_t key, const Tuple* new_tuple, const Tuple* old_tuple, void* context) {
  static char temperature_buffer[8];
  static char conditions_buffer[32];
  static char temperature_layer_buffer[32];
  
  switch (key) {
    case WEATHER_ICON_KEY:
      current_weather = new_tuple->value->uint8;
      switch (current_weather) {
        case 0:
          snprintf(conditions_buffer, sizeof(conditions_buffer), "SUNNY");
          break;
        case 1:
          snprintf(conditions_buffer, sizeof(conditions_buffer), "CLOUDY");
          break;
        case 2:
          snprintf(conditions_buffer, sizeof(conditions_buffer), "RAIN");
          break;
        case 3:
          snprintf(conditions_buffer, sizeof(conditions_buffer), "SNOW");
          break;
      }
      break;
    case WEATHER_TEMPERATURE_KEY:
      // App Sync keeps new_tuple in s_sync_buffer, so we may use it directly
      snprintf(temperature_buffer, sizeof(temperature_buffer), "%s", new_tuple->value->cstring);
      
      current_temp = new_tuple->value->uint8;
      
    // Assemble full string and display
    snprintf(temperature_layer_buffer, sizeof(temperature_layer_buffer), "%sF-%s", temperature_buffer, conditions_buffer);
    text_layer_set_text(s_temperature_layer, temperature_layer_buffer);
  }
  
  
  s_head_bitmap = gbitmap_create_with_resource(HEAD_ICONS[calculate_head()]);
  bitmap_layer_set_bitmap(s_head_layer, s_head_bitmap);
  s_chest_bitmap = gbitmap_create_with_resource(CHEST_ICONS[calculate_chest()]);
  bitmap_layer_set_bitmap(s_chest_layer, s_chest_bitmap);
  s_legs_bitmap = gbitmap_create_with_resource(LEGS_ICONS[calculate_legs()]);
  bitmap_layer_set_bitmap(s_legs_layer, s_legs_bitmap);
  if (current_weather == 2) {
    s_umbrella_bitmap = gbitmap_create_with_resource(UMBRELLA_ICON);
    bitmap_layer_set_bitmap(s_umbrella_layer, s_umbrella_bitmap);
  }
  else {
    bitmap_layer_set_bitmap(s_umbrella_layer, NULL);
  }
  
  s_weather_bitmap = gbitmap_create_with_resource(WEATHER_ICONS[current_weather]);
  bitmap_layer_set_bitmap(s_weather_layer, s_weather_bitmap);
}

static void request_weather(void) {
  DictionaryIterator *iter;
  app_message_outbox_begin(&iter);

  if (!iter) {
    // Error creating outbound message
    return;
  }

  int value = 1;
  dict_write_int(iter, 1, &value, sizeof(int32_t), true);
  dict_write_end(iter);

  app_message_outbox_send();
}

static void window_load(Window *window) {
  // get info about window
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  s_logo_layer = text_layer_create(GRect(0, 150, bounds.size.w, 12));
  text_layer_set_background_color(s_logo_layer, GColorClear);
  text_layer_set_text_color(s_logo_layer, GColorBlack);
  text_layer_set_font(s_logo_layer, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_TXT_12)));
  text_layer_set_text(s_logo_layer, "WEARCAST");
  text_layer_set_text_alignment(s_logo_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(s_logo_layer));
  
  // style time layer
  s_time_layer = text_layer_create(GRect(0, 100, bounds.size.w, 50));
  text_layer_set_background_color(s_time_layer, GColorClear);
  text_layer_set_text_color(s_time_layer, GColorBlack);
  text_layer_set_font(s_time_layer, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_BLACK_FOREST_48)));
  text_layer_set_text(s_time_layer, "00:00");
  text_layer_set_text_alignment(s_time_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(s_time_layer));
  
  // style temp layer
  s_temperature_layer = text_layer_create(GRect(0, 97, bounds.size.w, 20));
  text_layer_set_background_color(s_temperature_layer, GColorClear);
  text_layer_set_text_color(s_temperature_layer, GColorBlack);
  text_layer_set_font(s_temperature_layer, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_TXT_18)));
  text_layer_set_text_alignment(s_temperature_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(s_temperature_layer));
  
  // add icons to root
  s_head_layer = bitmap_layer_create(
    GRect(0, 7, bounds.size.w, 30));
  s_chest_layer = bitmap_layer_create(
    GRect(0, 28, bounds.size.w, 30));
  s_legs_layer = bitmap_layer_create(
    GRect(0, 54, bounds.size.w, 50));
  s_umbrella_layer = bitmap_layer_create(
    GRect(33, 33, bounds.size.w, 30));
  s_weather_layer = bitmap_layer_create(
    GRect(10, 15, 30, 15));
  layer_add_child(window_layer, bitmap_layer_get_layer(s_head_layer));
  layer_add_child(window_layer, bitmap_layer_get_layer(s_chest_layer));
  layer_add_child(window_layer, bitmap_layer_get_layer(s_legs_layer));
  layer_add_child(window_layer, bitmap_layer_get_layer(s_umbrella_layer));
  layer_add_child(window_layer, bitmap_layer_get_layer(s_weather_layer));
  
  Tuplet initial_values[] = {
    TupletInteger(WEATHER_ICON_KEY, (uint8_t) 1),
    TupletCString(WEATHER_TEMPERATURE_KEY, "")
  };

  app_sync_init(&s_sync, s_sync_buffer, sizeof(s_sync_buffer), 
      initial_values, ARRAY_LENGTH(initial_values),
      sync_tuple_changed_callback, sync_error_callback, NULL
  );

  request_weather();
}

static void window_unload(Window *window) {
  
  // destroy text
  text_layer_destroy(s_time_layer);
  text_layer_destroy(s_temperature_layer);
  text_layer_destroy(s_logo_layer);
  
  // destroy icons
  bitmap_layer_destroy(s_head_layer);
  bitmap_layer_destroy(s_chest_layer);
  bitmap_layer_destroy(s_legs_layer);
  bitmap_layer_destroy(s_umbrella_layer);
  bitmap_layer_destroy(s_weather_layer);
}

static void init(void) {
  // create window
  s_main_window = window_create();
  
  //set handlers
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload
  });
  
  //TODO remove
  window_set_background_color(s_main_window, GColorWhite);
  
  // show window on face
  window_stack_push(s_main_window, true);
  
  // display time from the start
  update_time();
  
  // register with TickTimerService
  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);

  app_message_open(64, 64);
}

static void deinit(void) {
  window_destroy(s_main_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}