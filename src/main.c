#include <pebble.h>

static Window *s_main_window;
static TextLayer *s_time_layer;
static TextLayer *s_weather_layer;

static BitmapLayer *s_icon_layer;
static GBitmap *s_head_bitmap = NULL;
static GBitmap *s_chest_bitmap = NULL;
static GBitmap *s_legs_bitmap = NULL;
static GBitmap *s_umbrella_bitmap = NULL;

static const uint32_t head_icons[] = {
  RESOURCE_ID_IMAGE_EYES,
  RESOURCE_ID_IMAGE_HAT
};
static const uint32_t chest_icons[] = {
  RESOURCE_ID_IMAGE_COAT,
  RESOURCE_ID_IMAGE_SWEATER,
  RESOURCE_ID_IMAGE_RAIN_JACKET,
  RESOURCE_ID_IMAGE_LONG_SLEEVE,
  RESOURCE_ID_IMAGE_SHORT_SLEEVE
};
static const uint32_t legs_icons[] = {
  RESOURCE_ID_IMAGE_PANTS_BOOTS,
  RESOURCE_ID_IMAGE_PANTS_SHOES,
  RESOURCE_ID_IMAGE_SHORTS_SHOES
};
static const uint32_t umbrella_icons = 
  RESOURCE_ID_IMAGE_UMBRELLA;

static Window *s_main_window;

static TextLayer *s_temperature_layer;
static TextLayer *s_city_layer;
static BitmapLayer *s_icon_layer;


static AppSync s_sync;
static uint8_t s_sync_buffer[64];

static char code_buffer[9];

enum WeatherKey {
  WEATHER_ICON_KEY = 0x0,         // TUPLE_INT
  WEATHER_TEMPERATURE_KEY = 0x1,  // TUPLE_CSTRING
  WEATHER_CITY_KEY = 0x2,         // TUPLE_CSTRING
};

static void sync_error_callback(DictionaryResult dict_error, AppMessageResult app_message_error, void *context) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "App Message Sync Error: %d", app_message_error);
}

static void sync_tuple_changed_callback(const uint32_t key, const Tuple* new_tuple, const Tuple* old_tuple, void* context) {
  
  switch (key) {
    case WEATHER_ICON_KEY:
      snprintf(code_buffer, sizeof(code_buffer), "%u", new_tuple->value->uint8);

    case WEATHER_TEMPERATURE_KEY:
      // App Sync keeps new_tuple in s_sync_buffer, so we may use it directly
      text_layer_set_text(s_temperature_layer, new_tuple->value->cstring);
      break;

    case WEATHER_CITY_KEY:
      text_layer_set_text(s_city_layer, new_tuple->value->cstring);
      break;
  }
}

static int calculate_head(uint8_t weather, int temp) {
  if (temp >= 42 && (weather == 0 || weather == 1))
      return 0;
  else
    return 1;
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
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  s_icon_layer = bitmap_layer_create(GRect(32, 10, 80, 80));
  layer_add_child(window_layer, bitmap_layer_get_layer(s_icon_layer));

  s_temperature_layer = text_layer_create(GRect(0, 75, bounds.size.w, 68));
  text_layer_set_text_color(s_temperature_layer, GColorWhite);
  text_layer_set_background_color(s_temperature_layer, GColorClear);
  text_layer_set_font(s_temperature_layer, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD));
  text_layer_set_text_alignment(s_temperature_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(s_temperature_layer));

  s_city_layer = text_layer_create(GRect(0, 105, bounds.size.w, 68));
  text_layer_set_text_color(s_city_layer, GColorWhite);
  text_layer_set_background_color(s_city_layer, GColorClear);
  text_layer_set_font(s_city_layer, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD));
  text_layer_set_text_alignment(s_city_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(s_city_layer));

  Tuplet initial_values[] = {
    TupletInteger(WEATHER_ICON_KEY, (uint8_t) 1),
    TupletCString(WEATHER_TEMPERATURE_KEY, "1234\u00B0C"),
    TupletCString(WEATHER_CITY_KEY, "St Pebblesburg"),
  };

  app_sync_init(&s_sync, s_sync_buffer, sizeof(s_sync_buffer), 
      initial_values, ARRAY_LENGTH(initial_values),
      sync_tuple_changed_callback, sync_error_callback, NULL
  );

  request_weather();
}

static void window_unload(Window *window) {
  /*if (s_icon_bitmap) {
    gbitmap_destroy(s_icon_bitmap);
  }*/

  text_layer_destroy(s_city_layer);
  text_layer_destroy(s_temperature_layer);
  bitmap_layer_destroy(s_icon_layer);
}

static void init(void) {
  s_main_window = window_create();
  window_set_background_color(s_main_window, GColorBlack);
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload
  });
  window_stack_push(s_main_window, true);

  app_message_open(64, 64);
}

static void deinit(void) {
  window_destroy(s_main_window);

  app_sync_deinit(&s_sync);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}