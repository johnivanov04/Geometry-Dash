#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "body.h"
#include "polygon.h"

const double STARTING_ROT = 0.0;

struct body {
  polygon_t *poly;
  double mass;
  vector_t force;
  vector_t impulse;
  bool removed;
  void *info;
  free_func_t info_freer;
};

body_t *body_init_with_info(list_t *shape, double mass, rgb_color_t color,
                            void *info, free_func_t info_freer) {
  body_t *body = malloc(sizeof(body_t));
  assert(body != NULL);
  body->poly =
      polygon_init(shape, VEC_ZERO, STARTING_ROT, color.r, color.g, color.b);
  body->mass = mass;
  body->force = VEC_ZERO;
  body->impulse = VEC_ZERO;
  body->removed = false;
  body->info = info;
  body->info_freer = info_freer;
  return body;
}

body_t *body_init(list_t *shape, double mass, rgb_color_t color) {
  return body_init_with_info(shape, mass, color, NULL, NULL);
}

polygon_t *body_get_polygon(body_t *body) { return body->poly; }

void *body_get_info(body_t *body) { return body->info; }

void body_free(body_t *body) {
  if (body != NULL) {
    polygon_free(body->poly);
    if (body->info_freer != NULL && body->info != NULL) {
      body->info_freer(body->info);
    }
    free(body);
  }
}

list_t *body_get_shape(body_t *body) {
  list_t *polygon_points = polygon_get_points(body->poly);
  list_t *ret_version = list_init(list_size(polygon_points), (free_func_t)free);
  for (size_t i = 0; i < list_size(polygon_points); i++) {
    vector_t *prev = list_get(polygon_points, i);
    vector_t *copy_over = malloc(sizeof(vector_t));

    *copy_over = *prev;
    list_add(ret_version, copy_over);
  }
  return ret_version;
}

vector_t body_get_centroid(body_t *body) {
  return polygon_get_center(body->poly);
}

vector_t body_get_velocity(body_t *body) {
  double vx = polygon_get_velocity_x(body->poly);
  double vy = polygon_get_velocity_y(body->poly);

  return (vector_t){vx, vy};
}

rgb_color_t *body_get_color(body_t *body) {
  return polygon_get_color(body->poly);
}

void body_set_color(body_t *body, rgb_color_t *col) {
  polygon_set_color(body->poly, col);
}

void body_set_centroid(body_t *body, vector_t x) {
  polygon_set_center(body->poly, x);
}

void body_set_velocity(body_t *body, vector_t v) {
  polygon_set_velocity(body->poly, v.x, v.y);
}

double body_get_rotation(body_t *body) {
  return polygon_get_rotation(body->poly);
}

void body_set_rotation(body_t *body, double angle) {
  polygon_set_rotation(body->poly, angle);
}

void body_tick(body_t *body, double dt) {
  vector_t prev_v = body_get_velocity(body);
  vector_t force_v = vec_multiply((dt / body->mass), body->force);
  vector_t imp_v = vec_multiply((1 / body->mass), body->impulse);
  vector_t new_v = vec_add(force_v, imp_v);
  vector_t total_v = vec_add(prev_v, new_v);
  body_set_velocity(body, total_v);

  vector_t avg_v = vec_multiply(.5, vec_add(prev_v, total_v));
  vector_t prev_center = vec_multiply(dt, avg_v);
  vector_t new_center = vec_add(body_get_centroid(body), prev_center);
  body_set_centroid(body, new_center);

  body->force = VEC_ZERO;
  body->impulse = VEC_ZERO;
}

double body_get_mass(body_t *body) { return body->mass; }

void body_add_force(body_t *body, vector_t force) {
  body->force = vec_add(body->force, force);
}

void body_add_impulse(body_t *body, vector_t impulse) {
  body->impulse = vec_add(body->impulse, impulse);
}

void body_remove(body_t *body) { body->removed = true; }

bool body_is_removed(body_t *body) { return body->removed; }

void body_reset(body_t *body) {
  body->impulse = VEC_ZERO;
  body->force = VEC_ZERO;
}