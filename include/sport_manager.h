
#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "../../sport_selector/include/sport_selector.h"

// -----------------------------------------------------------------------------
// UI state for sport selection
// -----------------------------------------------------------------------------
typedef enum {
  SPORT_UI_STATE_RUNNING = 0,  // Normal mode, big clock
  SPORT_UI_STATE_SELECT_SPORT, // Choosing which sport (basketball/football/...)
  SPORT_UI_STATE_SELECT_VARIANT // Viewing playclock variants for selected sport
} sport_ui_state_t;

// -----------------------------------------------------------------------------
// Grouped sports model
//  - NO duplicated metadata: name/seconds/behavior come from get_sport_config()
// -----------------------------------------------------------------------------
typedef struct {
  const char *logo[3];          // 3-line ASCII logo for this sport group
  const sport_type_t *variants; // list of sport_type_t (e.g. 24, 30)
  uint8_t variant_count;
} sport_group_t;

// -----------------------------------------------------------------------------
// Manager
// -----------------------------------------------------------------------------
typedef struct {
  // Currently active sport & variant
  sport_config_t current_sport;
  sport_type_t selected_sport_type;

  // Menu state
  sport_ui_state_t ui_state;
  uint8_t current_group_idx;
  uint8_t
      current_variant_idx; // index inside current group (default variant is 0)
} SportManager;

// -----------------------------------------------------------------------------
// Init
// -----------------------------------------------------------------------------
void sport_manager_init(SportManager *manager);

// -----------------------------------------------------------------------------
// Current active sport
// -----------------------------------------------------------------------------
sport_config_t sport_manager_get_current_sport(const SportManager *manager);

// The selected (highlighted) sport type in menus
sport_type_t sport_manager_get_selected_type(const SportManager *manager);

// Set active sport directly (used for compatibility / external calls)
void sport_manager_set_sport(SportManager *manager, sport_type_t sport_type);

// -----------------------------------------------------------------------------
// UI state accessors
// -----------------------------------------------------------------------------
sport_ui_state_t sport_manager_get_ui_state(const SportManager *manager);

// Index within groups/variants (for UI)
uint8_t sport_manager_get_current_group_index(const SportManager *manager);
uint8_t sport_manager_get_current_variant_index(const SportManager *manager);

// -----------------------------------------------------------------------------
// Group access (for UI drawing)
// -----------------------------------------------------------------------------
const sport_group_t *sport_manager_get_groups(size_t *count);
const sport_group_t *sport_manager_get_group(uint8_t index);
const sport_group_t *
sport_manager_get_current_group(const SportManager *manager);

// -----------------------------------------------------------------------------
// Menu navigation helpers (used by main based on InputAction)
// -----------------------------------------------------------------------------
void sport_manager_enter_sport_menu(SportManager *manager);
void sport_manager_enter_variant_menu(SportManager *manager);
void sport_manager_exit_menu(SportManager *manager); // cancel, back to running

// Move to next sport in list (BASKETBALL -> FOOTBALL -> ...)
void sport_manager_next_sport(SportManager *manager);

// Confirm currently selected variant (always the *current_variant_idx*)
// and switch to RUNNING
void sport_manager_confirm_selection(SportManager *manager);

// -----------------------------------------------------------------------------
// Legacy helpers (kept for compatibility; still usable if needed)
// -----------------------------------------------------------------------------
sport_type_t sport_manager_get_next(sport_type_t current);
sport_type_t sport_manager_get_prev(sport_type_t current);
void sport_manager_set_selected_type(SportManager *manager,
                                     sport_type_t sport_type);
