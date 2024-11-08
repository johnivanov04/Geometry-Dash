#include "asset_cache.h"
#include "collision.h"
#include "forces.h"
#include "sdl_wrapper.h"
#include <SDL2/SDL_mixer.h>
#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include <time.h>

const double ELASTICITY = 0;
const vector_t MIN = {0, 0};
const vector_t MAX = {1000, 500};
vector_t dasher_center = {50, MIN.y + 70};
vector_t OBSTACLE_C = {1000, MIN.y + 70};
SDL_Rect dasher_rect = {0, 0, 50, 50};
const vector_t off_screen_start = {1499, 500};
const size_t NUM_ATTEMPTS = 4;

const char *MUSIC_L1 = "assets/stereo-madness-full-song-download.wav";
const char *DEATH_SOUND = "assets/Death-Sound.wav";
const char *DASHER_IMAGE = "assets/geometryChar.png";
char *START_SCREEN = "assets/START_SCREEN.png";
const char *PLAY_BUTTON = "assets/PLAYBUTTON.jpeg";
const char *DETH_EFFECT = "assets/explosion.png";

const char *FRONTBACK_IMAGE = "assets/steromadnes.png";
const char *SPIKES = "assets/spikes.png";
const char *BOX = "assets/black_square.png";
const char *COINS = "assets/coin.png";
const char *COIN_SOUND_EFFECT = "assets/coin-sound.wav";
SDL_Rect bg1_rect = {0, 0, MAX.x, MAX.y};
const vector_t BG_SPEED = {25, 0};
const vector_t FRONT_SPEED = {50, 0};
const vector_t JUMP_VELO = {0, 750};
const double GRAVITY_STRENGTH = 2000;
const vector_t WALL_OBSTACLE_VELO = {-200, 0};
const vector_t COIN_OFFSCREEN = {-200, -200};
const double OFFSCREEN_OBJ = 0.0;
const double BLOCK_W = 40;
const double BLOCK_H = 40;
const double WALL_DIMENSION = 5;
const double FLOOR_HEIGHT = 10;
const double CENTROID_Y = 70;
const double BG_HEIGHT = 50;

char *LEVEL1 = "assets/level1.png";
char *LEVEL2 = "assets/level2.png";
char *END_SCREEN = "assets/END_SCREEN.png";

const rgb_color_t black = (rgb_color_t){0, 0, 0};
const double LEVEL_LENGTH = 30;
const double MIN_COIN = 100.0;
const double MAX_COIN = 200.0;
const double COIN_SPAWN_X = 1100;

const rgb_color_t WHITE = (rgb_color_t){255, 255, 255};
const char *FONT = "assets/impact.ttf";
const SDL_Rect ATTEMPTS_LOC1 = {500, 150, 200, 50};
const SDL_Rect ATTEMPS_TEXT_LOC = {430, 150, 200, 50};
const SDL_Rect ATTEMPTS_LOC2 = {300, 275, 200, 50};
const SDL_Rect ATTEMPS_TEXT_LOC2 = {300, 250, 200, 50};

const SDL_Rect COINS_TEXT_LOC = {15, 15, 200, 50};
const SDL_Rect COINS_LOC = {10, 40, 200, 50};
const SDL_Rect COINS_TEXT_LOC2 = {625, 250, 200, 50};
const SDL_Rect COINS_LOC2 = {625, 275, 200, 50};
const SDL_Rect LEVEL_TITLE_LOC = {430, 100, 200, 50};

const double INITIAL_OBSTACLE_VELOCITY = -260;
const double CURR_OB_VELO = -330;
double current_obstacle_velocity = INITIAL_OBSTACLE_VELOCITY;

typedef struct state {
  long long attempts;
  asset_t *end_screen_attempts;
  body_t *dasher;
  scene_t *scene;
  long long curr_coins;
  bool is_jumping;
  bool on_start_screen;
  bool on_end_screen;
  asset_t *button;
  char *curr_bg_path;
  asset_t *curr_bg;

  body_t *background;
  body_t *front_background;
  list_t *body_assets;
  body_t *backdrop;
  body_t *lower_background;
  double time;
} state_t;

