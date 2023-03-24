#include "gamecontrollers.h"
#include "SDL_events.h"
#include "SDL_keycode.h"
#include "event.h"
#include "time.h"
#include "it.h"
#include "log.h"
#include "page.h"
#include "sdlmain.h"
#include "song.h"
#include <math.h>

#define MAX_CONTROLLERS 4

SDL_GameController *game_controllers[MAX_CONTROLLERS];

static uint8_t keycode = 0; // value of the pressed key
static int num_joysticks = 0;
static input_msg_s key = {normal, 0};
static gamecontroller_config_params_s controller_config;

static gamecontroller_config_params_s init_game_controller_config() {

  gamecontroller_config_params_s gc_default_config;

  gc_default_config.gamepad_up = SDL_CONTROLLER_BUTTON_DPAD_UP;
  gc_default_config.gamepad_left = SDL_CONTROLLER_BUTTON_DPAD_LEFT;
  gc_default_config.gamepad_down = SDL_CONTROLLER_BUTTON_DPAD_DOWN;
  gc_default_config.gamepad_right = SDL_CONTROLLER_BUTTON_DPAD_RIGHT;
  gc_default_config.gamepad_select = SDL_CONTROLLER_BUTTON_BACK;
  gc_default_config.gamepad_start = SDL_CONTROLLER_BUTTON_START;
  gc_default_config.gamepad_opt = SDL_CONTROLLER_BUTTON_B;
  gc_default_config.gamepad_edit = SDL_CONTROLLER_BUTTON_A;
  gc_default_config.gamepad_quit = SDL_CONTROLLER_BUTTON_RIGHTSTICK;
  gc_default_config.gamepad_reset = SDL_CONTROLLER_BUTTON_LEFTSTICK;
  gc_default_config.gamepad_analog_threshold = 32766;
  gc_default_config.gamepad_analog_invert = 0;
  gc_default_config.gamepad_analog_axis_updown = SDL_CONTROLLER_AXIS_LEFTY;
  gc_default_config.gamepad_analog_axis_leftright = SDL_CONTROLLER_AXIS_LEFTX;
  gc_default_config.gamepad_analog_axis_start =
      SDL_CONTROLLER_AXIS_TRIGGERRIGHT;
  gc_default_config.gamepad_analog_axis_select =
      SDL_CONTROLLER_AXIS_TRIGGERLEFT;
  gc_default_config.gamepad_analog_axis_opt = SDL_CONTROLLER_AXIS_INVALID;
  gc_default_config.gamepad_analog_axis_edit = SDL_CONTROLLER_AXIS_INVALID;

  return gc_default_config;
}

static void _push_keyboard_event(SDL_Keycode keycode) {
  SDL_Event sdlevent = {};
  sdlevent.type = SDL_KEYDOWN;
  sdlevent.key.keysym.sym = keycode;
  SDL_PushEvent(&sdlevent);
  sdlevent.type = SDL_KEYUP;
  sdlevent.key.keysym.sym = keycode;
  SDL_PushEvent(&sdlevent);
}

static void _push_keyboard_event_mod(SDL_Keycode keycode, SDL_Keymod mod) {
  SDL_Event sdlevent = {};
  sdlevent.type = SDL_KEYDOWN;
  sdlevent.key.keysym.sym = keycode;
  sdlevent.key.keysym.mod = mod;
  SDL_PushEvent(&sdlevent);
  sdlevent.type = SDL_KEYUP;
  sdlevent.key.keysym.sym = keycode;
  sdlevent.key.keysym.mod = mod;
  SDL_PushEvent(&sdlevent);
}

