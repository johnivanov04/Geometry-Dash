#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <assert.h>

#include "asset.h"
#include "asset_cache.h"
#include "list.h"
#include "sdl_wrapper.h"

static list_t *ASSET_CACHE;

const size_t FONT_SIZE = 18;
const size_t INITIAL_CAPACITY = 5;

typedef struct {
  asset_type_t type;
  const char *filepath;
  void *obj;
} entry_t;

static void asset_cache_free_entry(entry_t *entry) {
  if (entry == NULL) {
    return;
  }

  switch (entry->type) {
  case ASSET_IMAGE: {
    SDL_DestroyTexture((SDL_Texture *)entry->obj);
    break;
  }
  case ASSET_FONT: {
    TTF_CloseFont((TTF_Font *)entry->obj);
    break;
  }
  case ASSET_BUTTON: {
    // FREEING IMAGE ASSET WITHIN BUTTON ASSET
    free(entry->obj);
    break;
  }
  }
  free(entry);
}

void asset_cache_init() {
  ASSET_CACHE =
      list_init(INITIAL_CAPACITY, (free_func_t)asset_cache_free_entry);
}

void asset_cache_destroy() { list_free(ASSET_CACHE); }

// helper function for asset_cache_obj_get_or_create
void *already_exists(asset_type_t ty, const char *filepath) {
  for (size_t i = 0; i < list_size(ASSET_CACHE); i++) {
    entry_t *entry = (entry_t *)list_get(ASSET_CACHE, i);
    assert(entry != NULL);
    if (entry->filepath && strcmp(entry->filepath, filepath) == 0) {
      assert(entry->type == ty);
      return entry->obj;
    }
  }
  return NULL;
}

void *asset_cache_obj_get_or_create(asset_type_t ty, const char *filepath) {
  void *exist_object = already_exists(ty, filepath);
  if (exist_object != NULL) {
    return exist_object;
  }

  entry_t *new_entry = malloc(sizeof(entry_t));
  assert(new_entry != NULL);
  new_entry->type = ty;
  new_entry->filepath = filepath;

  switch (ty) {
  case ASSET_IMAGE: {
    new_entry->obj = sdl_display(filepath);
    break;
  }
  case ASSET_FONT: {
    new_entry->obj = TTF_OpenFont(filepath, FONT_SIZE);
    break;
  }
  default: {
    return NULL;
  }
  }
  list_add(ASSET_CACHE, new_entry);
  return new_entry->obj;
}

void asset_cache_register_button(asset_t *button) {

  entry_t *new_entry = malloc(sizeof(entry_t));
  assert(new_entry != NULL);
  assert(asset_get_type(button) == ASSET_BUTTON);
  new_entry->type = ASSET_BUTTON;
  new_entry->filepath = NULL;
  new_entry->obj = button;

  list_add(ASSET_CACHE, new_entry);
}

void asset_cache_handle_buttons(state_t *state, double x, double y) {

  for (size_t i = 0; i < list_size(ASSET_CACHE); i++) {
    entry_t *entry = (entry_t *)list_get(ASSET_CACHE, i);
    if (entry && entry->type == ASSET_BUTTON) {
      asset_on_button_click(entry->obj, state, x, y);
    }
  }
}