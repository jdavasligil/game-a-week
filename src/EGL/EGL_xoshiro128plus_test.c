#include <EGL/EGL_testing.h>
#include <math.h>


#define N 1000000 // Sample Size
#define EPSILON 0.0000000001f // Tolerance for statistical tests.


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
		EGL_DECLARE_ERROR("T/F ratio (%.3f) differs from 0.5 by more than ε.", ratio);
	}
}

static void EGL_RandIntTest(EGL_Test *T) {
	EGL_DECLARE_TEST;

	uint32_t state[4] = {0,0,0,0};
	EGL_Seed(state, 0);
}


void EGL_Xoshiro128PlusTest(EGL_TestModule *M) {
	EGL_DECLARE_MODULE(EGL_random);

	EGL_RUN_TEST(EGL_RandBoolTest);
	EGL_RUN_TEST(EGL_RandIntTest);
}
