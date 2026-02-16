#include <pebble.h>
#include "main.h"

static Window *s_main_window;

//ClaySettings settings;
static bool alt_date_fmt = false;

//layout changes for different size screens
static uint8_t cal_icon_text_x;
static uint8_t cal_icon_text_y;
static uint8_t ampm_pos_x;
static uint8_t ampm_pos_y;

static TextLayer *s_long_date_layer;
static TextLayer *s_date_layer;
static TextLayer *s_cal_icon_layer;
//static TextLayer *s_time_layer;
static TextLayer *s_ampm_layer;
static TextLayer *s_status_layer;
static TextLayer *s_battery_layer;
static TextLayer *s_connection_layer;
static BitmapLayer *s_bt_conn_layer;
//static BitmapLayer *s_bt_err_layer;
static BitmapLayer *mblyr;
static GBitmap *bmptr;
static GBitmap *btconnptr;
static GBitmap *bterrptr;
static GBitmap *btonptr;

//Digit images
static GBitmap *digit_0_ptr;
static GBitmap *digit_1_ptr;
static GBitmap *digit_2_ptr;
static GBitmap *digit_3_ptr;
static GBitmap *digit_4_ptr;
static GBitmap *digit_5_ptr;
static GBitmap *digit_6_ptr;
static GBitmap *digit_7_ptr;
static GBitmap *digit_8_ptr;
static GBitmap *digit_9_ptr;

//layers for flap digits
static BitmapLayer *s_time_1_layer;
static BitmapLayer *s_time_2_layer;
static BitmapLayer *s_time_3_layer;
static BitmapLayer *s_time_4_layer;

//static GFont flap_font;
static GFont status_font;

static void handle_battery(BatteryChargeState charge_state) {
  static char battery_text[] = "100%";

  if (charge_state.is_charging) {
    snprintf(battery_text, sizeof(battery_text), "+%d%%", charge_state.charge_percent);
  } else {
    snprintf(battery_text, sizeof(battery_text), "%d%%", charge_state.charge_percent);
  }
  text_layer_set_text(s_battery_layer, battery_text);
}

static GBitmap* get_digit(int val) {
  GBitmap* res = digit_0_ptr;
  if (val == 0) { res = digit_0_ptr; }
  else if (val == 1) { res = digit_1_ptr; }
  else if (val == 2) { res = digit_2_ptr; }
  else if (val == 3) { res = digit_3_ptr; }
  else if (val == 4) { res = digit_4_ptr; }
  else if (val == 5) { res = digit_5_ptr; }
  else if (val == 6) { res = digit_6_ptr; }
  else if (val == 7) { res = digit_7_ptr; }
  else if (val == 8) { res = digit_8_ptr; }
  else if (val == 9) { res = digit_9_ptr; }
  
  return res;
}

static void handle_second_tick(struct tm* tick_time, TimeUnits units_changed) {
  // Needs to be static because it's used by the system later.
  static char s_date_text[] = "02/05/09";
  static char s_long_date_text[] = "Wed";
  static char s_cal_icon_text[] = "11";
  static char s_ampm_text[] = "  ";

  APP_LOG(APP_LOG_LEVEL_WARNING, "%s %u", "second_tick, Alt date fmt: ", alt_date_fmt );
  
  //Future, long date format.. would need to go in clock app like where it belongs to fit
  //strftime(s_long_date_text, sizeof(s_long_date_text), "%A, %B %e, %Y", tick_time);
  strftime(s_long_date_text, sizeof(s_long_date_text), "%a", tick_time);
  text_layer_set_text(s_long_date_layer, s_long_date_text);
  
  if (alt_date_fmt) {
    strftime(s_date_text, sizeof(s_date_text), "%d.%m.%y", tick_time);    
  } else {
    strftime(s_date_text, sizeof(s_date_text), "%D", tick_time);
  }
  text_layer_set_text(s_date_layer, s_date_text);

  Layer *window_layer = window_get_root_layer(s_main_window);
  GRect bounds = layer_get_frame(window_layer);
  strftime(s_cal_icon_text, sizeof(s_cal_icon_text), "%e", tick_time);
  text_layer_set_text(s_cal_icon_layer, s_cal_icon_text);
  if (tick_time->tm_mday < 10) {
    layer_set_frame((Layer*)s_cal_icon_layer,GRect(cal_icon_text_x - 2, cal_icon_text_y, 12, 14)); //was 89
  } else {
    layer_set_frame((Layer*)s_cal_icon_layer,GRect(cal_icon_text_x - 1, cal_icon_text_y, 12, 14)); //was 90
  }

  //Set digits..
  int hr = tick_time->tm_hour;
  //hr = 0; 
  if (!clock_is_24h_style()) {
    if (hr > 12) { hr -= 12; }
    else if (hr == 0) { hr = 12; }
  }
  int hr1 = (int)(hr / 10);
  int hr2 = (int)(hr % 10);
  int min1 = (int)(tick_time->tm_min / 10);
  int min2 = (int)(tick_time->tm_min % 10);
  //if (hr > 9 || clock_is_24h_style()) {
  if (hr > 9) {
    layer_set_hidden((Layer*)s_time_1_layer, false);
    bitmap_layer_set_bitmap(s_time_1_layer,get_digit(hr1)); 
  } else { //if (!clock_is_24h_style()) { //leading zero only for military, not 24hr
    layer_set_hidden((Layer*)s_time_1_layer, true);
    //bitmap_layer_set_bitmap(s_time_1_layer,digit_0_ptr); 
  }
  bitmap_layer_set_bitmap(s_time_2_layer,get_digit(hr2));
  bitmap_layer_set_bitmap(s_time_3_layer,get_digit(min1));
  bitmap_layer_set_bitmap(s_time_4_layer,get_digit(min2));

  //Set AM/PM indicator if necessary
  if (!clock_is_24h_style()) {
    strftime(s_ampm_text, sizeof(s_ampm_text), "%p", tick_time);
    text_layer_set_text(s_ampm_layer, s_ampm_text);
  } else {
    text_layer_set_text(s_ampm_layer, "  ");
  }
  
}

