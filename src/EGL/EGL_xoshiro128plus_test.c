#include <EGL/EGL_testing.h>
#include <math.h>


#define N 10000 // Sample Size (changing this value affects all tests!!)
#define ONE_N 0.0001f // 1 / N
#define EPSILON 0.01f // Tolerance for statistical tests (𝛼 of .05).
#define CHI_SQUARE 16.919f // Threshold for chi square test (𝛼 of .05).


static void EGL_RandBoolTest(EGL_Test *T) {
	EGL_DECLARE_TEST;

	uint32_t state[4] = {0,0,0,0};
	EGL_Seed(state, 0);

	float count = 0.0f;
	for (int i = 0; i < N; i++) {
		count += EGL_RandBool(state);
	}

	float ratio = (count / N);
	float delta = fabsf(ratio - 0.5f);
	if (delta > EPSILON) {
		EGL_DECLARE_ERROR("T/F ratio (%.3f) differs from 0.5 by more than %.2f.", ratio, EPSILON);
	}
}

/**
 * Chi-square goodness of fit test for discrete uniform distribution [0,10)
 * H_0: EGL_RandInt produces data representative of a uniform distribution.
 * H_a: EGL_RandInt does not produce representative data.
 */
static void EGL_RandIntTest(EGL_Test *T) {
	EGL_DECLARE_TEST;

	uint32_t state[4] = {0,0,0,0};
	EGL_Seed(state, 0);

	int digits[10] = {0,0,0,0,0,0,0,0,0,0};
	int randint = 0;

	for (int i = 0; i < N; i++) {
		randint = EGL_RandInt(state, 0, 10);
		if (randint < 0 || randint >= 10) {
			EGL_DECLARE_ERROR("Random integer %d out of bounds [0,10).", randint);
			return;
		}
		digits[randint]++;
	}
	float chi_square = 0.0f;
	float freq_diff = 0.0f;
	for (int i = 0; i < 10; i++) {
		freq_diff = digits[i] * ONE_N - 0.1f;
		chi_square += freq_diff * freq_diff * 10.0f;
	}
	if (chi_square > CHI_SQUARE) {
		EGL_DECLARE_ERROR("Random integers are not uniformly distributed: %.3f > %.3f.", chi_square, CHI_SQUARE);
	}
}


void EGL_Xoshiro128PlusTest(EGL_TestModule *M) {
	EGL_DECLARE_MODULE(EGL_random);

	EGL_RUN_TEST(EGL_RandBoolTest);
	EGL_RUN_TEST(EGL_RandIntTest);
}
