/**
 * @file EGL_strings.h
 * @brief Byte and string manipulation routines.
 */

#ifndef EGL_STRINGS_H
#define EGL_STRINGS_H

#include <stddef.h>

/** A simple byte reader to keep track of offset and size. */
typedef struct {
	char *data;
	size_t offset;
	size_t size;
} Reader;

/**
 * Read a single line from the reader and write into the destination string.
 *
 * While some C runtimes differ on how to deal with too-large strings, this
 * function null-terminates the output, by treating the null-terminator as
 * part of the `maxlen` count. Note that if `maxlen` is zero, however, no
 * bytes will be written at all.
 *
 * This function returns the number of _bytes_ (not _characters_) that should
 * be written, excluding the null-terminator character. If this returns a
 * number >= `maxlen`, it means the output string was truncated. A negative
 * return value means an error occurred.
 *
 * @param r the reader containing the data to read the next line from.
 * @param dst the buffer to write the line into. Must not be NULL.
 * @param maxlen the maximum bytes to write, including the null-terminator.
 *
 * @threadsafety It is safe to call this function from any thread.
 */
extern int EGL_ReadLine(Reader *r, char *dst, size_t maxlen);

#endif //EGL_STRINGS_H
