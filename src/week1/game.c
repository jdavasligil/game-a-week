/*
 * Florbles: Alien Invasion!
 * A planetary tower defense game.
 */
#define SDL_MAIN_USE_CALLBACKS 1


#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include <stdint.h>


#define PATH_MAX 4096

#define WINDOW_WIDTH  800
#define WINDOW_HEIGHT 800

#define STRIDE 32


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
	float *vertices; // jump 3: [(x,y,z)(x,y,z)...]
	float *normals;  // jump 3: [(x,y,z)(x,y,z)...]
	float *uvs;      // jump 2: [(u,v)(u,v)(u,v)...]
} World;

typedef struct {
	SDL_Window *window;
	SDL_Renderer *renderer;

	World world;

	Uint64 total_time;
	Uint64 physics_time;
	Uint64 prev_tick;
} AppState;


SDL_AppResult SDL_AppInit(void **appstate, int argc, char *argv[])
{
	char *path = (char *) SDL_malloc(PATH_MAX * sizeof(char));

	AppState *ctx = (AppState *)SDL_calloc(1, sizeof(AppState));
	if (!ctx) {
		return SDL_APP_FAILURE;
	}
	*appstate = ctx;

	/* Initialize App */
    SDL_SetAppMetadata("Florbles: Alien Invasion!", "0.0.0a", "com.florbles");

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("Couldn't initialize SDL: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

	/* Initialize Window */
    if (!SDL_CreateWindowAndRenderer("Florbles: Alien Invasion!", WINDOW_WIDTH, WINDOW_HEIGHT, 0, &ctx->window, &ctx->renderer)) {
        SDL_Log("Couldn't create window/renderer: %s\n", SDL_GetError());
        return SDL_APP_FAILURE;
    }

	/* Initialize Sphere */
	int err = SDL_snprintf(path, PATH_MAX, "%ssphere.bin", SDL_GetBasePath());
	if (err < 0) {
		SDL_Log("Failure to write path to buffer.\n");
		return SDL_APP_FAILURE;
	}

	size_t data_size = 0;
	int offset = 0;
	char *data = (char *) SDL_LoadFile(path, &(data_size));
	if (!data) {
		SDL_Log("Failure to load file at %s.\n", path);
		return SDL_APP_FAILURE;
	}

	size_t indices_size = *(uint32_t *)(data + offset);
	offset += 4;
	ctx->world.indices = (uint32_t *)SDL_malloc(indices_size);
	SDL_memcpy((void *)ctx->world.indices, (void *)(data + offset), indices_size);
	offset += indices_size;

	size_t vertices_size = *(uint32_t *)(data + offset);
	offset += 4;
	ctx->world.vertices = (float *)SDL_malloc(vertices_size);
	SDL_memcpy((void *)ctx->world.vertices, (void *)(data + offset), vertices_size);
	offset += vertices_size;

	size_t normals_size = *(uint32_t *)(data + offset);
	offset += 4;
	ctx->world.normals = (float *)SDL_malloc(normals_size);
	SDL_memcpy((void *)ctx->world.normals, (void *)(data + offset), normals_size);
	offset += normals_size;

	size_t uvs_size = *(uint32_t *)(data + offset);
	offset += 4;
	ctx->world.uvs = (float *)SDL_malloc(uvs_size);
	SDL_memcpy((void *)ctx->world.uvs, (void *)(data + offset), uvs_size);

	ctx->world.indices_size = indices_size;
	ctx->world.vertices_size = vertices_size;
	ctx->world.normals_size = normals_size;
	ctx->world.uvs_size = uvs_size;

	ctx->world.index_count = indices_size / 4;    // 4 bytes
	ctx->world.vertex_count = vertices_size / 12; // 4 bytes per 3 coordinates
	ctx->world.normal_count = normals_size / 12;  // 4 bytes per 3 coordinates
	ctx->world.uv_count = uvs_size / 8;           // 4 bytes per 2 coordinates

	SDL_free(data);

	SDL_SetRenderVSync(ctx->renderer, SDL_RENDERER_VSYNC_ADAPTIVE);

	SDL_free(path);

	return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event)
{
    if (event->type == SDL_EVENT_QUIT) {
        return SDL_APP_SUCCESS;
    }
    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void *appstate)
{
	//AppState *ctx = (AppState *)appstate;
    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void *appstate, SDL_AppResult result)
{
	if (appstate) {
		AppState *ctx = (AppState *)appstate;
		SDL_DestroyRenderer(ctx->renderer);
		SDL_DestroyWindow(ctx->window);

		//if (ctx->font.ttf) {
		//	TTF_CloseFont(ctx->font.ttf);
		//}

		//TTF_Quit();
		SDL_free(ctx->world.indices);
		//SDL_free(ctx->world.vertices);
		//SDL_free(ctx->world.normals);
		///SDL_free(ctx->world.uvs);
		SDL_free(ctx);
	}
}
