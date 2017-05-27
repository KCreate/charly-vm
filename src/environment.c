#include "environment.h"

/*
 * Allocate and initialize a new environment
 *
 * @param size_t size - The amount of values the environment should hold
 * @return ch_environment*
 * */
ch_environment* ch_environment_create(size_t size) {

  // Allocate space for the environment
  ch_environment* struct_environment = malloc(sizeof(ch_environment));
  if (!struct_environment) return NULL;

  // Initialize
  if (!ch_environment_init(struct_environment, size)) {
    free(struct_environment);
    return NULL;
  }

  return struct_environment;
}

/*
 * Initialize a new environment
 *
 * @param ch_environment* environment
 * @param size_t size
 * @return ch_environment* - The pointer that was passed to the function or NULL
 * */
ch_environment* ch_environment_init(ch_environment* environment, size_t size) {

  // Allocate space for the values
  ch_value* value_buffer = malloc(size * sizeof(ch_environment));
  if (!value_buffer) return NULL;

  // Initialize struct
  environment->values = value_buffer;
  environment->size = size;

  return environment;
}
