
#include "sport_manager.h"
#include "esp_log.h"

static const char *TAG = "SPORT_MGR";

// -----------------------------------------------------------------------------
// Group definitions
//  - Only logos + variant sport_type_t arrays
//  - All metadata (name, seconds, behavior, colors) comes from sport_selector
// -----------------------------------------------------------------------------

// Basketball: 24 / 30
static const sport_type_t basketball_variants[] = {
    SPORT_BASKETBALL_24_SEC,
    SPORT_BASKETBALL_30_SEC,
};

// Football: 40 / 25
static const sport_type_t football_variants[] = {
    SPORT_FOOTBALL_40_SEC,
    SPORT_FOOTBALL_25_SEC,
};

// Baseball: 15 / 20 / 14 / 19
static const sport_type_t baseball_variants[] = {
    SPORT_BASEBALL_15_SEC,
    SPORT_BASEBALL_20_SEC,
    SPORT_BASEBALL_14_SEC,
    SPORT_BASEBALL_19_SEC,
};

// Volleyball: 8
static const sport_type_t volleyball_variants[] = {
    SPORT_VOLLEYBALL_8_SEC,
};

// Lacrosse: 30
static const sport_type_t lacrosse_variants[] = {
    SPORT_LACROSSE_30_SEC,
};

static const sport_group_t g_sport_groups[] = {
    {.logo = {"  __==__  ", " (      ) ", "  '--==--'"},
     .variants = basketball_variants,
     .variant_count =
         sizeof(basketball_variants) / sizeof(basketball_variants[0])},
    {.logo = {" /------\\ ", " | ()  ()|", " \\------/ "},
     .variants = football_variants,
     .variant_count = sizeof(football_variants) / sizeof(football_variants[0])},
    {.logo = {"  _____   ", " /  o  \\  ", " \\_____/  "},
     .variants = baseball_variants,
     .variant_count = sizeof(baseball_variants) / sizeof(baseball_variants[0])},
    {.logo = {" \\ | /   ", "  -O-    ", " /_|_\\   "},
     .variants = volleyball_variants,
     .variant_count =
         sizeof(volleyball_variants) / sizeof(volleyball_variants[0])},
    {.logo = {"  /\\  /\\ ", " /  \\/  \\", " \\_/\\_/ "},
     .variants = lacrosse_variants,
     .variant_count =
         sizeof(lacrosse_variants) / sizeof(lacrosse_variants[0])}};

static const size_t g_sport_group_count =
    sizeof(g_sport_groups) / sizeof(g_sport_groups[0]);

// -----------------------------------------------------------------------------
// Helpers
// -----------------------------------------------------------------------------
static bool find_group_for_type(sport_type_t type, uint8_t *out_group_idx,
                                uint8_t *out_variant_idx) {
  for (uint8_t gi = 0; gi < g_sport_group_count; gi++) {
    const sport_group_t *g = &g_sport_groups[gi];
    for (uint8_t vi = 0; vi < g->variant_count; vi++) {
      if (g->variants[vi] == type) {
        if (out_group_idx)
          *out_group_idx = gi;
        if (out_variant_idx)
          *out_variant_idx = vi;
        return true;
      }
    }
  }
  return false;
}

// Keep indices consistent with selected_sport_type
static void sync_indices_from_selected(SportManager *m) {
  uint8_t gi = 0, vi = 0;
  if (find_group_for_type(m->selected_sport_type, &gi, &vi)) {
    m->current_group_idx = gi;
    m->current_variant_idx = vi;
  } else {
    // Fallback to first sport / first variant
    m->current_group_idx = 0;
    m->current_variant_idx = 0;
    m->selected_sport_type = g_sport_groups[0].variants[0];
    m->current_sport = get_sport_config(m->selected_sport_type);
  }
}

// -----------------------------------------------------------------------------
// Init
// -----------------------------------------------------------------------------
void sport_manager_init(SportManager *manager) {
  if (!manager)
    return;

  // Default: Basketball 24
  manager->selected_sport_type = SPORT_BASKETBALL_24_SEC;
  manager->current_sport = get_sport_config(manager->selected_sport_type);
  manager->ui_state = SPORT_UI_STATE_RUNNING;

  sync_indices_from_selected(manager);

  ESP_LOGI(TAG, "Sport manager init: %s %s (%u sec)",
           manager->current_sport.name, manager->current_sport.variation,
           manager->current_sport.play_clock_seconds);
}

// -----------------------------------------------------------------------------
// Current sport / basic getters
// -----------------------------------------------------------------------------
sport_config_t sport_manager_get_current_sport(const SportManager *manager) {
  return manager->current_sport;
}

sport_type_t sport_manager_get_selected_type(const SportManager *manager) {
  return manager->selected_sport_type;
}