typedef enum {
  DASHER,
  FLOOR, // Able to bounce on top
  WALL,  // Destruction when collide on side of wall
  SCREEN,
  OBSTACLE,
} body_type_t;

typedef struct button_info {
  const char *image_path;
  SDL_Rect image_box;
  button_handler_t handler;
} button_info_t;

void init_music() {
  SDL_Init(SDL_INIT_AUDIO);
  Mix_Init(0);
  Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048);
  Mix_Music *music = Mix_LoadMUS(MUSIC_L1);
  Mix_PlayMusic(music, -1);
}

void stop_music() { Mix_HaltMusic(); }

void sound_effects(const char *filepath) {
  SDL_Init(SDL_INIT_AUDIO);
  Mix_OpenAudio(4412, MIX_DEFAULT_FORMAT, 2, 300);
  Mix_Chunk *music = Mix_LoadWAV(filepath);
  Mix_PlayChannel(1, music, 0);
}

body_type_t *make_type_info(body_type_t type) {
  body_type_t *info = malloc(sizeof(body_type_t));
  assert(info);
  *info = type;
  return info;
}

body_type_t get_type(body_t *body) {
  return *(body_type_t *)body_get_info(body);
}

// button handler for the play button and play again button
void play(state_t *state) {
  state->on_start_screen = false;
  state->curr_bg_path = LEVEL1;
  init_music();

  for (size_t i = 0; i < scene_bodies(state->scene); i++) {
    body_t *body = scene_get_body(state->scene, i);
    body_type_t type = get_type(body);
    if (type == WALL || type == OBSTACLE) {
      body_set_velocity(body, WALL_OBSTACLE_VELO);
    }
  }
}

void play_again(state_t *state) {
  state->on_end_screen = false;
  state->curr_bg_path = LEVEL1;
  state->time = 0;
  state->curr_coins = 0;
  state->attempts = 1;
  stop_music();
  init_music();
  asset_t *new_asset1 = asset_make_image_with_body(LEVEL1, state->background);
  asset_t *new_asset3 = asset_make_image_with_body(LEVEL1, state->backdrop);
  list_set(state->body_assets, 0, new_asset1);
  list_set(state->body_assets, 1, new_asset3);

  for (size_t i = 0; i < scene_bodies(state->scene); i++) {
    body_t *body = scene_get_body(state->scene, i);
    body_type_t type = get_type(body);
    if (type == WALL || type == OBSTACLE) {
      body_set_velocity(body, WALL_OBSTACLE_VELO);
    }
  }
}

button_info_t play_button_info = {.image_path = "assets/PLAYBUTTON.jpeg",
                                  .image_box = {425, 275, 150, 40},
                                  .handler = (void *)play};

button_info_t play_again_button_info = {.image_path =
                                            "assets/PLAY_AGAIN_BUTTON.png",
                                        .image_box = {375, 380, 250, 65},
                                        .handler = (void *)play_again};

asset_t *get_background(const char *path) {
  SDL_Rect background_dim;
  background_dim.x = MIN.x;
  background_dim.y = MIN.y;
  background_dim.h = MAX.y;
  background_dim.w = MAX.x;
  asset_t *background = asset_make_image(path, background_dim);
  return background;
}

void remove_all_obstacles(state_t *state) {
  for (size_t i = 0; i < scene_bodies(state->scene); i++) {
    body_t *body = scene_get_body(state->scene, i);
    if (get_type(body) == WALL || get_type(body) == OBSTACLE ||
        get_type(body) == FLOOR) {
      body_reset(body);
      scene_remove_body(state->scene, i);
    }
  }
  for (size_t i = 0; i < list_size(state->body_assets); i++) {
    asset_t *asset = list_get(state->body_assets, i);
    body_t *body = asset_to_body(asset);
    if (body_is_removed(body)) {
      asset_destroy(asset);
      list_remove(state->body_assets, i);
      i--;
    }
  }
}

