
#include <pebble.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#define SETTINGS_KEY 42
#define COLOR_NAME_LEN 8

typedef enum {
  SEG_GRAY = 0,
  SEG_GREEN,
  SEG_BLUE,
  SEG_ORANGE,
  SEG_YELLOW,
  SEG_WHITE
} SegmentColor;

typedef struct {
  uint32_t background_color;
  uint32_t date_color;
  uint32_t time_color;
  bool show_seconds;
  bool blink_colon;
  bool show_inactive;
  uint8_t segment_color;
  uint8_t temperature_unit; // 0 = Celsius, 1 = Fahrenheit
} Settings;

static Settings s_settings;

static Window *s_window;
static Layer *s_canvas;

static GBitmap *s_large_off;
static GBitmap *s_small_off;
static GBitmap *s_large[10];
static GBitmap *s_small[10];

static GBitmap *s_colon;
static GBitmap *s_degree;
static GBitmap *s_battery[11];
//static GBitmap *s_bt_on;
//static GBitmap *s_bt_off;
static GBitmap *s_cursor;

static char s_location[32] = "LOCALISATION";
static int s_temperature = 0;
static bool s_temperature_valid = false;
static int s_battery_level = 100;
static bool s_bt_connected = true;
static bool s_colon_visible = true;

static GColor bg_color(void) {
  return GColorFromHEX(s_settings.background_color);
}

static GColor date_color(void) {
  return GColorFromHEX(s_settings.date_color);
}

static GColor time_color(void) {
  return GColorFromHEX(s_settings.time_color);
}

static void draw_bitmap(GContext *ctx, GBitmap *bmp, int x, int y) {
  if (!bmp) return;
  graphics_context_set_compositing_mode(ctx, GCompOpSet);
  graphics_draw_bitmap_in_rect(
    ctx,
    bmp,
    GRect(x, y,
      gbitmap_get_bounds(bmp).size.w,
      gbitmap_get_bounds(bmp).size.h)
  );
}

static void destroy_digit_sets(void) {
  for (int i = 0; i < 10; i++) {
    if (s_large[i]) {
      gbitmap_destroy(s_large[i]);
      s_large[i] = NULL;
    }
    if (s_small[i]) {
      gbitmap_destroy(s_small[i]);
      s_small[i] = NULL;
    }
  }
}