void sport_manager_set_sport(SportManager *manager, sport_type_t sport_type) {
  if (!manager)
    return;
  manager->selected_sport_type = sport_type;
  manager->current_sport = get_sport_config(sport_type);
  sync_indices_from_selected(manager);

  ESP_LOGI(TAG, "Sport set to: %s %s (%u sec)", manager->current_sport.name,
           manager->current_sport.variation,
           manager->current_sport.play_clock_seconds);
}

sport_ui_state_t sport_manager_get_ui_state(const SportManager *manager) {
  return manager ? manager->ui_state : SPORT_UI_STATE_RUNNING;
}

uint8_t sport_manager_get_current_group_index(const SportManager *manager) {
  return manager ? manager->current_group_idx : 0;
}

uint8_t sport_manager_get_current_variant_index(const SportManager *manager) {
  return manager ? manager->current_variant_idx : 0;
}

// -----------------------------------------------------------------------------
// Group access
// -----------------------------------------------------------------------------
const sport_group_t *sport_manager_get_groups(size_t *count) {
  if (count)
    *count = g_sport_group_count;
  return g_sport_groups;
}

const sport_group_t *sport_manager_get_group(uint8_t index) {
  if (index >= g_sport_group_count)
    return NULL;
  return &g_sport_groups[index];
}

const sport_group_t *
sport_manager_get_current_group(const SportManager *manager) {
  if (!manager)
    return NULL;
  return sport_manager_get_group(manager->current_group_idx);
}

// -----------------------------------------------------------------------------
// Menu navigation / state
// -----------------------------------------------------------------------------
void sport_manager_enter_sport_menu(SportManager *manager) {
  if (!manager)
    return;
  manager->ui_state = SPORT_UI_STATE_SELECT_SPORT;
  sync_indices_from_selected(manager);
}

void sport_manager_enter_variant_menu(SportManager *manager) {
  if (!manager)
    return;
  manager->ui_state = SPORT_UI_STATE_SELECT_VARIANT;
  sync_indices_from_selected(manager);
}

void sport_manager_exit_menu(SportManager *manager) {
  if (!manager)
    return;
  manager->ui_state = SPORT_UI_STATE_RUNNING;
  // Keep current_sport as active; no change
}

void sport_manager_next_sport(SportManager *manager) {
  if (!manager)
    return;
  if (manager->ui_state != SPORT_UI_STATE_SELECT_SPORT)
    return;

  manager->current_group_idx++;
  if (manager->current_group_idx >= g_sport_group_count)
    manager->current_group_idx = 0;

  // Always default to first variant when changing sport
  manager->current_variant_idx = 0;
  const sport_group_t *g = &g_sport_groups[manager->current_group_idx];
  manager->selected_sport_type = g->variants[manager->current_variant_idx];
}

void sport_manager_confirm_selection(SportManager *manager) {
  if (!manager)
    return;

  const sport_group_t *g = &g_sport_groups[manager->current_group_idx];
  if (g->variant_count == 0)
    return;

  // Always use current_variant_idx (usually 0 = default)
  sport_type_t chosen = g->variants[manager->current_variant_idx];
  manager->selected_sport_type = chosen;
  manager->current_sport = get_sport_config(chosen);
  manager->ui_state = SPORT_UI_STATE_RUNNING;

  ESP_LOGI(TAG, "Sport confirmed: %s %s (%u sec)", manager->current_sport.name,
           manager->current_sport.variation,
           manager->current_sport.play_clock_seconds);
}

// -----------------------------------------------------------------------------
// Legacy helpers (flat order for compatibility)
// -----------------------------------------------------------------------------
static const sport_type_t g_flat_order[] = {
    SPORT_BASKETBALL_24_SEC, SPORT_BASKETBALL_30_SEC, SPORT_FOOTBALL_40_SEC,
    SPORT_FOOTBALL_25_SEC,   SPORT_BASEBALL_15_SEC,   SPORT_BASEBALL_20_SEC,
    SPORT_BASEBALL_14_SEC,   SPORT_BASEBALL_19_SEC,   SPORT_VOLLEYBALL_8_SEC,
    SPORT_LACROSSE_30_SEC,
};

static const size_t g_flat_count =
    sizeof(g_flat_order) / sizeof(g_flat_order[0]);

sport_type_t sport_manager_get_next(sport_type_t current) {
  for (size_t i = 0; i < g_flat_count; i++) {
    if (g_flat_order[i] == current) {
      return g_flat_order[(i + 1) % g_flat_count];
    }
  }
  return g_flat_order[0];
}

sport_type_t sport_manager_get_prev(sport_type_t current) {
  for (size_t i = 0; i < g_flat_count; i++) {
    if (g_flat_order[i] == current) {
      if (i == 0)
        return g_flat_order[g_flat_count - 1];
      return g_flat_order[i - 1];
    }
  }
  return g_flat_order[0];
}

void sport_manager_set_selected_type(SportManager *manager,
                                     sport_type_t sport_type) {
  if (!manager)
    return;
  manager->selected_sport_type = sport_type;
  sync_indices_from_selected(manager);
}