void reset_to_level1(state_t *state) {
  stop_music();
  init_music();

  remove_all_obstacles(state);
  state->time = 0;

  state->curr_bg_path = LEVEL1;
  asset_t *new_bg = asset_make_image_with_body(LEVEL1, state->background);
  list_set(state->body_assets, 0, new_bg);
  asset_t *new_backdrop = asset_make_image_with_body(LEVEL1, state->backdrop);
  list_set(state->body_assets, 1, new_backdrop);

  current_obstacle_velocity = INITIAL_OBSTACLE_VELOCITY;
  for (size_t i = 0; i < scene_bodies(state->scene); i++) {
    body_t *body = scene_get_body(state->scene, i);
    body_type_t type = get_type(body);
    if (type == WALL || type == OBSTACLE) {
      vector_t body_vel = {current_obstacle_velocity, 0};
      body_set_velocity(body, body_vel);
    }
  }
}

void reset_game(body_t *body1, body_t *body2, vector_t axis, void *aux,
                double force_const) {
  state_t *state = aux;
  body_set_centroid(body1, dasher_center);
  body_reset(body1);

  // Short Visual effect to not distract player from game
  sound_effects(DEATH_SOUND);
  SDL_Rect death_box = {0, MAX.y - 175, 200, 200};
  asset_t *death = asset_make_image(DETH_EFFECT, death_box);
  asset_render(death);
  asset_destroy(death);
  state->curr_coins = 0;
  state->attempts++;
  state->is_jumping = true;
  state->time = 0;
  if (strcmp(state->curr_bg_path, LEVEL2) == 0) {
    reset_to_level1(state);
  } else {
    stop_music();
    init_music();
    remove_all_obstacles(state);
  }
}

void coin_collision(body_t *body1, body_t *body2, vector_t axis, void *aux,
                    double force_const) {
  state_t *state = aux;
  sound_effects(COIN_SOUND_EFFECT);
  state->curr_coins++;
  state->is_jumping = true;

  body_set_centroid(body2, COIN_OFFSCREEN);
  body_remove(body2);
}

asset_t *create_button_from_info(state_t *state, button_info_t info) {
  asset_t *image_asset = NULL;
  if (info.image_path != NULL) {
    image_asset = asset_make_image(info.image_path, info.image_box);
  }
  asset_t *new_button =
      asset_make_button(info.image_box, image_asset, NULL, info.handler);
  asset_cache_register_button(new_button);
  return new_button;
}

// Make a rectangle-shaped body object.
list_t *make_rectangle(vector_t center, double width, double height) {
  list_t *points = list_init(4, free);
  vector_t *p1 = malloc(sizeof(vector_t));
  assert(p1);
  *p1 = (vector_t){center.x - width / 2, center.y - height / 2};

  vector_t *p2 = malloc(sizeof(vector_t));
  assert(p2);
  *p2 = (vector_t){center.x + width / 2, center.y - height / 2};

  vector_t *p3 = malloc(sizeof(vector_t));
  assert(p3);
  *p3 = (vector_t){center.x + width / 2, center.y + height / 2};

  vector_t *p4 = malloc(sizeof(vector_t));
  assert(p4);
  *p4 = (vector_t){center.x - width / 2, center.y + height / 2};

  list_add(points, p1);
  list_add(points, p2);
  list_add(points, p3);
  list_add(points, p4);

  return points;
}

// Collision handler for dasher colliding with floor
void dasher_floor_collision_handler(body_t *dasher, body_t *floor,
                                    vector_t axis, void *aux,
                                    double elasticity) {
  state_t *state = aux;
  vector_t velocity = body_get_velocity(dasher);
  velocity.y = 0;
  body_set_velocity(dasher, velocity);
  vector_t dasher_centroid = body_get_centroid(dasher);

  body_set_centroid(dasher, dasher_centroid);

  state->is_jumping = false;
}

void off_floor(state_t *state) {
  vector_t centroid = body_get_centroid(state->dasher);
  if (centroid.y < (CENTROID_Y)) {
    vector_t new_cent = {centroid.x, CENTROID_Y};
    body_set_centroid(state->dasher, new_cent);
    state->is_jumping = false;
  }
}

void on_key(char key, key_event_type_t type, double held_time, state_t *state) {
  body_t *user = state->dasher;
  if (type == KEY_PRESSED) {
    switch (key) {
    case UP_ARROW:
    case SPACE_BAR:
    case SDL_MOUSEBUTTONDOWN:
      if (!state->is_jumping) {
        vector_t velocity = body_get_velocity(user);
        velocity.y = JUMP_VELO.y;
        body_set_velocity(user, velocity);
        state->is_jumping = true;
      }
      break;
    default:
      break;
    }
  }
}