static const uint32_t large_ids[6][10] = {
  {
    RESOURCE_ID_IMG_LARGE_GRAY_0, RESOURCE_ID_IMG_LARGE_GRAY_1,
    RESOURCE_ID_IMG_LARGE_GRAY_2, RESOURCE_ID_IMG_LARGE_GRAY_3,
    RESOURCE_ID_IMG_LARGE_GRAY_4, RESOURCE_ID_IMG_LARGE_GRAY_5,
    RESOURCE_ID_IMG_LARGE_GRAY_6, RESOURCE_ID_IMG_LARGE_GRAY_7,
    RESOURCE_ID_IMG_LARGE_GRAY_8, RESOURCE_ID_IMG_LARGE_GRAY_9
  },
  {
    RESOURCE_ID_IMG_LARGE_GREEN_0, RESOURCE_ID_IMG_LARGE_GREEN_1,
    RESOURCE_ID_IMG_LARGE_GREEN_2, RESOURCE_ID_IMG_LARGE_GREEN_3,
    RESOURCE_ID_IMG_LARGE_GREEN_4, RESOURCE_ID_IMG_LARGE_GREEN_5,
    RESOURCE_ID_IMG_LARGE_GREEN_6, RESOURCE_ID_IMG_LARGE_GREEN_7,
    RESOURCE_ID_IMG_LARGE_GREEN_8, RESOURCE_ID_IMG_LARGE_GREEN_9
  },
  {
    RESOURCE_ID_IMG_LARGE_BLUE_0, RESOURCE_ID_IMG_LARGE_BLUE_1,
    RESOURCE_ID_IMG_LARGE_BLUE_2, RESOURCE_ID_IMG_LARGE_BLUE_3,
    RESOURCE_ID_IMG_LARGE_BLUE_4, RESOURCE_ID_IMG_LARGE_BLUE_5,
    RESOURCE_ID_IMG_LARGE_BLUE_6, RESOURCE_ID_IMG_LARGE_BLUE_7,
    RESOURCE_ID_IMG_LARGE_BLUE_8, RESOURCE_ID_IMG_LARGE_BLUE_9
  },
  {
    RESOURCE_ID_IMG_LARGE_ORANGE_0, RESOURCE_ID_IMG_LARGE_ORANGE_1,
    RESOURCE_ID_IMG_LARGE_ORANGE_2, RESOURCE_ID_IMG_LARGE_ORANGE_3,
    RESOURCE_ID_IMG_LARGE_ORANGE_4, RESOURCE_ID_IMG_LARGE_ORANGE_5,
    RESOURCE_ID_IMG_LARGE_ORANGE_6, RESOURCE_ID_IMG_LARGE_ORANGE_7,
    RESOURCE_ID_IMG_LARGE_ORANGE_8, RESOURCE_ID_IMG_LARGE_ORANGE_9
  },
  {
    RESOURCE_ID_IMG_LARGE_YELLOW_0, RESOURCE_ID_IMG_LARGE_YELLOW_1,
    RESOURCE_ID_IMG_LARGE_YELLOW_2, RESOURCE_ID_IMG_LARGE_YELLOW_3,
    RESOURCE_ID_IMG_LARGE_YELLOW_4, RESOURCE_ID_IMG_LARGE_YELLOW_5,
    RESOURCE_ID_IMG_LARGE_YELLOW_6, RESOURCE_ID_IMG_LARGE_YELLOW_7,
    RESOURCE_ID_IMG_LARGE_YELLOW_8, RESOURCE_ID_IMG_LARGE_YELLOW_9
  },
  {
    RESOURCE_ID_IMG_LARGE_WHITE_0, RESOURCE_ID_IMG_LARGE_WHITE_1,
    RESOURCE_ID_IMG_LARGE_WHITE_2, RESOURCE_ID_IMG_LARGE_WHITE_3,
    RESOURCE_ID_IMG_LARGE_WHITE_4, RESOURCE_ID_IMG_LARGE_WHITE_5,
    RESOURCE_ID_IMG_LARGE_WHITE_6, RESOURCE_ID_IMG_LARGE_WHITE_7,
    RESOURCE_ID_IMG_LARGE_WHITE_8, RESOURCE_ID_IMG_LARGE_WHITE_9
  }
};

