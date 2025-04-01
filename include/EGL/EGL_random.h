/**
 * @file EGL_random.h
 * @brief Pseudo-random number generation routines.
 */

#ifndef EGL_RANDOM_H
#define EGL_RANDOM_H


#include <stdint.h>
#include <stdbool.h>


/**
 * Seed the given random number generator with the seed provided.
 *
 * @param state Pointer to the PRNG state buffer (SIZE MUST = 4).
 * @param seed the number used to generate a unique, deterministic sequence.
 */
void EGL_Seed(uint32_t *state, uint32_t seed);

/**
 * Get the next random 32 bits as an unsigned int.
 *
 * @param state Pointer to the PRNG state buffer (SIZE MUST = 4).
 * @return 32 random bits as an unsigned int.
 */
uint32_t EGL_RandNext(uint32_t *state);

/**
 * Generate a uniform random bool (true or false).
 *
 * @param state Pointer to the PRNG state buffer (SIZE MUST = 4).
 * @return True or false.
 */
bool EGL_RandBool(uint32_t *state);

/**
 * Generate a uniform random 32 bit float in the interval [0,1).
 *
 * @param state Pointer to the PRNG state buffer (SIZE MUST = 4).
 * @return Random float in [0,1).
 */
float EGL_RandFloat(uint32_t *state);

/**
 * Generate a uniform random 64 bit double in the interval [0,1).
 *
 * @param state Pointer to the PRNG state buffer (SIZE MUST = 4).
 * @return Random double in [0,1).
 */
double EGL_RandDouble(uint32_t *state);

/**
 * Generate a uniform random 32 bit integer in the interval [a,b).
 *
 * @param state Pointer to the PRNG state buffer (SIZE MUST = 4).
 * @return Random integer in [a,b).
 */
int EGL_RandInt(uint32_t *state, int a, int b);


#endif /* EGL_RANDOM_H */