void update_background_positions(state_t *state, double dt) {
  vector_t bg_pos = body_get_centroid(state->background);
  vector_t front_pos = body_get_centroid(state->front_background);
  vector_t drop_pos = body_get_centroid(state->backdrop);
  vector_t lower_pos = body_get_centroid(state->lower_background);

  bg_pos.x -= BG_SPEED.x * dt;
  drop_pos.x -= BG_SPEED.x * dt;
  front_pos.x -= FRONT_SPEED.x * dt;
  lower_pos.x -= FRONT_SPEED.x * dt;

  if (bg_pos.x <= -MAX.x / 2) {
    bg_pos.x = off_screen_start.x;
  }
  if (drop_pos.x <= -MAX.x / 2) {
    drop_pos.x = off_screen_start.x;
  }

  if (front_pos.x <= -MAX.x / 2) {
    front_pos.x = off_screen_start.x;
  }

  if (lower_pos.x <= -MAX.x / 2) {
    lower_pos.x = off_screen_start.x;
  }

  body_set_centroid(state->backdrop, drop_pos);
  body_set_centroid(state->background, bg_pos);
  body_set_centroid(state->front_background, front_pos);
  body_set_centroid(state->lower_background, lower_pos);
}

body_t *background_helper(state_t *state, vector_t center, double width,
                          double height, rgb_color_t color, const char *image) {
  list_t *rectangle = make_rectangle(center, width, height);
  body_t *back = body_init_with_info(rectangle, INFINITY, color,
                                     make_type_info(SCREEN), free);
  asset_t *asset_made = asset_make_image_with_body(image, back);
  list_add(state->body_assets, asset_made);
  scene_add_body(state->scene, back);
  return back;
}

body_t *make_dasher(vector_t center) {
  SDL_Rect rect = dasher_rect;
  list_t *shape = make_rectangle(center, rect.w, rect.h);
  body_t *dasher =
      body_init_with_info(shape, 1, WHITE, make_type_info(DASHER), free);
  return dasher;
}

void add_coin(state_t *state, vector_t center, double width, double height) {
  list_t *coin_shape = make_rectangle(center, width, height);
  body_t *coin = body_init_with_info(coin_shape, INFINITY, WHITE,
                                     make_type_info(OBSTACLE), free);
  asset_t *asset_obj = asset_make_image_with_body(COINS, coin);
  list_add(state->body_assets, asset_obj);
  scene_add_body(state->scene, coin);
  vector_t body_vel = {current_obstacle_velocity, 0};
  body_set_velocity(coin, body_vel);
  create_collision(state->scene, state->dasher, coin, coin_collision, state,
                   0.0);
}

// Creates obstacles for the user. This obstacle the user can not
// collide with otherwise they lose
void add_obstacles_unjumpable(state_t *state, vector_t center, double width,
                              double height) {
  list_t *obstacle_shape = make_rectangle(center, width, height);
  body_t *obstacle = body_init_with_info(obstacle_shape, INFINITY, WHITE,
                                         make_type_info(WALL), free);
  asset_t *asset_ob = asset_make_image_with_body(SPIKES, obstacle);
  list_add(state->body_assets, asset_ob);
  scene_add_body(state->scene, obstacle);

  vector_t body_vel = {current_obstacle_velocity, 0};
  body_set_velocity(obstacle, body_vel);
  create_collision(state->scene, state->dasher, obstacle, reset_game, state,
                   0.0);
}

