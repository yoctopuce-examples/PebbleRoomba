#include <pebble.h>


// PACKET DEFINITIONS  
  
#define KEY_CMD       0
#define CMD_CLEAN     0
#define CMD_SPOT      1
#define CMD_DOCK      2
#define CMD_GET_STATE 3
  
#define KEY_MESSAGE 1  
  
#define KEY_STATE       2  
#define STATE_CLEANING 0
#define STATE_SPOTING 1
#define STATE_DOCKING 2
#define STATE_OFF 3
#define STATE_OFFLINE 4
#define STATE_DOCKED 5

  
// UI size defintions
  
#define ROOMBA_SIZE 100
#define CROSS_SIZE 100
#define CLEAN_ICON_SIZE 15
#define SPOT_ICON_SIZE 15
#define HOME_ICON_SIZE 15
  
  
// static defintisons
static Window *window;
static TextLayer *s_text_layer_message;
static ActionBarLayer *s_action_bar;
static BitmapLayer *s_roomba_layer;
static BitmapLayer *s_cross_layer;

static GBitmap *s_roomba_bitmap;
static GBitmap *s_cross_bitmap;
static GBitmap *s_clean_bitmap;
static GBitmap *s_spot_bitmap;
static GBitmap *s_dock_bitmap;

static char message_buffer[32];
static int s_uptime = 0;

static void refreshMessage(const char* msg)
{
  snprintf(message_buffer, sizeof(message_buffer), "%s", msg);
  text_layer_set_text(s_text_layer_message, message_buffer);
}
  



static void refreshState(int state)
{
  switch(state) {
  case STATE_CLEANING :
  case STATE_SPOTING :
  case STATE_DOCKING :
  case STATE_OFF :
  case STATE_DOCKED :
    layer_set_hidden((Layer *)s_cross_layer,true);
    break;
  case STATE_OFFLINE :  
    layer_set_hidden((Layer *)s_cross_layer,false);
    break;
  }
}
  


static void inbox_received_callback(DictionaryIterator *iterator, void *context) {
  // Read first item
  Tuple *t = dict_read_first(iterator);

  // For all items
  while(t != NULL) {
    // Which key was received?
    switch(t->key) {
    case KEY_MESSAGE:
         refreshMessage(t->value->cstring);
      break;
    case KEY_STATE:
      refreshState(t->value->uint16);
      break;
    default:
      APP_LOG(APP_LOG_LEVEL_ERROR, "Key %d not recognized!", (int)t->key);
      break;
    }

    // Look for next item
    t = dict_read_next(iterator);
  }
}

static void send(int key, int message) {
  DictionaryIterator *iter;
  app_message_outbox_begin(&iter);
  dict_write_int(iter, key, &message, sizeof(int), true);
  app_message_outbox_send();
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

static void up_click_handler(ClickRecognizerRef recognizer, void *context) {

  send(KEY_CMD, CMD_CLEAN);
}

static void select_click_handler(ClickRecognizerRef recognizer, void *context) {
  send(KEY_CMD, CMD_SPOT);
}

static void down_click_handler(ClickRecognizerRef recognizer, void *context) {
  send(KEY_CMD, CMD_DOCK);
}

static void click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler);
  window_single_click_subscribe(BUTTON_ID_UP, up_click_handler);
  window_single_click_subscribe(BUTTON_ID_DOWN, down_click_handler);
}


static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
   // Increment s_uptime
  s_uptime++;
  if ( (s_uptime & 0x3) == 0){
    send(KEY_CMD, CMD_GET_STATE);
  }
}

static void window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  
  
  GRect bounds = layer_get_bounds(window_layer);
  // Create roomba GBitmap
  s_roomba_bitmap = gbitmap_create_with_resource(RESOURCE_ID_ROOMBA_FULL);
  // create roomba layer
  s_roomba_layer = bitmap_layer_create(GRect((bounds.size.w - ACTION_BAR_WIDTH - ROOMBA_SIZE)/2, (bounds.size.h - ROOMBA_SIZE - 5), ROOMBA_SIZE,ROOMBA_SIZE));
  bitmap_layer_set_bitmap(s_roomba_layer, s_roomba_bitmap);  
