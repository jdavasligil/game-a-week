#ifndef WORLD_H
#define WORLD_H


#include <SDL3/SDL_stdinc.h>
#include <stdint.h>
#include <cglm/mat4.h>
#include <EGL/EGL_3d.h>


typedef struct {
	uint32_t indices_size;
	uint32_t vertices_size;
	uint32_t normals_size;
	uint32_t uvs_size;
	uint32_t index_count;
	uint32_t vertex_count;
	uint32_t normal_count;
	uint32_t uv_count;
	uint32_t *indices;
	float *vertices; // vec3: [(x,y,z)(x,y,z)...]
	float *normals;  // vec3: [(x,y,z)(x,y,z)...]
	float *uvs;      // vec2: [(u,v)(u,v)(u,v)...]

	Transform transform;
	Transform render_transform;
} World;

static inline void World_Deserialize(World *w, char *data) {
	int offset = 0;

	size_t indices_size = *(uint32_t *)(data + offset);
	offset += 4;
	w->indices = (uint32_t *)SDL_malloc(indices_size);
	SDL_memcpy((void *)w->indices, (void *)(data + offset), indices_size);
	offset += indices_size;

	size_t vertices_size = *(uint32_t *)(data + offset);
	offset += 4;
	w->vertices = (float *)SDL_malloc(vertices_size);
	SDL_memcpy((void *)w->vertices, (void *)(data + offset), vertices_size);
	offset += vertices_size;

	size_t normals_size = *(uint32_t *)(data + offset);
	offset += 4;
	w->normals = (float *)SDL_malloc(normals_size);
	SDL_memcpy((void *)w->normals, (void *)(data + offset), normals_size);
	offset += normals_size;

	size_t uvs_size = *(uint32_t *)(data + offset);
	offset += 4;
	w->uvs = (float *)SDL_malloc(uvs_size);
	SDL_memcpy((void *)w->uvs, (void *)(data + offset), uvs_size);

	w->indices_size = indices_size;
	w->vertices_size = vertices_size;
	w->normals_size = normals_size;
	w->uvs_size = uvs_size;

	w->index_count = indices_size / 4;    // 4 bytes
	w->vertex_count = vertices_size / 12; // 4 bytes per 3 coordinates
	w->normal_count = normals_size / 12;  // 4 bytes per 3 coordinates
	w->uv_count = uvs_size / 8;           // 4 bytes per 2 coordinates
}


#endif // WORLD_H