void add_obstacles_jumpable(state_t *state, vector_t center, double width,
                            double height, double wall_dim) {
  vector_t floor_center = {center.x, center.y + height / 2};
  list_t *floor_shape = make_rectangle(floor_center, width, wall_dim - 2);
  body_t *floor = body_init_with_info(floor_shape, INFINITY, black,
                                      make_type_info(FLOOR), free);
  scene_add_body(state->scene, floor);

  vector_t left_wall_center = {(center.x - width / 2 + wall_dim / 2),
                               center.y - 2};
  list_t *left_wall_shape =
      make_rectangle(left_wall_center, wall_dim, height - (3 * wall_dim));
  body_t *left_wall = body_init_with_info(left_wall_shape, INFINITY, black,
                                          make_type_info(WALL), free);
  scene_add_body(state->scene, left_wall);

  list_t *center_square_shape =
      make_rectangle((vector_t){center.x, center.y}, width, height);
  body_t *center_square = body_init_with_info(
      center_square_shape, INFINITY, black, make_type_info(OBSTACLE), free);
  asset_t *asset_square = asset_make_image_with_body(BOX, center_square);
  list_add(state->body_assets, asset_square);
  scene_add_body(state->scene, center_square);

  vector_t body_vel = {current_obstacle_velocity, 0};
  body_set_velocity(center_square, body_vel);
  body_set_velocity(floor, body_vel);
  body_set_velocity(left_wall, body_vel);

  create_collision(state->scene, state->dasher, floor,
                   dasher_floor_collision_handler, state, 0.0);

  create_collision(state->scene, state->dasher, left_wall, reset_game, state,
                   0.0);
}

void triple_block(state_t *state, vector_t center) {
  double block_width = BLOCK_W;
  double block_height = BLOCK_H;

  vector_t block1_center = center;
  vector_t block2_center = {center.x + block_width, center.y};
  vector_t block3_center = {center.x + 2 * block_width, center.y};

  add_obstacles_jumpable(state, block1_center, block_width, block_height,
                         WALL_DIMENSION);
  add_obstacles_jumpable(state, block2_center, block_width, block_height,
                         WALL_DIMENSION);
  add_obstacles_jumpable(state, block3_center, block_width, block_height,
                         WALL_DIMENSION);
}

void five_block(state_t *state, vector_t center) {
  double block_width = BLOCK_W;
  double block_height = BLOCK_H;

  vector_t block1_center = center;
  vector_t block2_center = {center.x + block_width, center.y};
  vector_t block3_center = {center.x + 2 * block_width, center.y};
  vector_t block4_center = {center.x + 3 * block_width, center.y};
  vector_t block5_center = {center.x + 4 * block_width, center.y};

  add_obstacles_jumpable(state, block1_center, block_width, block_height,
                         WALL_DIMENSION);
  add_obstacles_jumpable(state, block2_center, block_width, block_height,
                         WALL_DIMENSION);
  add_obstacles_jumpable(state, block3_center, block_width, block_height,
                         WALL_DIMENSION);
  add_obstacles_jumpable(state, block4_center, block_width, block_height,
                         WALL_DIMENSION);
  add_obstacles_jumpable(state, block5_center, block_width, block_height,
                         WALL_DIMENSION);
}

void triple_staircase(state_t *state, vector_t center) {
  double block_width = BLOCK_W;
  double block_height = BLOCK_H;

  vector_t block1_center = center;

  vector_t block2_center = {center.x + 5 * block_width, 20 + center.y};

  vector_t block3_center = {center.x + 10 * block_width, 40 + center.y};

  add_obstacles_jumpable(state, block1_center, block_width, block_height,
                         WALL_DIMENSION);
  add_obstacles_jumpable(state, block2_center, block_width, 2 * block_height,
                         WALL_DIMENSION);
  add_obstacles_jumpable(state, block3_center, block_width, 3 * block_height,
                         WALL_DIMENSION);
}

void double_staircase(state_t *state, vector_t center) {
  double block_width = BLOCK_W;
  double block_height = BLOCK_H;

  vector_t block1_center = center;

  vector_t block2_center = {center.x + 6 * block_width, 20 + center.y};

  add_obstacles_jumpable(state, block1_center, block_width, block_height,
                         WALL_DIMENSION);
  add_obstacles_jumpable(state, block2_center, block_width, 2 * block_height,
                         WALL_DIMENSION);
}

void triple_spike(state_t *state, vector_t center) {
  double spike_width = BLOCK_W;
  double spike_height = BLOCK_H;

  vector_t spike1_center = center;
  vector_t spike2_center = {center.x + spike_width, center.y};
  vector_t spike3_center = {center.x + 2 * spike_width, center.y};

  add_obstacles_unjumpable(state, spike1_center, spike_width, spike_height);
  add_obstacles_unjumpable(state, spike2_center, spike_width, spike_height);
  add_obstacles_unjumpable(state, spike3_center, spike_width, spike_height);
}

