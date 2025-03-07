/*
 * Game A Week (Project GAW)
 * Wheel Of Fortune
 */
#include <SDL3/SDL_filesystem.h>
#include <SDL3/SDL_pixels.h>
#include <SDL3/SDL_render.h>
#include <SDL3/SDL_surface.h>
#define SDL_MAIN_USE_CALLBACKS 1

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include <SDL3/SDL_init.h>
#include <SDL3/SDL_log.h>
#include <SDL3/SDL_stdinc.h>
#include <SDL3/SDL_timer.h>

#include <cglm/cglm.h>

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 800

#define DELTA_T 32 // milliseconds per simulation tick (60 FPS)

#define IMPULSE 0.00025f // d𝛚 = (I / T) dt where IMPULSE = (I / T) [I = Moment of Inertia, T = Avg Torque]

typedef struct {
	float angle;
	float angle_prev;
	float angular_speed;
	float radius;
	SDL_Texture *texture;
} Wheel;

typedef struct {
	SDL_Window *window;
	SDL_Renderer *renderer;

	Uint64 prev_tick;
	Uint64 total_time;
	Uint64 frames;
	Uint64 physics_time;

	Wheel wheel;
} AppState;


SDL_AppResult SDL_AppInit(void **appstate, int argc, char *argv[])
{
	SDL_Surface *surface = NULL;
	char *bmp_path = NULL;

	AppState *ctx = (AppState *)SDL_calloc(1, sizeof(AppState));
	if (!ctx) {
		return SDL_APP_FAILURE;
	}

	*appstate = ctx;

	ctx->wheel.angular_speed = 0.72f; // deg / ms
	ctx->wheel.radius = WINDOW_WIDTH / 2.0f; 

    SDL_SetAppMetadata("Raycast Engine", "0.0.0a", "com.raycast");

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("Couldn't initialize SDL: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    if (!SDL_CreateWindowAndRenderer("GEOMETRY EXAMPLE", WINDOW_WIDTH, WINDOW_HEIGHT, 0, &ctx->window, &ctx->renderer)) {
        SDL_Log("Couldn't create window/renderer: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

	SDL_SetRenderVSync(ctx->renderer, SDL_RENDERER_VSYNC_ADAPTIVE);

	SDL_asprintf(&bmp_path, "%swheel.bmp", SDL_GetBasePath());
	surface = SDL_LoadBMP(bmp_path);
	if (!surface) {
		SDL_Log("Failure to load bitmap: %s", SDL_GetError());
		return SDL_APP_FAILURE;
	}
	SDL_free(bmp_path);

	ctx->wheel.texture = SDL_CreateTextureFromSurface(ctx->renderer, surface);
	if (!ctx->wheel.texture) {
		SDL_Log("Failure to create static texture: %s", SDL_GetError());
		return SDL_APP_FAILURE;
	}

	SDL_DestroySurface(surface);

	ctx->prev_tick = SDL_GetTicks();

    return SDL_APP_CONTINUE;  /* carry on with the program! */
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
	AppState *ctx = (AppState *)appstate;

	SDL_FPoint center;
	SDL_FRect wheel_AABB;

	const Uint64 now = SDL_GetTicks();
	const Uint64 dt = now - ctx->prev_tick;

	// Physics
	ctx->physics_time += dt;

	while (ctx->physics_time >= DELTA_T) {

		ctx->wheel.angle_prev = ctx->wheel.angle;
		ctx->wheel.angle += ctx->wheel.angular_speed * DELTA_T;
		ctx->wheel.angular_speed -= IMPULSE * DELTA_T;
		ctx->wheel.angular_speed = (ctx->wheel.angular_speed < 0) ? 0 : ctx->wheel.angular_speed;

		ctx->physics_time -= DELTA_T;
	}

	// Game Logic
	float diameter = 2.0f * ctx->wheel.radius;

	wheel_AABB.x = (WINDOW_WIDTH - diameter) / 2.0f;
	wheel_AABB.y = (WINDOW_HEIGHT - diameter) / 2.0f;
	wheel_AABB.w = diameter;
	wheel_AABB.h = diameter;

	center.x = diameter / 2.0f;
	center.y = diameter / 2.0f;

	// Draw frame
	SDL_SetRenderDrawColor(ctx->renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
	SDL_RenderClear(ctx->renderer);
	SDL_RenderTextureRotated(ctx->renderer, ctx->wheel.texture, NULL, &wheel_AABB, glm_lerp(ctx->wheel.angle_prev, ctx->wheel.angle, ((float) ctx->physics_time) / DELTA_T), &center, SDL_FLIP_NONE);
	SDL_RenderPresent(ctx->renderer);

	// Update frame timer
	ctx->total_time += now - ctx->prev_tick;
	ctx->frames += 1;
	ctx->prev_tick = now;

    return SDL_APP_CONTINUE;
}


void SDL_AppQuit(void *appstate, SDL_AppResult result)
{
	if (NULL != appstate) {
		AppState *ctx = (AppState *)appstate;
		//SDL_Log("Avg frames per second: %f\n", 1000.0f * (float)ctx->frames / (float)ctx->total_time);
		SDL_DestroyRenderer(ctx->renderer);
		SDL_DestroyWindow(ctx->window);
		SDL_free(ctx);
	}
}
