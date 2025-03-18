/*  Written in 2018 by David Blackman and Sebastiano Vigna (vigna@acm.org)
    Modified in 2025 by Jaedin Davasligil (jdavasligil.dev@proton.me)

To the extent possible under law, the author has dedicated all copyright
and related and neighboring rights to this software to the public domain
worldwide.

Permission to use, copy, modify, and/or distribute this software for any
purpose with or without fee is hereby granted.

THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR
IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE. */

#include <EGL/EGL_random.h>

/* This is xoshiro128+ 1.0, our best and fastest 32-bit generator for 32-bit
   floating-point numbers. We suggest to use its upper bits for
   floating-point generation, as it is slightly faster than xoshiro128**.
   It passes all tests we are aware of except for
   linearity tests, as the lowest four bits have low linear complexity, so
   if low linear complexity is not considered an issue (as it is usually
   the case) it can be used to generate 32-bit outputs, too.

   We suggest to use a sign test to extract a random Boolean value, and
   right shifts to extract subsets of bits.

   The state must be seeded so that it is not everywhere zero. */


static inline uint32_t rotl(const uint32_t x, int k) {
	return (x << k) | (x >> (32 - k));
}

static inline double to_double(uint64_t x) {
    const union { uint64_t i; double d; } u = {.i = UINT64_C(0x3FF) << 52 | x >> 12 };
    return u.d - 1.0;
}

static inline float to_float(uint32_t x) {
    const union { uint32_t i; float f; } u = {.i = UINT32_C(0x7F) << 23 | ((uint32_t)x) >> 9 };
    return u.f - 1.0;
}

static inline uint32_t next(uint32_t *state) {
	const uint32_t result = state[0] + state[3];

	const uint32_t t = state[1] << 9;

	state[2] ^= state[0];
	state[3] ^= state[1];
	state[1] ^= state[2];
	state[0] ^= state[3];

	state[2] ^= t;

	state[3] = rotl(state[3], 11);

	return result;
}


/* This is the jump function for the generator. It is equivalent
   to 2^64 calls to next(); it can be used to generate 2^64
   non-overlapping subsequences for parallel computations. */

//static void jump(uint32_t *state) {
//	static const uint32_t JUMP[] = { 0x8764000b, 0xf542d2d3, 0x6fa035c3, 0x77f2db5b };
//
//	uint32_t s0 = 0;
//	uint32_t s1 = 0;
//	uint32_t s2 = 0;
//	uint32_t s3 = 0;
//	for(unsigned long i = 0; i < sizeof JUMP / sizeof *JUMP; i++)
//		for(unsigned int b = 0; b < 32; b++) {
//			if (JUMP[i] & UINT32_C(1) << b) {
//				s0 ^= state[0];
//				s1 ^= state[1];
//				s2 ^= state[2];
//				s3 ^= state[3];
//			}
//			next(state);	
//		}
//		
//	state[0] = s0;
//	state[1] = s1;
//	state[2] = s2;
//	state[3] = s3;
//}


/* This is the long-jump function for the generator. It is equivalent to
   2^96 calls to next(); it can be used to generate 2^32 starting points,
   from each of which jump() will generate 2^32 non-overlapping
   subsequences for parallel distributed computations. */

//static void long_jump(uint32_t *state) {
//	static const uint32_t LONG_JUMP[] = { 0xb523952e, 0x0b6f099f, 0xccf5a0ef, 0x1c580662 };
//
//	uint32_t s0 = 0;
//	uint32_t s1 = 0;
//	uint32_t s2 = 0;
//	uint32_t s3 = 0;
//	for(unsigned long i = 0; i < sizeof LONG_JUMP / sizeof *LONG_JUMP; i++)
//		for(unsigned int b = 0; b < 32; b++) {
//			if (LONG_JUMP[i] & UINT32_C(1) << b) {
//				s0 ^= state[0];
//				s1 ^= state[1];
//				s2 ^= state[2];
//				s3 ^= state[3];
//			}
//			next(state);	
//		}
//		
//	state[0] = s0;
//	state[1] = s1;
//	state[2] = s2;
//	state[3] = s3;
//}

void EGL_Seed(uint32_t *state, uint32_t seed) {
	for (uint32_t i = 1; i < 624; i++) {
		seed = UINT32_C(1812433253) * (seed ^ (seed >> 30)) + i; //// Knuth TAOCP Vol2. 3rd Ed. P.106 for multiplier.
	}
	state[0] = UINT32_C(0x3954c094) ^ seed;
	state[1] = UINT32_C(0x30a56abb) ^ seed;
	state[2] = UINT32_C(0x1d311568) ^ seed;
	state[3] = UINT32_C(0x39adfa64) ^ seed;
}

uint32_t EGL_RandNext(uint32_t *state) {
	return next(state);
}

bool EGL_RandBool(uint32_t *state) {
	return (next(state) >> 31) == 0;
}

float EGL_RandFloat(uint32_t *state) {
	return to_float(next(state));
}

double EGL_RandDouble(uint32_t *state) {
	next(state);
	return to_double(*((uint64_t *)state));
}

int EGL_RandInt(uint32_t *state, int a, int b) {
	return (int)( a + (b - a) * to_float(next(state)) );
}
