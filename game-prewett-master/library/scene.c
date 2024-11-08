#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "forces.h"
#include "scene.h"

const size_t BODIES_INIT = 0;
const size_t CAPACITY_INIT = 10;

struct scene {
  size_t num_bodies;
  list_t *bodies;
  size_t capacity;
  list_t *force_creator_list;
};

scene_t *scene_init(void) {
  scene_t *scene = malloc(sizeof(scene_t));
  assert(scene != NULL);

  scene->num_bodies = BODIES_INIT;
  scene->capacity = CAPACITY_INIT;
  scene->bodies = list_init(scene->capacity, (free_func_t)body_free);
  scene->force_creator_list =
      list_init(scene->capacity, (free_func_t)forcer_free);
  return scene;
}

void scene_free(scene_t *scene) {
  list_free(scene->bodies);
  list_free(scene->force_creator_list);
  free(scene);
}

size_t scene_bodies(scene_t *scene) { return scene->num_bodies; }

body_t *scene_get_body(scene_t *scene, size_t index) {
  return list_get(scene->bodies, index);
}

void scene_add_body(scene_t *scene, body_t *body) {
  list_add(scene->bodies, body);
  scene->num_bodies++;
}

void scene_remove_body(scene_t *scene, size_t index) {
  assert(index < list_size(scene->bodies));
  assert(index >= 0);
  body_remove(list_get(scene->bodies, index));
}

void scene_tick(scene_t *scene, double dt) {
  for (size_t i = 0; i < list_size(scene->force_creator_list); i++) {
    forcer_t *force = list_get(scene->force_creator_list, i);
    if (force && force->creator) {
      force->creator(force->aux);
    }
  }

  for (ssize_t i = 0; i < (ssize_t)scene->num_bodies; i++) {
    body_t *current_body = list_get(scene->bodies, i);
    if (body_is_removed(current_body)) {
      ssize_t f_length = list_size(scene->force_creator_list);
      for (ssize_t j = 0; j < f_length; j++) {
        forcer_t *force = list_get(scene->force_creator_list, j);

        ssize_t body_length = list_size(force->bodies);
        for (ssize_t k = 0; k < body_length; k++) {
          body_t *current_b = list_get(force->bodies, k);

          if (current_b == current_body) {
            list_remove(scene->force_creator_list, j);
            forcer_free(force);
            f_length--;
            j--;
            break;
          }
        }
      }
      list_remove(scene->bodies, i);
      body_free(current_body);
      scene->num_bodies--;
      i--;
    } else {
      body_tick(current_body, dt);
    }
  }
}

void scene_add_force_creator(scene_t *scene, force_creator_t force_creator,
                             void *aux) {
  scene_add_bodies_force_creator(scene, force_creator, aux, list_init(0, free));
}

void scene_add_bodies_force_creator(scene_t *scene, force_creator_t forcer,
                                    void *aux, list_t *bodies) {
  forcer_t *new_forcer = forcer_init(forcer, aux, bodies);
  list_add(scene->force_creator_list, new_forcer);
}
