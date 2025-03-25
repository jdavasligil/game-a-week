/*
 * Florbles: Alien Invasion!
 * A planetary tower defense game.
 */
#define SDL_MAIN_USE_CALLBACKS 1


#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>


#define WINDOW_WIDTH  800
#define WINDOW_HEIGHT 800

#define WORLD_VERTEX_SIZE 2187   // packed (x,y,z)
#define WORLD_NORMAL_SIZE 2187   // packed (x,y,z)
#define WORLD_UV_SIZE     1458   // packed (u,v)
#define WORLD_INDEX_SIZE  3840

#define STRIDE 32


typedef struct {
	float *indices;
	float *vertices;
	float *normals;
	float *uvs;
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

	SDL_SetRenderVSync(ctx->renderer, SDL_RENDERER_VSYNC_ADAPTIVE);

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
		SDL_free(ctx);
	}
}
