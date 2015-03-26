#include <pebble.h>
  
static Window *s_main_window;
static TextLayer *s_time_layer;
static TextLayer *s_reg_time_layer;
static GFont s_time_font;

static char *int2bin(unsigned n, char *buf)
{
    #define BITS 4

    static char static_buf[BITS + 1];
    int i;

    if (buf == NULL)
        buf = static_buf;

    for (i = BITS - 1; i >= 0; --i) {
        buf[i] = (n & 1) ? '1' : '0';
        n >>= 1;
    }

    buf[BITS] = '\0';
    return buf;

    #undef BITS
}

void update_time() {
  // Get a tm structure
  time_t temp = time(NULL); 
  struct tm *tick_time = localtime(&temp);

  // Create a long-lived buffer
  static char buffer[] = "0000\n0000\n0000\n0000";
  char ten_hour_buffer[] = "0000";
  char ten_min_buffer[] = "0000";
  char hour_buffer[] = "0000";
  char min_buffer[] = "0000";
  
  int ten_hour = (tick_time->tm_hour - (tick_time->tm_hour % 10))/10;
  int hour = tick_time->tm_hour % 10;
  int ten_min =  (tick_time->tm_min - (tick_time->tm_min % 10))/10;
  int minutes = tick_time->tm_min % 10;

  // Write the current hours and minutes into the buffer
  if(clock_is_24h_style() == true) {
    // Use 24 hour format
    int2bin(ten_hour, ten_hour_buffer);
    int2bin(hour, hour_buffer);
  } else {
    // Use 12 hour format
    if (tick_time->tm_hour > 12) {
      int hours = tick_time->tm_hour - 12;
      ten_hour = (hours - (hours % 10))/10;
      hour = hours % 10;
    }
    int2bin(ten_hour, ten_hour_buffer);
    int2bin(hour, hour_buffer);
  }
    int2bin(ten_min, ten_min_buffer);
    int2bin(minutes, min_buffer);
    snprintf(buffer, sizeof(buffer), "%s\n%s\n%s\n%s", ten_hour_buffer, hour_buffer, ten_min_buffer, min_buffer);
    APP_LOG(APP_LOG_LEVEL_DEBUG, buffer);
  // Display this time on the TextLayer
  text_layer_set_text(s_time_layer, buffer);
}

void animation_stopped(Animation *animation, bool finished, void *data) {
    //Free the memory used by the Animation
    property_animation_destroy((PropertyAnimation*) animation);
}

void animate_layer(Layer *layer, GRect *start, GRect *finish, int duration, int delay) {
    //Declare animation
    PropertyAnimation *anim = property_animation_create_layer_frame(layer, start, finish);
 
    //Set characteristics
    animation_set_duration((Animation*) anim, duration);
    animation_set_delay((Animation*) anim, delay);
 
    //Set stopped handler to free memory
    AnimationHandlers handlers = {
        //The reference to the stopped handler is the only one in the array
        .stopped = (AnimationStoppedHandler) animation_stopped
    };
    animation_set_handlers((Animation*) anim, handlers, NULL);
 
    //Start animation!
    animation_schedule((Animation*) anim);
}

void updateTimeRegular() {
  // Get a tm structure
  time_t temp = time(NULL); 
  struct tm *tick_time = localtime(&temp);

  // Create a long-lived buffer
  static char buffer[] = "00:00";

  // Write the current hours and minutes into the buffer
  if(clock_is_24h_style() == true) {
    //Use 2h hour format
    strftime(buffer, sizeof("00:00"), "%H:%M", tick_time);
  } else {
    //Use 12 hour format
    strftime(buffer, sizeof("00:00"), "%I:%M", tick_time);
  }

  // Display this time on the TextLayer
  text_layer_set_text(s_reg_time_layer, buffer);

  // Set start and end
  GRect from_frame = GRect(0, -60, 144, 60);
  GRect to_frame = GRect(0, 228, 144, 30);

  animate_layer(text_layer_get_layer(s_reg_time_layer), &from_frame, &to_frame, 3000, 500);
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  update_time();
}

static void tap_handler(AccelAxisType axis, int32_t direction) {
  updateTimeRegular();
}

static void main_window_load(Window *window) {
  // Create time TextLayer
  s_time_layer = text_layer_create(GRect(0, 0, 144, 168));
  text_layer_set_background_color(s_time_layer, GColorBlack);
  text_layer_set_text_color(s_time_layer, GColorWhite);
  text_layer_set_text(s_time_layer, "0000\n0000\n0000\n0000");

  s_reg_time_layer = text_layer_create(GRect(0, -60, 144, 60));
  text_layer_set_background_color(s_reg_time_layer, GColorWhite);
  text_layer_set_text_color(s_reg_time_layer, GColorBlack);
  text_layer_set_text(s_reg_time_layer, "00:00");

  // Improve the layout to be more like a watchface
  // Create GFont
  s_time_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_COURIER_NEW_BOLD_40));

  // Apply to TextLayer
  text_layer_set_font(s_time_layer, s_time_font);  
  text_layer_set_font(s_reg_time_layer, s_time_font);  
  text_layer_set_text_alignment(s_time_layer, GTextAlignmentCenter);
  text_layer_set_overflow_mode(s_time_layer, GTextOverflowModeWordWrap);
  text_layer_set_text_alignment(s_reg_time_layer, GTextAlignmentCenter);
  text_layer_set_overflow_mode(s_reg_time_layer, GTextOverflowModeWordWrap);

  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_time_layer));
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_reg_time_layer));

}

static void main_window_unload(Window *window) {

  // Unload GFont
  fonts_unload_custom_font(s_time_font);  
  // Destroy TextLayer
  text_layer_destroy(s_time_layer);
  text_layer_destroy(s_reg_time_layer);

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
  accel_tap_service_subscribe(tap_handler);
}

static void deinit() {
  tick_timer_service_unsubscribe();
  accel_tap_service_unsubscribe();
  // Destroy Window
  window_destroy(s_main_window);
  
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
