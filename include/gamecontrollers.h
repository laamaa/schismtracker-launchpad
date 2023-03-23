#ifndef GAMECONTROLLERS_H_
#define GAMECONTROLLERS_H_

#include <stdint.h>

typedef enum gamepad_pages_t {
  PG_LOAD_MODULE,
  PG_PATTERN_EDITOR,
  PG_SAMPLE_LIST,
  PG_INFO,  
  PG_ORDER_LIST,
  PG_SONG_VARIABLES,
  PG_WATERFALL,
  PG_MAX
} gamepad_pages_t;

typedef struct gamecontroller_config_params_s {

  int gamepad_up;
  int gamepad_left;
  int gamepad_down;
  int gamepad_right;
  int gamepad_select;
  int gamepad_start;
  int gamepad_opt;
  int gamepad_edit;
  int gamepad_quit;
  int gamepad_reset;

  int gamepad_analog_threshold;
  int gamepad_analog_invert;
  int gamepad_analog_axis_updown;
  int gamepad_analog_axis_leftright;
  int gamepad_analog_axis_start;
  int gamepad_analog_axis_select;
  int gamepad_analog_axis_opt;
  int gamepad_analog_axis_edit;

} gamecontroller_config_params_s;

// Bits for input messages
enum keycodes {
  key_left = 1 << 7,
  key_up = 1 << 6,
  key_down = 1 << 5,
  key_select = 1 << 4,
  key_start = 1 << 3,
  key_right = 1 << 2,
  key_opt = 1 << 1,
  key_edit = 1
};

typedef enum input_buttons_t {
    INPUT_UP,
    INPUT_DOWN,
    INPUT_LEFT,
    INPUT_RIGHT,
    INPUT_OPT,
    INPUT_EDIT,
    INPUT_SELECT,
    INPUT_START,
    INPUT_MAX
} input_buttons_t;

typedef enum input_type_t {
  normal,
  keyjazz,
  special
} input_type_t;

typedef enum special_messages_t {
  msg_quit = 1,
  msg_reset_display = 2
} special_messages_t;

typedef struct input_msg_s {
  input_type_t type;
  uint8_t value;
  uint8_t value2;
  uint32_t eventType;
} input_msg_s;

int initialize_game_controllers();
void close_game_controllers();
void update_game_controllers();
input_msg_s get_input_msg();

#endif