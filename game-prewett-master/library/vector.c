#include "vector.h"
#include <math.h>

const vector_t VEC_ZERO = {0, 0};

vector_t vec_add(vector_t v1, vector_t v2) {
  vector_t result = {v1.x + v2.x, v1.y + v2.y};
  return result;
}

vector_t vec_subtract(vector_t v1, vector_t v2) {
  vector_t result = {v1.x - v2.x, v1.y - v2.y};
  return result;
}

vector_t vec_negate(vector_t v) {
  vector_t result = {v.x * -1, v.y * -1};
  return result;
}

vector_t vec_multiply(double scalar, vector_t v) {
  vector_t result = {v.x * scalar, v.y * scalar};
  return result;
}

double vec_dot(vector_t v1, vector_t v2) {
  return (v1.x * v2.x) + (v1.y * v2.y);
}

double vec_cross(vector_t v1, vector_t v2) {
  return (v1.x * v2.y) - (v1.y * v2.x);
}

vector_t vec_rotate(vector_t v, double angle) {
  vector_t result = {(v.x * cos(angle)) - (v.y * sin(angle)),
                     (v.x * sin(angle)) + (v.y * cos(angle))};
  return result;
}

double vec_get_length(vector_t v) { return sqrt(pow(v.x, 2) + pow(v.y, 2)); }