void double_spike(state_t *state, vector_t center) {
  double spike_width = BLOCK_W;
  double spike_height = BLOCK_H;

  vector_t spike1_center = center;
  vector_t spike2_center = {center.x + spike_width, center.y};

  add_obstacles_unjumpable(state, spike1_center, spike_width, spike_height);
  add_obstacles_unjumpable(state, spike2_center, spike_width, spike_height);
}

// Update screen if level is complete
void update_level(state_t *state) {
  if (state->time > LEVEL_LENGTH) {
    if (strcmp(state->curr_bg_path, LEVEL1) == 0) {
      remove_all_obstacles(state);
      state->curr_bg_path = LEVEL2;
      state->time = 0;

      asset_t *new_asset1 =
          asset_make_image_with_body(LEVEL2, state->background);
      asset_t *new_asset3 = asset_make_image_with_body(LEVEL2, state->backdrop);
      list_set(state->body_assets, 0, new_asset1);
      list_set(state->body_assets, 1, new_asset3);

      current_obstacle_velocity = CURR_OB_VELO;
    } else if (strcmp(state->curr_bg_path, LEVEL2) == 0) {
      state->curr_bg_path = END_SCREEN;
      state->button = create_button_from_info(state, play_again_button_info);
      current_obstacle_velocity = INITIAL_OBSTACLE_VELOCITY;
      state->on_end_screen = true;
    }
  }
}

typedef void (*obstacle_func_t)(state_t *state, vector_t center);

void add_obstacles_unjumpable_wrapper(state_t *state, vector_t center) {
  add_obstacles_unjumpable(state, center, BLOCK_W, BLOCK_H);
}

void add_obstacles_jumpable_wrapper(state_t *state, vector_t center) {
  add_obstacles_jumpable(state, center, BLOCK_W, BLOCK_H, WALL_DIMENSION);
}

obstacle_func_t level1_obstacle_functions[] = {add_obstacles_jumpable_wrapper,
                                               add_obstacles_jumpable_wrapper,
                                               add_obstacles_jumpable_wrapper,
                                               add_obstacles_jumpable_wrapper,
                                               add_obstacles_unjumpable_wrapper,
                                               add_obstacles_unjumpable_wrapper,
                                               add_obstacles_unjumpable_wrapper,
                                               add_obstacles_unjumpable_wrapper,
                                               double_spike,
                                               double_spike,
                                               triple_staircase,
                                               triple_staircase,
                                               five_block,
                                               five_block,
                                               triple_block,
                                               triple_block};

obstacle_func_t level2_obstacle_functions[] = {add_obstacles_jumpable_wrapper,
                                               add_obstacles_jumpable_wrapper,
                                               add_obstacles_unjumpable_wrapper,
                                               add_obstacles_unjumpable_wrapper,
                                               double_spike,
                                               double_spike,
                                               double_staircase,
                                               double_staircase,
                                               five_block,
                                               five_block,
                                               triple_block,
                                               triple_block,
                                               triple_spike};

void add_random_obstacles(state_t *state) {
  size_t num_obstacles;
  size_t random_index;
  vector_t obstacle_center = OBSTACLE_C;

  if (strcmp(state->curr_bg_path, LEVEL1) == 0) {
    num_obstacles = sizeof(level1_obstacle_functions) /
                    sizeof(level1_obstacle_functions[0]);
    random_index = rand() % num_obstacles;
    level1_obstacle_functions[random_index](state, obstacle_center);
  } else if (strcmp(state->curr_bg_path, LEVEL2) == 0) {
    num_obstacles = sizeof(level2_obstacle_functions) /
                    sizeof(level2_obstacle_functions[0]);
    random_index = rand() % num_obstacles;
    level2_obstacle_functions[random_index](state, obstacle_center);
  }
}

void remove_off_screen_bodies(state_t *state) {
  for (size_t i = 0; i < scene_bodies(state->scene); i++) {
    body_t *body = scene_get_body(state->scene, i);
    body_type_t type = get_type(body);
    if (type == OBSTACLE || type == WALL || type == FLOOR) {
      if (body_get_centroid(body).x < OFFSCREEN_OBJ) {
        body_reset(body);
        scene_remove_body(state->scene, i);
      }
    }
  }
}

