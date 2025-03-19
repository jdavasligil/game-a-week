#ifndef EGL_TESTING_H
#define EGL_TESTING_H


#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>


#define VERBOSE_TEST // Uncomment for verbose tests.

#define LOG_BUFFER_MAX 4096
#define ERRORS_MAX 64
#define TESTS_MAX 256
#define ERROR_MAX 128
#define LABEL_MAX 128


// ANSI TERMINAL ESCAPE CODE:  "\033[#m" (where # is a number)
#define ANSI_RED(text)     ("\033[31m" text)
#define ANSI_GREEN(text)   ("\033[32m" text)
#define ANSI_YELLOW(text)  ("\033[33m" text)
#define ANSI_BLUE(text)    ("\033[34m" text)
#define ANSI_MAGENTA(text) ("\033[35m" text)
#define ANSI_CYAN(text)    ("\033[36m" text)
#define ANSI_RESET         ("\033[0m")

#define HLINE "-------------------------------------------------------------------------------\n"


/* Testing API Macros */

/**
 * Declare the current set of tests as part of the same module.
 *
 * @param m the title of the module (no string quotes necessary).
 */
#define EGL_DECLARE_MODULE(m)\
	M->filename[0] = '\0';\
	snprintf(M->filename, LABEL_MAX, __FILE_NAME__);\
	M->module[0] = '\0';\
	snprintf(M->module, LABEL_MAX, "%s", #m);\
	M->test_count = 0

/**
 * Declare the current function as a test.
 */
#define EGL_DECLARE_TEST\
	snprintf(T->testname, LABEL_MAX, __func__);\
	T->error_count = 0

/**
 * Declare a test error to be logged and reported.
 *
 * The length of the formatted string must not exceed ERROR_MAX (including
 * the null terminator). If longer errors are necessary, increase the max.
 *
 * @param format A printf style format string.
 */
#define EGL_DECLARE_ERROR(format, ...)\
	T->errors[T->error_count].line = __LINE__;\
	T->errors[T->error_count].error[0] = '\0';\
	snprintf(T->errors[T->error_count].error, ERROR_MAX, format, ##__VA_ARGS__);\
	T->error_count += 1

#define EGL_RUN_TEST(t)\
	if (M->test_count >= TESTS_MAX) {\
		fprintf(stderr, "\033[31mFATAL\033[35m%d\033[0m    Test count has exceeded the max %d\n", __LINE__, TESTS_MAX);\
		exit(EXIT_FAILURE);\
	}\
	M->begin = clock();\
	t(&(M->tests[M->test_count]));\
	M->end = clock();\
	M->tests[M->test_count].time = (double)(M->end - M->begin) / CLOCKS_PER_SEC;\
	M->test_count++

#define EGL_RUN_MODULE(m)\
	m(&M);\
	EGL_LogModule(&M, &L)


typedef struct {
	char  *buffer;
	size_t size;
	size_t capacity;
} EGL_TestLog;

typedef struct {
	int line;
	char error[ERROR_MAX];
} EGL_TestError;

typedef struct {
	char testname[LABEL_MAX];
	EGL_TestError errors[ERRORS_MAX];
	int error_count;
	double time;
} EGL_Test;

typedef struct {
	char filename[LABEL_MAX];
	char module[LABEL_MAX];
	EGL_Test tests[TESTS_MAX];
	int test_count;
	int fail_count;
	clock_t begin;
	clock_t end;
} EGL_TestModule;


static bool TESTS_FAILING = false;

static inline void EGL_LogModule(EGL_TestModule *M, EGL_TestLog *L) {
	if (!L->buffer) {
		fprintf(stderr, "\033[31mFATAL\033[35m%d\033[0m    NULL log buffer.\n", __LINE__);
		exit(EXIT_FAILURE);
	} else if (L->size >= L->capacity) {
		fprintf(stderr, "\033[31mFATAL\033[35m%d\033[0m    Logger out of memory.\n", __LINE__);
		exit(EXIT_FAILURE);
	}
	
	/* Double memory if remaining memory is less than half the original max */
	if (L->capacity - L->size < (L->capacity >> 1)) {
		L->capacity <<= 1;
		L->buffer = (char *)realloc(L->buffer, L->capacity);
	}

	int total_errors = 0;
	double total_time = 0;
	int bytes_written = 0;

	char *head = L->buffer + L->size;
	for (int i = 0; i < M->test_count; i++) {
		if (M->tests[i].error_count > 0) {
			M->fail_count++;
		}
		total_errors += M->tests[i].error_count;
		total_time += M->tests[i].time;
	}

	/* It is 'safe' not to check bounds here since bytes >= 2048. */
	if (total_errors == 0) {
		memcpy(head, ANSI_GREEN(HLINE), sizeof(ANSI_GREEN(HLINE)));
		head += sizeof(ANSI_GREEN(HLINE)) - 1;
		bytes_written = snprintf(head, L->capacity - L->size, " PASS | %s (%.3fs)\n", M->module, total_time);
		if (bytes_written < 0) {
			fprintf(stderr, "\033[31mFATAL\033[35m%d\033[0m    Encoding error with snprintf.\n", __LINE__);
			exit(EXIT_FAILURE);
		}
		head += bytes_written;
		memcpy(head, HLINE, sizeof(HLINE));
		head += sizeof(HLINE) - 1;
		L->size += sizeof(ANSI_GREEN(HLINE)) + sizeof(HLINE) + bytes_written - 2;
	} else {
		TESTS_FAILING = true;
		memcpy(head, ANSI_RED(HLINE), sizeof(ANSI_RED(HLINE)));
		head += sizeof(ANSI_RED(HLINE)) - 1;
		bytes_written = snprintf(head, L->capacity - L->size, " FAIL | %s (%.3fs)\n", M->module, total_time);
		if (bytes_written < 0) {
			fprintf(stderr, "\033[31mFATAL\033[35m%d\033[0m    Encoding error with snprintf.\n", __LINE__);
			exit(EXIT_FAILURE);
		}
		head += bytes_written;
		memcpy(head, HLINE, sizeof(HLINE));
		head += sizeof(HLINE) - 1;
		L->size += sizeof(ANSI_RED(HLINE)) + sizeof(HLINE) + bytes_written - 2;
	}

#ifdef VERBOSE_TEST
	for (int i = 0; i < M->test_count; i++) {
		if (L->capacity - L->size < (L->capacity >> 1)) {
			L->capacity <<= 1;
			L->buffer = (char *)realloc(L->buffer, L->capacity);
		}
		EGL_Test *t = &M->tests[i];
		if (t->error_count == 0) {
			bytes_written = snprintf(head, L->capacity - L->size, ANSI_GREEN(" PASS | %s (%.3fs)\n"), t->testname, t->time);
			if (bytes_written < 0) {
				fprintf(stderr, "\033[31mFATAL\033[35m%d\033[0m    Encoding error with snprintf.\n", __LINE__);
				exit(EXIT_FAILURE);
			}
			head += bytes_written;
			L->size += bytes_written;
		} else {
			bytes_written = snprintf(head, L->capacity - L->size, ANSI_RED(" FAIL | %s (%.3fs)\n"), t->testname, t->time);
			if (bytes_written < 0) {
				fprintf(stderr, "\033[31mFATAL\033[35m%d\033[0m    Encoding error with snprintf.\n", __LINE__);
				exit(EXIT_FAILURE);
			}
			head += bytes_written;
			L->size += bytes_written;
			for (int j = 0; j < t->error_count; j++) {
				EGL_TestError *e = &t->errors[j];
				if (L->capacity - L->size < (L->capacity >> 1)) {
					L->capacity <<= 1;
					L->buffer = (char *)realloc(L->buffer, L->capacity);
				}
				bytes_written = snprintf(head, L->capacity - L->size, "      |     Trace:    \033[36m%s\033[35m:%d\033[31m\n", M->filename, e->line);
				if (bytes_written < 0) {
					fprintf(stderr, "\033[31mFATAL\033[35m%d\033[0m    Encoding error with snprintf.\n", __LINE__);
					exit(EXIT_FAILURE);
				}
				head += bytes_written;
				L->size += bytes_written;

				bytes_written = snprintf(head, L->capacity - L->size, "      |     Error:    %s\n", e->error);
				if (bytes_written < 0) {
					fprintf(stderr, "\033[31mFATAL\033[35m%d\033[0m    Encoding error with snprintf.\n", __LINE__);
					exit(EXIT_FAILURE);
				}
				head += bytes_written;
				L->size += bytes_written;
			}
		}
	}
#endif
}


/*$ HEADERS */
#include <EGL/EGL_random.h>
/*$ END HEADERS */


/*$ TESTS */
void EGL_Xoshiro128PlusTest(EGL_TestModule *M);
/*$ END TESTS */

#endif /* EGL_TESTING_H */