#ifdef PBL_SDK_3
  bitmap_layer_set_compositing_mode(s_roomba_layer,GCompOpSet);
#endif
  layer_add_child(window_get_root_layer(window), bitmap_layer_get_layer(s_roomba_layer));

 // Create cross GBitmap
  s_cross_bitmap = gbitmap_create_with_resource(RESOURCE_ID_CROSS_FULL);
  // create cross layer
  s_cross_layer = bitmap_layer_create(GRect((bounds.size.w - ACTION_BAR_WIDTH - CROSS_SIZE)/2, (bounds.size.h - CROSS_SIZE - 5), CROSS_SIZE,CROSS_SIZE));
  bitmap_layer_set_bitmap(s_cross_layer, s_cross_bitmap);  
#ifdef PBL_SDK_3
  bitmap_layer_set_compositing_mode(s_cross_layer,GCompOpSet);
#endif
  layer_add_child(window_get_root_layer(window), bitmap_layer_get_layer(s_cross_layer));
  
  // create message layer
  s_text_layer_message = text_layer_create((GRect) { .origin = { 0, 10 }, .size = { bounds.size.w -  ACTION_BAR_WIDTH , 30 } });
  text_layer_set_background_color(s_text_layer_message,GColorClear);
  text_layer_set_font(s_text_layer_message, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
  text_layer_set_text_alignment(s_text_layer_message, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(s_text_layer_message));
  
   // Initialize the action bar:
  s_action_bar = action_bar_layer_create();
  action_bar_layer_add_to_window(s_action_bar, window);
  action_bar_layer_set_click_config_provider(s_action_bar, click_config_provider);
  s_clean_bitmap = gbitmap_create_with_resource(RESOURCE_ID_CLEAN_ICON);
  s_spot_bitmap = gbitmap_create_with_resource(RESOURCE_ID_SPOT_ICON);
  s_dock_bitmap = gbitmap_create_with_resource(RESOURCE_ID_DOCK_ICON);
  action_bar_layer_set_icon(s_action_bar, BUTTON_ID_UP, s_clean_bitmap);
  action_bar_layer_set_icon(s_action_bar, BUTTON_ID_SELECT, s_spot_bitmap);
  action_bar_layer_set_icon(s_action_bar, BUTTON_ID_DOWN, s_dock_bitmap);


  refreshMessage("Connecting...");
  send(KEY_CMD, CMD_GET_STATE);
}

static void window_unload(Window *window) {
  action_bar_layer_destroy(s_action_bar);
  text_layer_destroy(s_text_layer_message);
  bitmap_layer_destroy(s_cross_layer);
  gbitmap_destroy(s_cross_bitmap);
  bitmap_layer_destroy(s_roomba_layer);
  gbitmap_destroy(s_roomba_bitmap);
  
}

static void init(void) {
  window = window_create();
#ifndef PBL_SDK_3
    window_set_fullscreen(window, true);
#endif
#ifdef PBL_COLOR
  window_set_background_color(window, GColorLimerick);
#else
  window_set_background_color(window, GColorWhite);
#endif

  window_set_click_config_provider(window, click_config_provider);

  window_set_window_handlers(window, (WindowHandlers) {
  .load = window_load,
    .unload = window_unload,
  });
  
  // App message callbacks
  app_message_register_inbox_received(inbox_received_callback);
  app_message_register_inbox_dropped(inbox_dropped_callback);
  app_message_register_outbox_failed(outbox_failed_callback);
  app_message_register_outbox_sent(outbox_sent_callback);
  // Open AppMessage
  app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());

  
  const bool animated = true;
  window_stack_push(window, animated);
  
  // Subscribe to TickTimerService
  tick_timer_service_subscribe(SECOND_UNIT, tick_handler);
}

static void deinit(void) {
  window_destroy(window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