static void handle_bluetooth(bool connected) {
  text_layer_set_text(s_connection_layer, connected ? "*" : "-");
  if (connected) {
    bitmap_layer_set_bitmap(s_bt_conn_layer,btconnptr);
  } else {
    bitmap_layer_set_bitmap(s_bt_conn_layer,bterrptr);
  }
}

static void inbox_received_callback(DictionaryIterator *iterator, void *context) {

  // Prefix is added automatically and helps distinguish a key from a local variable
  
  // Read boolean preferences
/*
  Tuple *date_fmt_val1 = dict_find(iterator, MESSAGE_KEY_DateFormat);
  if(date_fmt_val1) {
    alt_date_fmt = date_fmt_val1->value->int32 == 1;
    APP_LOG(APP_LOG_LEVEL_WARNING, "%s %u", "inbox_received_callback, Alt date fmt: ", alt_date_fmt );
    prv_save_settings(); //Persist new setting
    
    time_t now = time(NULL);
    struct tm *current_time = localtime(&now);
    handle_second_tick(current_time, MINUTE_UNIT);
  }
*/
  Tuple *date_fmt_val = dict_find(iterator, MESSAGE_KEY_DateFmt);
  if(date_fmt_val) {
    char *my_string_value = date_fmt_val->value->cstring;
    APP_LOG(APP_LOG_LEVEL_INFO, "DateFmt Config value: %s", my_string_value);
    APP_LOG(APP_LOG_LEVEL_INFO, "DateFmt Config converted value: %i", atoi(my_string_value));
    alt_date_fmt = atoi(my_string_value) == 1;
    APP_LOG(APP_LOG_LEVEL_WARNING, "%s %u", "inbox_received_callback, Alt date fmt: ", alt_date_fmt );
    prv_save_settings(); //Persist new setting
    
    time_t now = time(NULL);
    struct tm *current_time = localtime(&now);
    handle_second_tick(current_time, MINUTE_UNIT);
    }

}

static void main_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_frame(window_layer);

  APP_LOG(APP_LOG_LEVEL_WARNING, "%s %u", "main_window_load, Alt date fmt: ", alt_date_fmt );
  
  //flap_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_WEBOS_PRELUDE_CONDENSED_30) );
  //status_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_WEBOS_PRELUDE_MEDIUM_12) );
  status_font = fonts_get_system_font(FONT_KEY_GOTHIC_14);

  //background
  bmptr = gbitmap_create_with_resource(RESOURCE_ID_webos_clock);
  mblyr = bitmap_layer_create(bounds);
  bitmap_layer_set_bitmap(mblyr,bmptr);
  layer_add_child(window_layer, bitmap_layer_get_layer(mblyr));
  