bool is_in_contact_with_floor(state_t *state) {
  for (size_t i = 0; i < scene_bodies(state->scene); i++) {
    body_t *body = scene_get_body(state->scene, i);
    if (get_type(body) == FLOOR) {
      if (find_collision(state->dasher, body).collided) {
        if (body_get_velocity(state->dasher).y <= 0) {
          return true;
        }
      }
    }
  }
  return false;
}

state_t *emscripten_init() {
  asset_cache_init();
  sdl_init(MIN, MAX);
  state_t *state = malloc(sizeof(state_t));
  assert(state);
  state->scene = scene_init();
  assert(state->scene);
  state->is_jumping = false;
  state->curr_coins = 0;
  state->body_assets = list_init(5, (free_func_t)asset_destroy);
  state->attempts = 1;
  state->end_screen_attempts = NULL;
  state->on_start_screen = true;
  state->on_end_screen = false;
  state->curr_bg_path = START_SCREEN;
  state->time = 0;
  state->curr_coins = 0;

  // Render start screen
  asset_t *start_background = get_background(START_SCREEN);
  state->curr_bg = start_background;
  asset_render(state->curr_bg);

  // Create play button
  state->button = create_button_from_info(state, play_button_info);

  body_t *temp = background_helper(state, (vector_t){MAX.x / 2, MAX.y / 2},
                                   MAX.x, MAX.y, black, LEVEL1);
  state->background = temp;

  body_t *temp2 = background_helper(
      state, (vector_t){off_screen_start.x, off_screen_start.y / 2}, MAX.x,
      MAX.y, black, LEVEL1);
  state->backdrop = temp2;

  body_t *temp3 = background_helper(state, (vector_t){MAX.x / 2, MIN.y + 25},
                                    MAX.x, BG_HEIGHT, black, FRONTBACK_IMAGE);
  state->front_background = temp3;

  body_t *temp4 =
      background_helper(state, (vector_t){off_screen_start.x, MIN.y + 25},
                        MAX.x, BG_HEIGHT, black, FRONTBACK_IMAGE);
  state->lower_background = temp4;

  body_t *dash = make_dasher(dasher_center);

  body_set_centroid(dash, dasher_center);
  scene_add_body(state->scene, dash);
  asset_t *dash_asset = asset_make_image_with_body(DASHER_IMAGE, dash);
  list_add(state->body_assets, dash_asset);
  state->dasher = dash;

  sdl_on_key((key_handler_t)on_key);
  return state;
}

