#ifndef EGL_3D_H
#define EGL_3D_H


#include <SDL3/SDL_stdinc.h>
#include <SDL3/SDL_log.h>
#include <cglm/cglm.h>


typedef versor quat;


typedef struct {
	vec4 scale;
	quat rotation;
	union {
		vec4 translation;
		struct {
			float tx;
			float ty;
			float tz;
			float tw;
		};
	};
	mat4 model;

} Transform;


static inline void EGL_TransformNew(Transform *t) {
	glm_vec4_one(t->scale);
	glm_quat_identity(t->rotation);
	glm_vec4_zero(t->translation);
	glm_mat4_identity(t->model);
}

static inline void EGL_TransformUpdate(Transform *t) {
	glm_mat4_identity(t->model);
	glm_scale(t->model, t->scale);
	glm_quat_rotate(t->model, t->rotation, t->model);
	glm_vec4_copy(t->translation, t->model[3]);
}

static inline void EGL_TransformCopy(Transform *src, Transform *dest) {
	SDL_memcpy((void *)dest, (void *)src, sizeof(Transform));
}

static inline void EGL_TransformRotate(Transform *t, float angle, vec3 axis) {
	glm_rotate(t->model, angle, axis);
	glm_mat4_quat(t->model, t->rotation);
	glm_vec4_copy(t->translation, t->model[3]);
}

static inline void EGL_TransformPrint(Transform *t) {
	char *log = (char *)SDL_malloc(sizeof(char) * 1024);
	char line[64];

	line[0] = '\0';
	log[0] = '\0';

	SDL_snprintf(line, sizeof(char) * 64, "Rotation:    [%.2f, %.2f, %.2f, %.2f]\n", t->rotation[0], t->rotation[1], t->rotation[2], t->rotation[3]);
	SDL_strlcat(log, line, sizeof(char) * 1024);

	SDL_snprintf(line, sizeof(char) * 64, "Scale:       [%.2f, %.2f, %.2f, %.2f]\n", t->scale[0], t->scale[1], t->scale[2], t->scale[3]);
	SDL_strlcat(log, line, sizeof(char) * 1024);

	SDL_snprintf(line, sizeof(char) * 64, "Translation: [%.2f, %.2f, %.2f, %.2f]\n", t->translation[0], t->translation[1], t->translation[2], t->translation[3]);
	SDL_strlcat(log, line, sizeof(char) * 1024);

	SDL_snprintf(line, sizeof(char) * 64, "Model: \n");
	SDL_strlcat(log, line, sizeof(char) * 1024);

	for (int r = 0; r < 4; r++) {
		SDL_strlcat(log, "\t", sizeof(char) * 1024);
		for (int c = 0; c < 4; c++) {
			SDL_snprintf(line, sizeof(char) * 64, "%.2f ", t->model[c][r]);
			SDL_strlcat(log, line, sizeof(char) * 1024);
		}
		SDL_strlcat(log, "\n", sizeof(char) * 1024);
	}

	SDL_Log("%s", log);
	SDL_free(log);
}


#endif // EGL_3D_H