static const uint32_t small_ids[6][10] = {
  {
    RESOURCE_ID_IMG_SMALL_GRAY_0, RESOURCE_ID_IMG_SMALL_GRAY_1,
    RESOURCE_ID_IMG_SMALL_GRAY_2, RESOURCE_ID_IMG_SMALL_GRAY_3,
    RESOURCE_ID_IMG_SMALL_GRAY_4, RESOURCE_ID_IMG_SMALL_GRAY_5,
    RESOURCE_ID_IMG_SMALL_GRAY_6, RESOURCE_ID_IMG_SMALL_GRAY_7,
    RESOURCE_ID_IMG_SMALL_GRAY_8, RESOURCE_ID_IMG_SMALL_GRAY_9
  },
  {
    RESOURCE_ID_IMG_SMALL_GREEN_0, RESOURCE_ID_IMG_SMALL_GREEN_1,
    RESOURCE_ID_IMG_SMALL_GREEN_2, RESOURCE_ID_IMG_SMALL_GREEN_3,
    RESOURCE_ID_IMG_SMALL_GREEN_4, RESOURCE_ID_IMG_SMALL_GREEN_5,
    RESOURCE_ID_IMG_SMALL_GREEN_6, RESOURCE_ID_IMG_SMALL_GREEN_7,
    RESOURCE_ID_IMG_SMALL_GREEN_8, RESOURCE_ID_IMG_SMALL_GREEN_9
  },
  {
    RESOURCE_ID_IMG_SMALL_BLUE_0, RESOURCE_ID_IMG_SMALL_BLUE_1,
    RESOURCE_ID_IMG_SMALL_BLUE_2, RESOURCE_ID_IMG_SMALL_BLUE_3,
    RESOURCE_ID_IMG_SMALL_BLUE_4, RESOURCE_ID_IMG_SMALL_BLUE_5,
    RESOURCE_ID_IMG_SMALL_BLUE_6, RESOURCE_ID_IMG_SMALL_BLUE_7,
    RESOURCE_ID_IMG_SMALL_BLUE_8, RESOURCE_ID_IMG_SMALL_BLUE_9
  },
  {
    RESOURCE_ID_IMG_SMALL_ORANGE_0, RESOURCE_ID_IMG_SMALL_ORANGE_1,
    RESOURCE_ID_IMG_SMALL_ORANGE_2, RESOURCE_ID_IMG_SMALL_ORANGE_3,
    RESOURCE_ID_IMG_SMALL_ORANGE_4, RESOURCE_ID_IMG_SMALL_ORANGE_5,
    RESOURCE_ID_IMG_SMALL_ORANGE_6, RESOURCE_ID_IMG_SMALL_ORANGE_7,
    RESOURCE_ID_IMG_SMALL_ORANGE_8, RESOURCE_ID_IMG_SMALL_ORANGE_9
  },
  {
    RESOURCE_ID_IMG_SMALL_YELLOW_0, RESOURCE_ID_IMG_SMALL_YELLOW_1,
    RESOURCE_ID_IMG_SMALL_YELLOW_2, RESOURCE_ID_IMG_SMALL_YELLOW_3,
    RESOURCE_ID_IMG_SMALL_YELLOW_4, RESOURCE_ID_IMG_SMALL_YELLOW_5,
    RESOURCE_ID_IMG_SMALL_YELLOW_6, RESOURCE_ID_IMG_SMALL_YELLOW_7,
    RESOURCE_ID_IMG_SMALL_YELLOW_8, RESOURCE_ID_IMG_SMALL_YELLOW_9
  },
  {
    RESOURCE_ID_IMG_SMALL_WHITE_0, RESOURCE_ID_IMG_SMALL_WHITE_1,
    RESOURCE_ID_IMG_SMALL_WHITE_2, RESOURCE_ID_IMG_SMALL_WHITE_3,
    RESOURCE_ID_IMG_SMALL_WHITE_4, RESOURCE_ID_IMG_SMALL_WHITE_5,
    RESOURCE_ID_IMG_SMALL_WHITE_6, RESOURCE_ID_IMG_SMALL_WHITE_7,
    RESOURCE_ID_IMG_SMALL_WHITE_8, RESOURCE_ID_IMG_SMALL_WHITE_9
  }
};

static void load_digit_sets(void) {
  destroy_digit_sets();

  uint8_t c = s_settings.segment_color;
  if (c > SEG_WHITE) c = SEG_GRAY;
  s_settings.segment_color = c;

  for (int i = 0; i < 10; i++) {
    s_large[i] = gbitmap_create_with_resource(large_ids[c][i]);
    s_small[i] = gbitmap_create_with_resource(small_ids[c][i]);
  }
}

static uint8_t parse_temperature_unit(const char *value) {
  if (value && strcmp(value, "F") == 0) return 1;
  return 0;
}

static SegmentColor parse_segment_color(const char *value) {
  if (!value) return SEG_GRAY;
  if (strcmp(value, "green") == 0) return SEG_GREEN;
  if (strcmp(value, "blue") == 0) return SEG_BLUE;
  if (strcmp(value, "orange") == 0) return SEG_ORANGE;
  if (strcmp(value, "yellow") == 0) return SEG_YELLOW;
  if (strcmp(value, "white") == 0) return SEG_WHITE;
  return SEG_GRAY;
}

static void draw_digit(
  GContext *ctx,
  int d,
  int x,
  int y,
  bool large
) {
  if (d < 0 || d > 9) return;

  if (s_settings.show_inactive) {
    draw_bitmap(
      ctx,
      large ? s_large_off : s_small_off,
      x,
      y
    );
  }

  draw_bitmap(
    ctx,
    large ? s_large[d] : s_small[d],
    x,
    y
  );
}