// Check whether a button is pressed on a gamepad and return 1 if pressed.
static int get_game_controller_button(gamecontroller_config_params_s *conf,
                                      SDL_GameController *controller,
                                      int button) {

  const int button_mappings[8] = {conf->gamepad_up,     conf->gamepad_down,
                                  conf->gamepad_left,   conf->gamepad_right,
                                  conf->gamepad_opt,    conf->gamepad_edit,
                                  conf->gamepad_select, conf->gamepad_start};

  // Check digital buttons
  if (SDL_GameControllerGetButton(controller, button_mappings[button])) {
    return 1;
  } else {
    // If digital button isn't pressed, check the corresponding analog control
    switch (button) {
    case INPUT_UP:
      return SDL_GameControllerGetAxis(controller,
                                       conf->gamepad_analog_axis_updown) <
             -conf->gamepad_analog_threshold;
    case INPUT_DOWN:
      return SDL_GameControllerGetAxis(controller,
                                       conf->gamepad_analog_axis_updown) >
             conf->gamepad_analog_threshold;
    case INPUT_LEFT:
      return SDL_GameControllerGetAxis(controller,
                                       conf->gamepad_analog_axis_leftright) <
             -conf->gamepad_analog_threshold;
    case INPUT_RIGHT:
      return SDL_GameControllerGetAxis(controller,
                                       conf->gamepad_analog_axis_leftright) >
             conf->gamepad_analog_threshold;
    case INPUT_OPT:
      return SDL_GameControllerGetAxis(controller,
                                       conf->gamepad_analog_axis_opt) >
             conf->gamepad_analog_threshold;
    case INPUT_EDIT:
      return SDL_GameControllerGetAxis(controller,
                                       conf->gamepad_analog_axis_edit) >
             conf->gamepad_analog_threshold;
    case INPUT_SELECT:
      return SDL_GameControllerGetAxis(controller,
                                       conf->gamepad_analog_axis_select) >
             conf->gamepad_analog_threshold;
    case INPUT_START:
      return SDL_GameControllerGetAxis(controller,
                                       conf->gamepad_analog_axis_start) >
             conf->gamepad_analog_threshold;
    default:
      return 0;
    }
  }
  return 0;
}

// Handle game controllers, simply check all buttons and analog axis on every
// cycle
static int
handle_game_controller_buttons(gamecontroller_config_params_s *conf) {

  const int keycodes[8] = {key_up,  key_down, key_left,   key_right,
                           key_opt, key_edit, key_select, key_start};

  int key = 0;

  // Cycle through every active game controller
  for (int gc = 0; gc < num_joysticks; gc++) {
    // Cycle through all M8 buttons
    for (int button = 0; button < (input_buttons_t)INPUT_MAX; button++) {
      // If the button is active, add the keycode to the variable containing
      // active keys
      if (get_game_controller_button(conf, game_controllers[gc], button)) {
        key |= keycodes[button];
      }
    }
  }

  return key;
}

// Opens available game controllers and returns the amount of opened controllers
int initialize_game_controllers() {

  num_joysticks = SDL_NumJoysticks();
  int controller_index = 0;

  log_nl();
  log_append(2, 0, "Looking for game controllers");
  SDL_Delay(
      10); // Some controllers like XBone wired need a little while to get ready

  // Try to load the game controller database file
  char db_filename[1024] = {0};
  snprintf(db_filename, sizeof(db_filename), "%sgamecontrollerdb.txt",
           SDL_GetPrefPath("", "schism"));
  log_appendf(2, "Trying to open game controller database from %s",
              db_filename);
  SDL_RWops *db_rw = SDL_RWFromFile(db_filename, "rb");
  if (db_rw == NULL) {
    snprintf(db_filename, sizeof(db_filename), "%sgamecontrollerdb.txt",
             SDL_GetBasePath());
    log_appendf(2, "Trying to open game controller database from %s",
                db_filename);
    db_rw = SDL_RWFromFile(db_filename, "rb");
  }

  if (db_rw != NULL) {
    int mappings = SDL_GameControllerAddMappingsFromRW(db_rw, 1);
    if (mappings != -1)
      log_appendf(2, "Found %d game controller mappings", mappings);
    else
      SDL_LogError(SDL_LOG_CATEGORY_INPUT,
                   "Error loading game controller mappings.");
  } else {
    SDL_LogError(SDL_LOG_CATEGORY_INPUT,
                 "Unable to open game controller database file.");
  }

  // Open all available game controllers
  for (int i = 0; i < num_joysticks; i++) {
    if (!SDL_IsGameController(i))
      continue;
    if (controller_index >= MAX_CONTROLLERS)
      break;
    game_controllers[controller_index] = SDL_GameControllerOpen(i);
    SDL_Log("Controller %d: %s", controller_index + 1,
            SDL_GameControllerName(game_controllers[controller_index]));
    controller_index++;
  }

  controller_config = init_game_controller_config();

  return controller_index;
}