#if PBL_DISPLAY_HEIGHT == 228
  s_long_date_layer = text_layer_create(GRect(106, 70, 40, 16));
  int8_t status_position_y = -2;  //-6 for gothic 18
  int8_t connection_position_y = 2;
  s_bt_conn_layer = bitmap_layer_create(GRect(bounds.size.w - 14, 1, 13, 12)); //was 130
  cal_icon_text_x = 127;
  cal_icon_text_y = bounds.size.h-27;
  ampm_pos_x = 53;
  ampm_pos_y = 126;
#else
  s_long_date_layer = text_layer_create(GRect(66, 46, 40, 16));
  int8_t status_position_y = -5;
  int8_t connection_position_y = -3;
  s_bt_conn_layer = bitmap_layer_create(GRect(bounds.size.w - 14, -1, 13, 12)); //was 130
  cal_icon_text_x = 91;
  cal_icon_text_y = bounds.size.h-23;
  //  s_ampm_layer = text_layer_create(GRect(38, 92, 16, 16));
  ampm_pos_x = 38;
  ampm_pos_y = 92;
#endif

  //bluetooth status
  btconnptr = gbitmap_create_with_resource(RESOURCE_ID_BT_CONNECTED);
  bterrptr = gbitmap_create_with_resource(RESOURCE_ID_BT_ERROR);
  btonptr = gbitmap_create_with_resource(RESOURCE_ID_BT_ON);
  bitmap_layer_set_bitmap(s_bt_conn_layer,btconnptr);
  
  text_layer_set_text_color(s_long_date_layer, GColorLightGray);
  text_layer_set_background_color(s_long_date_layer, GColorClear);
  text_layer_set_font(s_long_date_layer, status_font);//fonts_get_system_font(FONT_KEY_GOTHIC_09));
  text_layer_set_text_alignment(s_long_date_layer, GTextAlignmentRight);
  
  s_date_layer = text_layer_create(GRect(0, status_position_y, bounds.size.w, 16));
  text_layer_set_text_color(s_date_layer, GColorWhite);
  text_layer_set_background_color(s_date_layer, GColorClear);
  text_layer_set_font(s_date_layer, status_font);//fonts_get_system_font(FONT_KEY_GOTHIC_09));
  text_layer_set_text_alignment(s_date_layer, GTextAlignmentCenter);

  //s_cal_icon_layer = text_layer_create(GRect(89, bounds.size.h-19, 12, 11));//For 9 pt Gothic
  s_cal_icon_layer = text_layer_create(GRect(cal_icon_text_x, cal_icon_text_y, 12, 14));
  text_layer_set_text_color(s_cal_icon_layer, GColorBlack);
  text_layer_set_background_color(s_cal_icon_layer, GColorClear);
  text_layer_set_font(s_cal_icon_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD));
  text_layer_set_text_alignment(s_cal_icon_layer, GTextAlignmentCenter);
  
  /*
  s_time_layer = text_layer_create(GRect(0, 62, bounds.size.w, 34));
  text_layer_set_text_color(s_time_layer, GColorWhite);
  text_layer_set_background_color(s_time_layer, GColorClear);
  text_layer_set_font(s_time_layer, flap_font); //FONT_KEY_GOTHIC_28_BOLD //  FONT_KEY_BITHAM_30_BLACK
  text_layer_set_text_alignment(s_time_layer, GTextAlignmentCenter);
  */
 //Need to adjust for PTS2 vs legacy screen
 #if PBL_DISPLAY_HEIGHT == 228
  uint8_t offset_x = 25;
  uint8_t offset_y = 22;
  s_time_1_layer = bitmap_layer_create(GRect(54, 65 + offset_y, 24, 48));
  s_time_2_layer = bitmap_layer_create(GRect(76, 65 + offset_y, 24, 48));
  s_time_3_layer = bitmap_layer_create(GRect(102, 65 + offset_y, 24, 48));
  s_time_4_layer = bitmap_layer_create(GRect(124, 65 + offset_y, 24, 48));
#elif PBL_DISPLAY_HEIGHT == 180
  uint8_t offset_x = 0;
  uint8_t offset_y = 16;
  s_time_1_layer = bitmap_layer_create(GRect(38 + offset_x, 65 + offset_y, 17, 34));
  s_time_2_layer = bitmap_layer_create(GRect(53 + offset_x, 65 + offset_y, 17, 34));
  s_time_3_layer = bitmap_layer_create(GRect(73 + offset_x, 65 + offset_y, 17, 34));
  s_time_4_layer = bitmap_layer_create(GRect(90 + offset_x, 65 + offset_y, 17, 34));