static void draw_text(
  GContext *ctx,
  const char *txt,
  GFont font,
  GRect rect,
  GColor color,
  GTextAlignment align
) {
  graphics_context_set_text_color(ctx, color);
  graphics_draw_text(
    ctx,
    txt,
    font,
    rect,
    GTextOverflowModeTrailingEllipsis,
    align,
    NULL
  );
}

static void draw_time(GContext *ctx, struct tm *t) {
  draw_digit(ctx, t->tm_hour / 10, 5, 31, true);
  draw_digit(ctx, t->tm_hour % 10, 35, 31, true);

  if (s_colon_visible) {
    draw_bitmap(ctx, s_colon, 67, 35);
  }

  draw_digit(ctx, t->tm_min / 10, 75, 31, true);
  draw_digit(ctx, t->tm_min % 10, 105, 31, true);

  if (s_settings.show_seconds) {
    draw_digit(ctx, t->tm_sec / 10, 116, 84, false);
    draw_digit(ctx, t->tm_sec % 10, 126, 84, false);
  }
}

static void draw_weather(GContext *ctx) {
  draw_text(
    ctx,
    s_location,
    fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD),
    GRect(4, 103, 94, 20),
    date_color(),
    GTextAlignmentLeft
  );

  char temp[12];
  int display_temperature = s_temperature;

  if (s_settings.temperature_unit == 1) {
    // Open-Meteo supplies Celsius. Convert only for display.
    display_temperature = (int)((s_temperature * 9 + 2) / 5 + 32);
  }

  if (s_temperature_valid) {
    snprintf(temp, sizeof(temp), "%d", display_temperature);
  } else {
    snprintf(temp, sizeof(temp), "--");
  }

  draw_text(
    ctx,
    temp,
    fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD),
    GRect(99, 101, 32, 23),
    time_color(),
    GTextAlignmentRight
  );

  draw_bitmap(ctx, s_degree, 132, 101);

  draw_text(
    ctx,
    s_settings.temperature_unit == 1 ? "F" : "C",
    fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD),
    GRect(137, 103, 8, 18),
    time_color(),
    GTextAlignmentLeft
  );
}

static void draw_battery(GContext *ctx) {
  draw_text(
    ctx,
    "BAT",
    fonts_get_system_font(FONT_KEY_GOTHIC_09),
    GRect(3, 132, 23, 12),
    date_color(),
    GTextAlignmentLeft
  );

  int index = (s_battery_level + 5) / 10;
  if (index < 0) index = 0;
  if (index > 10) index = 10;

  draw_bitmap(ctx, s_battery[index], 27, 132);
}

static void draw_weekdays(GContext *ctx, struct tm *t) {
  static const char *days[7] = {"L","M","M","J","V","S","D"};
  int current = (t->tm_wday == 0) ? 6 : t->tm_wday - 1;

  for (int i = 0; i < 7; i++) {
    int x = 4 + i * 14;

    draw_text(
      ctx,
      days[i],
      fonts_get_system_font(FONT_KEY_GOTHIC_09),
      GRect(x, 145, 12, 13),
      date_color(),
      GTextAlignmentCenter
    );

    if (i == current) {
      draw_bitmap(ctx, s_cursor, x + 1, 156);
    }
  }
}

static void canvas_update(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);

  graphics_context_set_fill_color(ctx, bg_color());
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);

  time_t now = time(NULL);
  struct tm *t = localtime(&now);

  char date[8];
  snprintf(
    date,
    sizeof(date),
    "%02d-%02d",
    t->tm_mday,
    t->tm_mon + 1
  );

  draw_text(
    ctx,
    date,
    fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD),
    GRect(5, 4, 55, 18),
    date_color(),
    GTextAlignmentLeft
  );
/*
  draw_bitmap(
    ctx,
    s_bt_connected ? s_bt_on : s_bt_off,
    126,
    4
  );
*/
  draw_time(ctx, t);
  draw_weather(ctx);

  graphics_context_set_stroke_color(ctx, date_color());
  graphics_draw_line(
    ctx,
    GPoint(3, 128),
    GPoint(141, 128)
  );

  draw_battery(ctx);
  draw_weekdays(ctx, t);

}