// Closes all open game controllers
void close_game_controllers() {

  for (int i = 0; i < MAX_CONTROLLERS; i++) {
    if (game_controllers[i])
      SDL_GameControllerClose(game_controllers[i]);
  }
}

static void switch_page(gamepad_pages_t page) {

  SDL_Event e;
  e.type = SCHISM_EVENT_NATIVE;
  e.user.code = SCHISM_EVENT_NATIVE_SCRIPT;

  switch (page) {
  case PG_PATTERN_EDITOR:
    e.user.data1 = "pattern";
    SDL_PushEvent(&e);
    break;
  case PG_SAMPLE_LIST:
    e.user.data1 = "sample_page";
    SDL_PushEvent(&e);
    break;
  case PG_WATERFALL:
    e.user.data1 = "waterfall";
    SDL_PushEvent(&e);
    break;
  case PG_LOAD_MODULE:
    e.user.data1 = "load";
    SDL_PushEvent(&e);
    break;
  case PG_ORDER_LIST:
    e.user.data1 = "orders";
    SDL_PushEvent(&e);
    break;
  case PG_SONG_VARIABLES:
    e.user.data1 = "variables";
    SDL_PushEvent(&e);
    break;
  case PG_INFO:
    e.user.data1 = "info";
    SDL_PushEvent(&e);
  default:
    break;
  }
}

void update_game_controllers() {

  static int prev_key = 0;
  static gamepad_pages_t page = PG_LOAD_MODULE;

  // Read joysticks
  int key = handle_game_controller_buttons(&controller_config);

  if (key != prev_key) {
    switch (key) {
    case key_select | key_up: {
      if (status.current_page == PAGE_ORDERLIST_VOLUMES ||
          status.current_page == PAGE_ORDERLIST_PANNING) {
        if (page < PAGE_MAX) {
          page++;
          switch_page(page);
        }
      }
    } break;
    case key_select | key_right: {
      if (status.current_page == PAGE_ORDERLIST_VOLUMES ||
          status.current_page == PAGE_ORDERLIST_PANNING) {
        _push_keyboard_event(SDLK_g);
      } else {
        if (page < PAGE_MAX) {
          page++;
          switch_page(page);
        }
      }
    } break;
    case key_select | key_left: {
      if (page > 0) {
        page--;
        switch_page(page);
      }
    } break;
    case key_start: {
      if (song_get_mode() == MODE_PLAYING) {
        song_stop();
      } else {
        if (status.current_page == PAGE_ORDERLIST_VOLUMES ||
            status.current_page == PAGE_ORDERLIST_PANNING) {
          // On order list view, start from the pattern where the cursor is
          _push_keyboard_event(SDLK_F7);
        } else {
          song_start_at_order(song_get_current_order(), 0);
        }
      }
    } break;
    case key_start | key_left: {
      // On order list view: If the song is playing, queue the next order or if
      // song is stopped, start from selected position
      if (status.current_page == PAGE_ORDERLIST_VOLUMES ||
          status.current_page == PAGE_ORDERLIST_PANNING) {
        if (song_get_mode() == MODE_PLAYING) {
          // If song is playing, queue the selected order (pressing CTRL+F7)
          _push_keyboard_event_mod(SDLK_F7, KMOD_CTRL);
        } else {
          _push_keyboard_event(SDLK_F7);
        }
      }
    } break;
    case key_edit:
      _push_keyboard_event(SDLK_RETURN);
      break;
    case key_left:
      _push_keyboard_event(SDLK_LEFT);
      break;
    case key_right:
      _push_keyboard_event(SDLK_RIGHT);
      break;
    case key_up:
      _push_keyboard_event(SDLK_UP);
      break;
    case key_down:
      _push_keyboard_event(SDLK_DOWN);
      break;
    default:
      break;
    }
    prev_key = key;
  }
}