#else
  uint8_t offset_x = 0;
  uint8_t offset_y = 0;
  s_time_1_layer = bitmap_layer_create(GRect(38, 65 + offset_y, 17, 34));
  s_time_2_layer = bitmap_layer_create(GRect(53, 65 + offset_y, 17, 34));
  s_time_3_layer = bitmap_layer_create(GRect(73, 65 + offset_y, 17, 34));
  s_time_4_layer = bitmap_layer_create(GRect(90, 65 + offset_y, 17, 34));
#endif
  digit_0_ptr = gbitmap_create_with_resource(RESOURCE_ID_digit_0 );
  digit_1_ptr = gbitmap_create_with_resource(RESOURCE_ID_digit_1 );
  digit_2_ptr = gbitmap_create_with_resource(RESOURCE_ID_digit_2 );
  digit_3_ptr = gbitmap_create_with_resource(RESOURCE_ID_digit_3 );
  digit_4_ptr = gbitmap_create_with_resource(RESOURCE_ID_digit_4 );
  digit_5_ptr = gbitmap_create_with_resource(RESOURCE_ID_digit_5 );
  digit_6_ptr = gbitmap_create_with_resource(RESOURCE_ID_digit_6 );
  digit_7_ptr = gbitmap_create_with_resource(RESOURCE_ID_digit_7 );
  digit_8_ptr = gbitmap_create_with_resource(RESOURCE_ID_digit_8 );
  digit_9_ptr = gbitmap_create_with_resource(RESOURCE_ID_digit_9 );

  bitmap_layer_set_compositing_mode(s_time_1_layer, GCompOpSet );
  bitmap_layer_set_compositing_mode(s_time_2_layer, GCompOpSet );
  bitmap_layer_set_compositing_mode(s_time_3_layer, GCompOpSet );
  bitmap_layer_set_compositing_mode(s_time_4_layer, GCompOpSet );
  bitmap_layer_set_bitmap(s_time_1_layer,digit_0_ptr);
  bitmap_layer_set_bitmap(s_time_2_layer,digit_0_ptr);
  bitmap_layer_set_bitmap(s_time_3_layer,digit_0_ptr);
  bitmap_layer_set_bitmap(s_time_4_layer,digit_0_ptr);
  
  if (true || !clock_is_24h_style()) {
  s_ampm_layer = text_layer_create(GRect(ampm_pos_x, ampm_pos_y, 16, 16));
  text_layer_set_text_color(s_ampm_layer, GColorWhite);
  text_layer_set_background_color(s_ampm_layer, GColorClear);
  text_layer_set_font(s_ampm_layer, fonts_get_system_font(FONT_KEY_GOTHIC_09));
  text_layer_set_text_alignment(s_ampm_layer, GTextAlignmentLeft);
  }
  
  s_connection_layer = text_layer_create(GRect(130, connection_position_y, 10, 16));
  text_layer_set_text_color(s_connection_layer, GColorWhite);
  text_layer_set_background_color(s_connection_layer, GColorBlack);
  text_layer_set_font(s_connection_layer, status_font); //fonts_get_system_font(FONT_KEY_GOTHIC_14));
  text_layer_set_text_alignment(s_connection_layer, GTextAlignmentLeft);
  handle_bluetooth(connection_service_peek_pebble_app_connection());

  s_battery_layer = text_layer_create(GRect(0, status_position_y, bounds.size.w -14, 16));
  text_layer_set_text_color(s_battery_layer, GColorWhite);
  text_layer_set_background_color(s_battery_layer, GColorBlack);
  text_layer_set_font(s_battery_layer, status_font); //fonts_get_system_font(FONT_KEY_GOTHIC_14));
  text_layer_set_text_alignment(s_battery_layer, GTextAlignmentRight);
  text_layer_set_text(s_battery_layer, "--");

  s_status_layer = text_layer_create(GRect(5, status_position_y, 75, 16));
  text_layer_set_text_color(s_status_layer, GColorWhite);
  text_layer_set_background_color(s_status_layer, GColorBlack);
  text_layer_set_font(s_status_layer, status_font); //fonts_get_system_font(FONT_KEY_GOTHIC_14));
  text_layer_set_text_alignment(s_status_layer, GTextAlignmentLeft);
  text_layer_set_text(s_status_layer, "webOS");

  // Ensures time is displayed immediately (will break if NULL tick event accessed).
  // (This is why it's a good idea to have a separate routine to do the update itself.)
  time_t now = time(NULL);
  struct tm *current_time = localtime(&now);
  handle_second_tick(current_time, MINUTE_UNIT);

  tick_timer_service_subscribe(MINUTE_UNIT, handle_second_tick);
  battery_state_service_subscribe(handle_battery);

  connection_service_subscribe((ConnectionHandlers) {
    .pebble_app_connection_handler = handle_bluetooth
  });

  //Replaced with images so I don't have to distribute font
  //layer_add_child(window_layer, text_layer_get_layer(s_time_layer));
  if (true || !clock_is_24h_style()) {
  layer_add_child(window_layer, text_layer_get_layer(s_ampm_layer));
  }
  layer_add_child(window_layer, text_layer_get_layer(s_battery_layer));
  layer_add_child(window_layer, text_layer_get_layer(s_status_layer));
  //layer_add_child(window_layer, text_layer_get_layer(s_connection_layer));
  layer_add_child(window_layer, text_layer_get_layer(s_date_layer));
  layer_add_child(window_layer, text_layer_get_layer(s_cal_icon_layer));
  layer_add_child(window_layer, bitmap_layer_get_layer(s_bt_conn_layer));

  layer_add_child(window_layer, bitmap_layer_get_layer(s_time_1_layer));
  layer_add_child(window_layer, bitmap_layer_get_layer(s_time_2_layer));
  layer_add_child(window_layer, bitmap_layer_get_layer(s_time_3_layer));
  layer_add_child(window_layer, bitmap_layer_get_layer(s_time_4_layer));

  layer_add_child(window_layer, text_layer_get_layer(s_long_date_layer));

  handle_battery(battery_state_service_peek());
}