static void battery_cb(BatteryChargeState state) {
  s_battery_level = state.charge_percent;
  layer_mark_dirty(s_canvas);
}

static void bt_cb(bool connected) {
  s_bt_connected = connected;
  layer_mark_dirty(s_canvas);
}

static void save_settings(void) {
  persist_write_data(
    SETTINGS_KEY,
    &s_settings,
    sizeof(s_settings)
  );
}

static void inbox_received(DictionaryIterator *iter, void *ctx) {
  bool reload_digits = false;

  Tuple *v;

  if ((v = dict_find(iter, MESSAGE_KEY_BackgroundColor))) {
    s_settings.background_color = v->value->int32;
  }

  if ((v = dict_find(iter, MESSAGE_KEY_DateColor))) {
    s_settings.date_color = v->value->int32;
  }

  if ((v = dict_find(iter, MESSAGE_KEY_TimeColor))) {
    s_settings.time_color = v->value->int32;
  }

  if ((v = dict_find(iter, MESSAGE_KEY_ShowSeconds))) {
    s_settings.show_seconds = v->value->int32 != 0;
  }

  if ((v = dict_find(iter, MESSAGE_KEY_BlinkColon))) {
    s_settings.blink_colon = v->value->int32 != 0;

    if (!s_settings.blink_colon) {
      s_colon_visible = true;
    }
  }

  if ((v = dict_find(iter, MESSAGE_KEY_ShowInactiveSegments))) {
    s_settings.show_inactive = v->value->int32 != 0;
  }

  if ((v = dict_find(iter, MESSAGE_KEY_SegmentColor))) {
    SegmentColor new_color =
      parse_segment_color(v->value->cstring);

    if (new_color != s_settings.segment_color) {
      s_settings.segment_color = new_color;
      reload_digits = true;
    }
  }

  // Weather data comes from PebbleKit JS. The previous version received
  // these messages but never copied them into the watchface variables.
  if ((v = dict_find(iter, MESSAGE_KEY_LOCATION)) &&
      v->type == TUPLE_CSTRING) {
    strncpy(s_location, v->value->cstring, sizeof(s_location) - 1);
    s_location[sizeof(s_location) - 1] = '\0';
  }

  if ((v = dict_find(iter, MESSAGE_KEY_TEMPERATURE)) &&
      (v->type == TUPLE_INT || v->type == TUPLE_UINT)) {
    s_temperature = v->value->int32;
    s_temperature_valid = true;
  }

  if ((v = dict_find(iter, MESSAGE_KEY_TemperatureUnit))) {
    uint8_t unit = parse_temperature_unit(v->value->cstring);
    s_settings.temperature_unit = unit;
  }

  save_settings();

  if (reload_digits) {
    load_digit_sets();
  }

  layer_mark_dirty(s_canvas);
}

static void tick_handler(
  struct tm *tick_time,
  TimeUnits units_changed
) {
  if (s_settings.blink_colon &&
      (units_changed & SECOND_UNIT)) {
    s_colon_visible = !s_colon_visible;
  }

  layer_mark_dirty(s_canvas);
}

static void load_settings(void) {
  s_settings.background_color = 0x000000;
  s_settings.date_color = 0xFFFFFF;
  s_settings.time_color = 0xFFFFFF;
  s_settings.show_seconds = true;
  s_settings.blink_colon = true;
  s_settings.show_inactive = true;
  s_settings.segment_color = SEG_GRAY;
  s_settings.temperature_unit = 0;

  if (persist_exists(SETTINGS_KEY)) {
    persist_read_data(
      SETTINGS_KEY,
      &s_settings,
      sizeof(s_settings)
    );

    if (s_settings.segment_color > SEG_WHITE) {
      s_settings.segment_color = SEG_GRAY;
    }
  }
}

static void window_load(Window *window) {
  Layer *root = window_get_root_layer(window);

  s_canvas = layer_create(
    layer_get_bounds(root)
  );

  layer_set_update_proc(
    s_canvas,
    canvas_update
  );

  layer_add_child(root, s_canvas);
}

