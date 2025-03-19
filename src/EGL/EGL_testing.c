#include <EGL/EGL_testing.h>


int main(int argc, char **argv)
{
	EGL_TestModule M;
	EGL_TestLog L = {
		.buffer = (char *)malloc(sizeof(char) * LOG_BUFFER_MAX),
		.size = 0,
		.capacity = LOG_BUFFER_MAX
	};
	M.fail_count = 0;

	/*$ TESTS */
	EGL_RUN_MODULE(EGL_Xoshiro128PlusTest);
	/*$ END TESTS */

	printf("\n\033[0m TEST | ");
	if (TESTS_FAILING) {
		printf("\033[31m%d/%d \033[0mPASSING\n", M.test_count - M.fail_count, M.test_count);
		memcpy(L.buffer + L.size, ANSI_RED(HLINE), sizeof(ANSI_RED(HLINE)));
		L.size += sizeof(ANSI_RED(HLINE));
	} else {
		printf("\033[32m%d/%d \033[0mPASSING\n", M.test_count - M.fail_count, M.test_count);
		memcpy(L.buffer + L.size, ANSI_GREEN(HLINE), sizeof(ANSI_GREEN(HLINE)));
		L.size += sizeof(ANSI_GREEN(HLINE));
	}

	printf("%s%s\n", L.buffer, ANSI_RESET);

	free(L.buffer);

	return 0;
}
