#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <assert.h>

#include "asset.h"
#include "asset_cache.h"
#include "color.h"
#include "sdl_wrapper.h"

typedef struct asset {
  asset_type_t type;
  SDL_Rect bounding_box;
} asset_t;

typedef struct text_asset {
  asset_t base;
  TTF_Font *font;
  const char *text;
  rgb_color_t color;
} text_asset_t;

typedef struct image_asset {
  asset_t base;
  SDL_Texture *texture;
  body_t *body;
} image_asset_t;

typedef struct button_asset {
  asset_t base;
  image_asset_t *image_asset;
  text_asset_t *text_asset;
  button_handler_t handler;
  bool is_rendered;
} button_asset_t;

/**
 * Allocates memory for an asset with the given parameters.
 *
 * @param ty the type of the asset
 * @param bounding_box the bounding box containing the location and dimensions
 * of the asset when it is rendered
 * @return a pointer to the newly allocated asset
 */
static asset_t *asset_init(asset_type_t ty, SDL_Rect bounding_box) {
  asset_t *new;
  switch (ty) {
  case ASSET_IMAGE: {
    new = malloc(sizeof(image_asset_t));
    break;
  }
  case ASSET_FONT: {
    new = malloc(sizeof(text_asset_t));
    break;
  }
  case ASSET_BUTTON: {
    new = malloc(sizeof(button_asset_t));
    break;
  }
  default: {
    assert(false && "Unknown asset type");
  }
  }
  assert(new);
  new->type = ty;
  new->bounding_box = bounding_box;
  return new;
}

asset_type_t asset_get_type(asset_t *asset) { return asset->type; }

asset_t *asset_make_image(const char *filepath, SDL_Rect bounding_box) {
  image_asset_t *img_asset =
      (image_asset_t *)asset_init(ASSET_IMAGE, bounding_box);
  img_asset->texture =
      (SDL_Texture *)asset_cache_obj_get_or_create(ASSET_IMAGE, filepath);
  img_asset->body = NULL;
  assert(img_asset->texture);

  return (asset_t *)img_asset;
}

asset_t *asset_make_image_with_body(const char *filepath, body_t *body) {
  SDL_Rect arbitrary_rect = {0, 0, 0, 0};
  image_asset_t *img_asset =
      (image_asset_t *)asset_init(ASSET_IMAGE, arbitrary_rect);
  img_asset->texture =
      (SDL_Texture *)asset_cache_obj_get_or_create(ASSET_IMAGE, filepath);
  img_asset->body = body;
  assert(img_asset->texture);

  return (asset_t *)img_asset;
}

asset_t *asset_make_text(const char *filepath, SDL_Rect bounding_box,
                         const char *text, rgb_color_t color) {
  text_asset_t *text_asset =
      (text_asset_t *)asset_init(ASSET_FONT, bounding_box);
  text_asset->font =
      (TTF_Font *)asset_cache_obj_get_or_create(ASSET_FONT, filepath);
  assert(text_asset->font);
  text_asset->text = strdup(text);
  text_asset->color = color;
  return (asset_t *)text_asset;
}

asset_t *asset_make_button(SDL_Rect bounding_box, asset_t *image_asset,
                           asset_t *text_asset, button_handler_t handler) {
  button_asset_t *new_button =
      (button_asset_t *)asset_init(ASSET_BUTTON, bounding_box);
  if (image_asset != NULL) {
    assert(image_asset->type == ASSET_IMAGE);
    new_button->image_asset = (image_asset_t *)image_asset;
  } else {
    new_button->image_asset = NULL;
  }
  if (text_asset != NULL) {
    assert(text_asset->type == ASSET_FONT);
    new_button->text_asset = (text_asset_t *)text_asset;
  } else {
    new_button->text_asset = NULL;
  }

  new_button->handler = handler;
  new_button->is_rendered = false;
  return (asset_t *)new_button;
}

// helper function for asset_on_button_click
static bool is_point_in_rect(double x, double y, SDL_Rect rect) {
  return (x >= rect.x && x < rect.x + rect.w && y >= rect.y &&
          y < rect.y + rect.h);
}

void asset_on_button_click(asset_t *button, state_t *state, double x,
                           double y) {
  button_asset_t *button_asset = (button_asset_t *)button;
  if (!button_asset->is_rendered) {
    return;
  }
  if (is_point_in_rect(x, y, button_asset->base.bounding_box)) {
    button_asset->handler(state);
  }
  button_asset->is_rendered = false;
}

void asset_render(asset_t *asset) {
  switch (asset->type) {
  case ASSET_IMAGE: {
    image_asset_t *img_asset = (image_asset_t *)asset;
    SDL_Rect render_rect;
    if (img_asset->body != NULL) {
      render_rect = get_body_bounding_box(img_asset->body);
    } else {
      render_rect = img_asset->base.bounding_box;
    }
    sdl_render(img_asset->texture, render_rect.x, render_rect.y, render_rect.w,
               render_rect.h);
    break;
  }
  case ASSET_FONT: {
    text_asset_t *text_asset = (text_asset_t *)asset;
    SDL_Color sdl_color;
    sdl_color.r = (Uint8)text_asset->color.r;
    sdl_color.g = (Uint8)text_asset->color.g;
    sdl_color.b = (Uint8)text_asset->color.b;

    SDL_Texture *texture = text_render(text_asset->text, text_asset->font);
    vector_t position =
        (vector_t){.x = (double)text_asset->base.bounding_box.x,
                   .y = (double)text_asset->base.bounding_box.y};
    text_display(texture, position);
    SDL_DestroyTexture(texture);
    break;
  }
  case ASSET_BUTTON: {
    button_asset_t *button_asset = (button_asset_t *)asset;
    asset_render((asset_t *)(button_asset->image_asset));
    if (button_asset->text_asset) {
      asset_render((asset_t *)(button_asset->text_asset));
    }
    button_asset->is_rendered = true;
    break;
  }
  }
}

void asset_destroy(asset_t *asset) { free(asset); }

body_t *asset_to_body(asset_t *image) {
  image_asset_t *img = (image_asset_t *)(image);
  if (img->base.type == ASSET_IMAGE) {
    body_t *body = img->body;
    return body;
  }
  return NULL;
}
