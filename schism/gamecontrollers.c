#include "sdlmain.h"
#include "log.h"
#include <math.h>


#define MAX_CONTROLLERS 4

SDL_GameController *game_controllers[MAX_CONTROLLERS];

static uint8_t keycode = 0; // value of the pressed key
static int num_joysticks = 0;

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
  log_appendf(2,"Trying to open game controller database from %s", db_filename);
  SDL_RWops* db_rw = SDL_RWFromFile(db_filename, "rb");
  if (db_rw == NULL) {
    snprintf(db_filename, sizeof(db_filename), "%sgamecontrollerdb.txt",
    SDL_GetBasePath());
    log_appendf(2,"Trying to open game controller database from %s", db_filename);
    db_rw = SDL_RWFromFile(db_filename, "rb");
  }

  if (db_rw != NULL) {
    int mappings = SDL_GameControllerAddMappingsFromRW(db_rw, 1);
    if (mappings != -1)
      log_appendf(2,"Found %d game controller mappings", mappings);
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
    log_appendf(2,"Controller %d: %s", controller_index + 1,
            SDL_GameControllerName(game_controllers[controller_index]));
    controller_index++;
  }

  return controller_index;
}

// Closes all open game controllers
void close_game_controllers() {

  for (int i = 0; i < MAX_CONTROLLERS; i++) {
    if (game_controllers[i])
      SDL_GameControllerClose(game_controllers[i]);
  }
}