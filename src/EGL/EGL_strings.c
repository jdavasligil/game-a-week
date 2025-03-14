#include <EGL/EGL_strings.h>

extern int EGL_ReadLine(Reader *r, char *dst, size_t maxlen) {
	if (NULL == r || NULL == dst) {
		return -1;
	} else if (r->offset >= r->size) {
		return -2;
	} else if (maxlen == 0) {
		return 0;
	}

	char *head = (r->data + r->offset);
	int bytes_written = 0;

	while (r->offset < r->size && bytes_written < ((int)maxlen - 1) && *head != '\n' && *head != '\0') {
		*dst = *head;
		++dst;
		++head;
		++bytes_written;
		++r->offset;
	}
	*dst = '\0';
	++r->offset;

	return bytes_written;
}
