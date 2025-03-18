#include <EGL/EGL_testing.h>


int main(int argc, char **argv)
{
	EGL_TestModule M;
	EGL_TestLog L = {
		.buffer = (char *)malloc(sizeof(char) * LOG_BUFFER_MAX),
		.size = 0,
		.capacity = LOG_BUFFER_MAX
	};

	printf("\nTESTS\n");

	/*$ TESTS */
	EGL_RUN_MODULE(EGL_Xoshiro128PlusTest);
	/*$ END TESTS */

	if (TESTS_FAILING) {
		memcpy(L.buffer + L.size, ANSI_RED(HLINE), sizeof(ANSI_RED(HLINE)));
		L.size += sizeof(ANSI_RED(HLINE));
	} else {
		memcpy(L.buffer + L.size, ANSI_GREEN(HLINE), sizeof(ANSI_GREEN(HLINE)));
		L.size += sizeof(ANSI_GREEN(HLINE));
	}

	printf("%s%s\n", L.buffer, ANSI_RESET);

	free(L.buffer);

	return 0;
}