bool emscripten_main(state_t *state) {
  double dt = time_since_last_tick();
  sdl_clear();
  if (!state->on_start_screen && !state->on_end_screen) {
    state->time += dt;

    static double obstacle_timer = 0;
    obstacle_timer += dt;
    static double coin_timer = 0;
    coin_timer += dt;
    if (coin_timer >= 5.0) {
      double random_double =
          MIN_COIN + (double)rand() / RAND_MAX * (MAX_COIN - MIN_COIN);
      add_coin(state, (vector_t){COIN_SPAWN_X, random_double}, 30, 30);
      coin_timer = 0;
    }

    double obstacle_interval;
    if (strcmp(state->curr_bg_path, LEVEL2) == 0) {
      obstacle_interval = 1.5;
    } else {
      obstacle_interval = 2.0;
    }

    if (obstacle_timer >= obstacle_interval) {
      add_random_obstacles(state);
      obstacle_timer = 0;
    }
  }

  remove_off_screen_bodies(state);
  update_background_positions(state, dt);
  update_level(state);

  bool in_contact_with_floor = is_in_contact_with_floor(state);

  if (state->is_jumping || !in_contact_with_floor) {
    vector_t velocity = body_get_velocity(state->dasher);
    velocity.y -= GRAVITY_STRENGTH * dt;
    body_set_velocity(state->dasher, velocity);
  }

  asset_t *screen = asset_make_image(state->curr_bg_path, bg1_rect);
  asset_render(screen);

  state->curr_bg = asset_make_image(state->curr_bg_path, bg1_rect);
  if (state->on_start_screen) {
    asset_render(state->button);
  } else if (!state->on_start_screen && !state->on_end_screen) {
    for (size_t i = 0; i < list_size(state->body_assets); i++) {
      asset_t *asset = list_get(state->body_assets, i);
      body_t *body = asset_to_body(asset);
      if (body_is_removed(body)) {
        list_remove(state->body_assets, i);
        i--;
      } else {
        asset_render(list_get(state->body_assets, i));
      }
    }

    char *attempts_txt = malloc(sizeof(char) * (NUM_ATTEMPTS + 1));
    assert(attempts_txt);
    snprintf(attempts_txt, NUM_ATTEMPTS + 1, "%3lld", state->attempts);
    asset_t *attempts_txt_asset =
        asset_make_text(FONT, ATTEMPTS_LOC1, attempts_txt, WHITE);

    asset_render(attempts_txt_asset);

    asset_t *attempts_context =
        asset_make_text(FONT, ATTEMPS_TEXT_LOC, "Attempts:", WHITE);

    asset_render(attempts_context);

    char *coins_txt = malloc(sizeof(char) * (NUM_ATTEMPTS + 1));
    assert(coins_txt);
    snprintf(coins_txt, NUM_ATTEMPTS + 1, "%3lld", state->curr_coins);
    asset_t *coins_txt_asset =
        asset_make_text(FONT, COINS_LOC, coins_txt, WHITE);

    asset_render(coins_txt_asset);

    asset_t *coins_context =
        asset_make_text(FONT, COINS_TEXT_LOC, "Coins:", WHITE);
    asset_render(coins_context);

    if (strcmp(state->curr_bg_path, LEVEL1) == 0) {
      asset_t *level_title_asset =
          asset_make_text(FONT, LEVEL_TITLE_LOC, "Level1", WHITE);
      asset_render(level_title_asset);

      asset_destroy(level_title_asset);
    } else if (strcmp(state->curr_bg_path, LEVEL2) == 0) {
      asset_t *level_title_asset =
          asset_make_text(FONT, LEVEL_TITLE_LOC, "Level2", WHITE);
      asset_render(level_title_asset);

      asset_destroy(level_title_asset);
    }

    free(coins_txt);
    asset_destroy(coins_txt_asset);
    asset_destroy(coins_context);
    free(attempts_txt);
    asset_destroy(attempts_txt_asset);
    asset_destroy(attempts_context);
  } else {
    remove_all_obstacles(state);
    char *attempts_txt = malloc(sizeof(char) * (NUM_ATTEMPTS + 1));
    assert(attempts_txt);
    snprintf(attempts_txt, NUM_ATTEMPTS + 1, "%3lld", state->attempts);
    asset_t *attempts_txt_asset =
        asset_make_text(FONT, ATTEMPTS_LOC2, attempts_txt, WHITE);
    asset_render(attempts_txt_asset);

    asset_t *attempts_context =
        asset_make_text(FONT, ATTEMPS_TEXT_LOC2, "Attempts:", WHITE);
    asset_render(attempts_context);

    char *coins_txt = malloc(sizeof(char) * (NUM_ATTEMPTS + 1));
    assert(coins_txt);
    snprintf(coins_txt, NUM_ATTEMPTS + 1, "%3lld", state->curr_coins);
    asset_t *coins_txt_asset =
        asset_make_text(FONT, COINS_LOC2, coins_txt, WHITE);
    asset_render(coins_txt_asset);

    asset_t *coins_context =
        asset_make_text(FONT, COINS_TEXT_LOC2, "Coins:", WHITE);
    asset_render(coins_context);
    free(coins_txt);
    asset_destroy(coins_txt_asset);
    asset_destroy(coins_context);
    free(attempts_txt);
    asset_destroy(attempts_txt_asset);
    asset_destroy(attempts_context);
    asset_render(state->button);
    stop_music();
  }

  scene_tick(state->scene, dt);
  off_floor(state);
  sdl_show();
  return false;
}

void emscripten_free(state_t *state) {
  TTF_Quit();
  scene_free(state->scene);
  list_free(state->body_assets);
  asset_cache_destroy();
  free(state);
}