static void main_window_unload(Window *window) {
  tick_timer_service_unsubscribe();
  battery_state_service_unsubscribe();
  connection_service_unsubscribe();
  text_layer_destroy(s_long_date_layer);
  text_layer_destroy(s_date_layer);
  text_layer_destroy(s_cal_icon_layer);
  //text_layer_destroy(s_time_layer);
  if (true || !clock_is_24h_style()) {
  text_layer_destroy(s_ampm_layer);
  }
  text_layer_destroy(s_status_layer);
  text_layer_destroy(s_connection_layer);
  text_layer_destroy(s_battery_layer);
  gbitmap_destroy(bmptr);
  bitmap_layer_destroy(mblyr);
  gbitmap_destroy(btconnptr);
  gbitmap_destroy(btonptr);
  gbitmap_destroy(bterrptr);
  bitmap_layer_destroy(s_bt_conn_layer);
  bitmap_layer_destroy(s_time_1_layer);
  bitmap_layer_destroy(s_time_2_layer);
  bitmap_layer_destroy(s_time_3_layer);
  bitmap_layer_destroy(s_time_4_layer);
  gbitmap_destroy(digit_0_ptr);
  gbitmap_destroy(digit_1_ptr);
  gbitmap_destroy(digit_2_ptr);
  gbitmap_destroy(digit_3_ptr);
  gbitmap_destroy(digit_4_ptr);
  gbitmap_destroy(digit_5_ptr);
  gbitmap_destroy(digit_6_ptr);
  gbitmap_destroy(digit_7_ptr);
  gbitmap_destroy(digit_8_ptr);
  gbitmap_destroy(digit_9_ptr);
}

// Save the settings to persistent storage
static void prv_save_settings() {
  //persist_write_data(SETTINGS_KEY, &settings, sizeof(settings));
  persist_write_bool(SETTINGS_DATEFMT_KEY, alt_date_fmt);
  APP_LOG(APP_LOG_LEVEL_WARNING, "%s %u", "Saved settings, Alt date fmt: ", alt_date_fmt );
}

// Load the settings from persistent storage
static void prv_load_settings() {
  APP_LOG(APP_LOG_LEVEL_WARNING, "%s", "Loading settings..");
  alt_date_fmt = persist_read_bool(SETTINGS_DATEFMT_KEY);
  APP_LOG(APP_LOG_LEVEL_WARNING, "%s %u", "Loaded settings, Alt date fmt: ", alt_date_fmt );
}

static void init() {
  prv_load_settings();
  s_main_window = window_create();
  window_set_background_color(s_main_window, GColorBlack);
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload,
  });
  
  // Largest expected inbox and outbox message sizes
  const uint32_t inbox_size = 30;
  const uint32_t outbox_size = 0;

  // Open AppMessage
  app_message_open(inbox_size, outbox_size);
  // Prep AppMessage receive
  app_message_register_inbox_received(inbox_received_callback);
  
  window_stack_push(s_main_window, true);
}

static void deinit() {
  window_destroy(s_main_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
