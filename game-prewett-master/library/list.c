#include "list.h"
#include <assert.h>
#include <math.h>
#include <stdlib.h>

typedef struct list {

  size_t length;
  size_t capacity;
  void **data;
  free_func_t freer;
} list_t;

list_t *list_init(size_t initial_capacity, free_func_t freer) {
  list_t *list = malloc(sizeof(list_t));
  assert(list != NULL);
  list->capacity = initial_capacity;
  list->length = 0;
  list->data = calloc(initial_capacity, sizeof(void *));
  list->freer = freer;

  return list;
}

void list_free(list_t *list) {
  if (list->freer) {
    for (size_t i = 0; i < list->length; i++) {
      list->freer(list->data[i]);
    }
  }
  free(list->data);
  free(list);
}

size_t list_size(list_t *list) {
  if (list == NULL) {
    return 0;
  }
  return list->length;
}

void *list_get(list_t *list, size_t index) {

  assert(index < list_size(list));
  assert(index >= 0);
  assert(index < list->length);

  void *ret = list->data[index];

  return ret;
}

void list_add(list_t *list, void *value) {
  assert(value != NULL);
  if (list->length >= list->capacity) {
    void **new_data = realloc(list->data, list->capacity * 2 * sizeof(void *));
    assert(new_data != NULL);

    list->capacity *= 2;
    list->data = new_data;
  }

  list->data[list->length] = value;
  list->length++;
}

void *list_remove(list_t *list, size_t index) {
  assert(index < list->length);
  void *removed_value = list->data[index];

  for (size_t i = index; i < list->length - 1; i++) {
    list->data[i] = list->data[i + 1];
  }
  list->length--;

  return removed_value;
}

void list_set(list_t *list, size_t index, void *value) {
  assert(index < list_size(list));
  list->data[index] = value;
}