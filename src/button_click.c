#include <pebble.h>

#define TEST_MESSAGE_STR 1  
  
  
#define KEY_BUTTON    0
#define BUTTON_UP     0
#define BUTTON_SELECT 1
#define BUTTON_DOWN   2
  
static Window *window;
static TextLayer *text_layer;
static TextLayer *text_layer_message;
static ActionBarLayer *action_bar;

static BitmapLayer *s_background_layer;
static GBitmap *s_background_bitmap;

// The implementation of my_next_click_handler and my_previous_click_handler
// is omitted for the sake of brevity. See the Clicks reference docs.


static char conditions_buffer[32];

static void inbox_received_callback(DictionaryIterator *iterator, void *context) {
  // Read first item
  Tuple *t = dict_read_first(iterator);

  // For all items
  while(t != NULL) {
    // Which key was received?
    switch(t->key) {
    case TEST_MESSAGE_STR:
      snprintf(conditions_buffer, sizeof(conditions_buffer), "%s", t->value->cstring);
      text_layer_set_text(text_layer_message, conditions_buffer);
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


static void select_click_handler(ClickRecognizerRef recognizer, void *context) {
  text_layer_set_text(text_layer, "Select");
  send(KEY_BUTTON, BUTTON_SELECT);
}

static void up_click_handler(ClickRecognizerRef recognizer, void *context) {
  text_layer_set_text(text_layer, "Up");
  send(KEY_BUTTON, BUTTON_UP);
}

static void down_click_handler(ClickRecognizerRef recognizer, void *context) {
  text_layer_set_text(text_layer, "Down");
  send(KEY_BUTTON, BUTTON_DOWN);
}

static void click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler);
  window_single_click_subscribe(BUTTON_ID_UP, up_click_handler);
  window_single_click_subscribe(BUTTON_ID_DOWN, down_click_handler);
}

static void window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);
  // Create GBitmap, then set to created BitmapLayer
  s_background_bitmap = gbitmap_create_with_resource(RESOURCE_ID_PEEBLE_IMAGE);
  s_background_layer = bitmap_layer_create(GRect(0, 20, 114,114));
  bitmap_layer_set_bitmap(s_background_layer, s_background_bitmap);
  //bitmap_layer_set_compositing_mode(s_background_layer,GCompOpSet);
  layer_add_child(window_get_root_layer(window), bitmap_layer_get_layer(s_background_layer));

  

  text_layer = text_layer_create((GRect) { .origin = { 0, 0 }, .size = { bounds.size.w - ACTION_BAR_WIDTH, 20 } });
  text_layer_set_text(text_layer, "Press a button");
  text_layer_set_text_alignment(text_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(text_layer));

  text_layer_message = text_layer_create((GRect) { .origin = { 0, bounds.size.h - 21 }, .size = { bounds.size.w - ACTION_BAR_WIDTH, 20 } });
  text_layer_set_text(text_layer_message, "...");
  text_layer_set_text_alignment(text_layer_message, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(text_layer_message));

  // Initialize the action bar:
  action_bar = action_bar_layer_create();
  // Associate the action bar with the window:
  action_bar_layer_add_to_window(action_bar, window);
  // Set the click config provider:
  action_bar_layer_set_click_config_provider(action_bar,
                                             click_config_provider);

  // Set the icons:
  // The loading the icons is omitted for brevity... See HeapBitmap.
  //action_bar_layer_set_icon_animated(action_bar, BUTTON_ID_UP, &my_icon_previous, true);
  //action_bar_layer_set_icon_animated(action_bar, BUTTON_ID_DOWN, &my_icon_next, true);


  
  
}

static void window_unload(Window *window) {
  text_layer_destroy(text_layer);
}

static void init(void) {
  window = window_create();
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
}

static void deinit(void) {
  window_destroy(window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}