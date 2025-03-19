#include <EGL/EGL_testing.h>
#include <math.h>
#include <stdlib.h>


#define N 10000 // Sample Size (changing this value affects all tests!!)
#define ONE_N 0.0001f // 1 / N
#define EPSILON 0.01f // Tolerance for statistical tests (𝛼 of .05).
#define KS_STATISTIC 0.013581f // KS-statistic cutoff for statistical tests (𝛼 of .05).
#define CHI_SQUARE 16.919f // Threshold for chi square test (𝛼 of .05).


static inline int compare_float(const void* a, const void* b) {
	return (*(float *)a - *(float *)b);
}


/**
 * Simple proportion test for fairness.
 * H_0: EGL_RandBool produces an equal proportion of true and false (0.5).
 * H_a: EGL_RandBool does not produce an equal proportion.
 * Reject if |p - 0.5| > 0.01 at 𝛼 = .05.
 */
static void EGL_RandBoolTest(EGL_Test *T) {
	EGL_DECLARE_TEST;

	uint32_t state[4] = {0,0,0,0};
	EGL_Seed(state, 0);

	float count = 0.0f;
	for (int i = 0; i < N; i++) {
		count += EGL_RandBool(state);
	}

	float ratio = (count / N);
	if (fabsf(ratio - 0.5f) > EPSILON) {
		EGL_DECLARE_ERROR("T/F ratio (%.3f) differs from 0.5 by more than %.2f.", ratio, EPSILON);
	}
}

/**
 * Chi-square Goodness-of-Fit Test for Random Integers on [0,10).
 * H_0: EGL_RandInt produces data representative of a uniform distribution.
 * H_a: EGL_RandInt does not produce representative data.
 * Reject if X^2 > CHI_SQUARE at 𝛼 = .05.
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

/**
 * Kolmogorov-Smirnov Goodness-of-Fit Test for Random Floats on [0,1).
 * H_0: EGL_RandFloat produces data representative of a uniform distribution.
 * H_a: EGL_RandFloat does not produce representative data.
 * Reject if D_n = sup_x |F_n(x) - F_0(x)| > KS_STATISTIC at 𝛼 = .05.
 *
 * F_0(x_i) = x_i for x_i in [0,1)
 * F_n(x_i) = i/N for x_i in [0,1)
 */
static void EGL_RandFloatTest(EGL_Test *T) {
	EGL_DECLARE_TEST;

	uint32_t state[4] = {0,0,0,0};
	EGL_Seed(state, 0);

	float randfloat = 0.0f;
	float *observations = (float *)malloc(sizeof(float) * (N + 1));
	observations[0] = 0.0f;

	for (int i = 1; i <= N; i++) {
		randfloat = EGL_RandFloat(state);
		if (randfloat < 0.0f || randfloat >= 1.0f) {
			EGL_DECLARE_ERROR("Random float %.3f out of bounds [0,1).", randfloat);
			return;
		}
		observations[i] = randfloat;
	}

	qsort(observations, N + 1, sizeof(float), compare_float);

	float maxdiff = 0.0f;
	float diff = 0.0f;
	float F_zero = 0.0f;
	float F_n_prev = 0.0f;
	float F_n = 0.0f;
	for (int i = 1; i <= N; i++) {
		F_zero = observations[i];
		F_n = i * ONE_N;
		diff = fabsf(fabsf(F_n_prev - F_zero) - fabsf(F_zero - F_n));
		maxdiff = (diff > maxdiff) ? diff : maxdiff;
		F_n_prev = F_n;
	}
	if (maxdiff > KS_STATISTIC) {
		EGL_DECLARE_ERROR("Random floats are not uniformly distributed: %.3f > %.3f.", maxdiff, KS_STATISTIC);
	}
	free(observations);
}


void EGL_Xoshiro128PlusTest(EGL_TestModule *M) {
	EGL_DECLARE_MODULE(EGL_random);

	EGL_RUN_TEST(EGL_RandBoolTest);
	EGL_RUN_TEST(EGL_RandIntTest);
	EGL_RUN_TEST(EGL_RandFloatTest);
}