static void window_unload(Window *window) {
  if (s_canvas) {
    layer_destroy(s_canvas);
    s_canvas = NULL;
  }
}

static void init(void) {
  load_settings();

  s_large_off =
    gbitmap_create_with_resource(
      RESOURCE_ID_IMG_LARGE_OFF
    );

  s_small_off =
    gbitmap_create_with_resource(
      RESOURCE_ID_IMG_SMALL_OFF
    );

  s_colon =
    gbitmap_create_with_resource(
      RESOURCE_ID_IMG_COLON
    );

  s_degree =
    gbitmap_create_with_resource(
      RESOURCE_ID_IMG_DEGREE
    );
/*
  s_bt_on =
    gbitmap_create_with_resource(
      RESOURCE_ID_IMG_BT_ON
    );

  s_bt_off =
    gbitmap_create_with_resource(
      RESOURCE_ID_IMG_BT_OFF
    );
*/
  s_cursor =
    gbitmap_create_with_resource(
      RESOURCE_ID_IMG_DAY_CURSOR
    );


  s_battery[0] =
    gbitmap_create_with_resource(RESOURCE_ID_IMG_BATTERY_000);
  s_battery[1] =
    gbitmap_create_with_resource(RESOURCE_ID_IMG_BATTERY_010);
  s_battery[2] =
    gbitmap_create_with_resource(RESOURCE_ID_IMG_BATTERY_020);
  s_battery[3] =
    gbitmap_create_with_resource(RESOURCE_ID_IMG_BATTERY_030);
  s_battery[4] =
    gbitmap_create_with_resource(RESOURCE_ID_IMG_BATTERY_040);
  s_battery[5] =
    gbitmap_create_with_resource(RESOURCE_ID_IMG_BATTERY_050);
  s_battery[6] =
    gbitmap_create_with_resource(RESOURCE_ID_IMG_BATTERY_060);
  s_battery[7] =
    gbitmap_create_with_resource(RESOURCE_ID_IMG_BATTERY_070);
  s_battery[8] =
    gbitmap_create_with_resource(RESOURCE_ID_IMG_BATTERY_080);
  s_battery[9] =
    gbitmap_create_with_resource(RESOURCE_ID_IMG_BATTERY_090);
  s_battery[10] =
    gbitmap_create_with_resource(RESOURCE_ID_IMG_BATTERY_100);

  load_digit_sets();

  s_window = window_create();

  window_set_background_color(
    s_window,
    bg_color()
  );

  window_set_window_handlers(
    s_window,
    (WindowHandlers) {
      .load = window_load,
      .unload = window_unload
    }
  );

  battery_state_service_subscribe(battery_cb);

  s_battery_level =
    battery_state_service_peek().charge_percent;

  connection_service_subscribe(
    (ConnectionHandlers) {
      .pebble_app_connection_handler = bt_cb,
      .pebblekit_connection_handler = NULL
    }
  );

  s_bt_connected =
    connection_service_peek_pebble_app_connection();

  app_message_register_inbox_received(inbox_received);

  app_message_open(512, 512);

  TimeUnits units = MINUTE_UNIT;
  if (s_settings.show_seconds ||
      s_settings.blink_colon) {
    units |= SECOND_UNIT;
  }

  tick_timer_service_subscribe(
    units,
    tick_handler
  );

  window_stack_push(s_window, true);
}

static void deinit(void) {
  tick_timer_service_unsubscribe();
  battery_state_service_unsubscribe();
  connection_service_unsubscribe();
  app_message_deregister_callbacks();

  destroy_digit_sets();

  if (s_large_off) gbitmap_destroy(s_large_off);
  if (s_small_off) gbitmap_destroy(s_small_off);
  if (s_colon) gbitmap_destroy(s_colon);
  if (s_degree) gbitmap_destroy(s_degree);
  //if (s_bt_on) gbitmap_destroy(s_bt_on);
  //if (s_bt_off) gbitmap_destroy(s_bt_off);
  if (s_cursor) gbitmap_destroy(s_cursor);

  for (int i = 0; i < 11; i++) {
    if (s_battery[i]) gbitmap_destroy(s_battery[i]);
  }

  window_destroy(s_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
