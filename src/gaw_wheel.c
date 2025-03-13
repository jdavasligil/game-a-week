/*
 * Game A Week (Project GAW)
 * Wheel Of Fortune
 */

#include <SDL3/SDL_error.h>
#include <SDL3/SDL_iostream.h>
#define SDL_MAIN_USE_CALLBACKS 1

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3/SDL_filesystem.h>
#include <SDL3/SDL_pixels.h>
#include <SDL3/SDL_render.h>
#include <SDL3/SDL_surface.h>
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_log.h>
#include <SDL3/SDL_stdinc.h>
#include <SDL3/SDL_timer.h>
#include <SDL3_ttf/SDL_ttf.h>

#include <cglm/cglm.h>

#define DEBUG // Uncomment this line to enable runtime debugging.

#define PATH_MAX 4096

#define TEXT_PT_SIZE 128.0f

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 800

#define DELTA_T 32 // milliseconds per simulation tick (16 ~ 60 FPS, 32 ~ 30 FPS)
#define IMPULSE 0.00025f // d𝛚 = (I / T) dt where IMPULSE = (I / T) [I = Moment of Inertia, T = Avg Torque]

#define STR_FastReset(s) ( s[0] = '\0' )

typedef struct {
	float angle;
	float angle_prev;
	float angular_speed;
	float radius;
	SDL_Texture *texture;
} Wheel;

typedef struct {
	TTF_Font    *ttf;
	SDL_Texture *texture;
} Font;

typedef struct {
	SDL_Window *window;
	SDL_Renderer *renderer;
	Font font;
	Wheel wheel;

#ifdef DEBUG
	float avg_fps;
	Uint32 frames;
	Uint32 frame_time;
#endif

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
    SDL_SetAppMetadata("Wheel Of Fortune", "0.0.0a", "com.wheel");

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("Couldn't initialize SDL: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

	/* Initialize Window */
    if (!SDL_CreateWindowAndRenderer("GEOMETRY EXAMPLE", WINDOW_WIDTH, WINDOW_HEIGHT, 0, &ctx->window, &ctx->renderer)) {
        SDL_Log("Couldn't create window/renderer: %s\n", SDL_GetError());
        return SDL_APP_FAILURE;
    }

	SDL_SetRenderVSync(ctx->renderer, SDL_RENDERER_VSYNC_ADAPTIVE);

	/* Load Wheel Texture */
	int err = SDL_snprintf(path, PATH_MAX, "%swheel.bmp", SDL_GetBasePath());
	if (err < 0) {
		SDL_Log("Failure to write path to buffer.\n");
		return SDL_APP_FAILURE;
	}

	SDL_Surface *surface = SDL_LoadBMP(path);
	if (!surface) {
		SDL_Log("Failure to load bitmap: %s", SDL_GetError());
		return SDL_APP_FAILURE;
	}

	ctx->wheel.texture = SDL_CreateTextureFromSurface(ctx->renderer, surface);
	if (!ctx->wheel.texture) {
		SDL_Log("Failure to create static texture: %s\n", SDL_GetError());
		return SDL_APP_FAILURE;
	}

	SDL_DestroySurface(surface);

	/* Load Font */
	if (!TTF_Init()) {
		SDL_Log("Failure to initialize SDL_ttf: %s\n", SDL_GetError());
		return SDL_APP_FAILURE;
	}

	STR_FastReset(path);
	err = SDL_snprintf(path, PATH_MAX, "%shey_comic.ttf", SDL_GetBasePath());
	if (err < 0) {
		SDL_Log("Failure to write path to buffer.\n");
		return SDL_APP_FAILURE;
	}

	ctx->font.ttf = TTF_OpenFontIO(SDL_IOFromFile(path, "r"), true, TEXT_PT_SIZE);
	if (!ctx->font.ttf) {
		SDL_Log("Failure to open font: %s\n", SDL_GetError());
		return SDL_APP_FAILURE;
	}

	SDL_Color color = { 255, 255, 255, SDL_ALPHA_OPAQUE };
	SDL_Surface *text = TTF_RenderText_Blended(ctx->font.ttf, "TEST", 0, color);
	if (text) {
		ctx->font.texture = SDL_CreateTextureFromSurface(ctx->renderer, text);
		SDL_DestroySurface(text);
	}
	if (!ctx->font.texture) {
		SDL_Log("Failure to create text texture: %s\n", SDL_GetError());
		return SDL_APP_FAILURE;
	}

	/* Initialize Wheel */
	ctx->wheel.angular_speed = 0.72f; // deg / ms
	ctx->wheel.radius = WINDOW_WIDTH / 2.0f; 

	SDL_free(path);

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
	const float render_angle = glm_lerp(ctx->wheel.angle_prev, ctx->wheel.angle, ((float) ctx->physics_time) / DELTA_T);

	SDL_SetRenderDrawColor(ctx->renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
	SDL_RenderClear(ctx->renderer);

	SDL_RenderTextureRotated(ctx->renderer, ctx->wheel.texture, NULL, &wheel_AABB, render_angle, &center, SDL_FLIP_NONE);
	SDL_RenderTextureRotated(ctx->renderer, ctx->font.texture, NULL, &wheel_AABB, render_angle, &center, SDL_FLIP_NONE);
	SDL_SetRenderDrawColor(ctx->renderer, 255, 255, 255, SDL_ALPHA_OPAQUE);  /* white, full alpha */

#ifdef DEBUG
	if (ctx->frames >= 10) {
		ctx->avg_fps = 10000.0f / (float)ctx->frame_time;
		ctx->frame_time = 0;
		ctx->frames = 0;
	}
    SDL_RenderDebugTextFormat(ctx->renderer, 10, 10, "%f FPS", ctx->avg_fps);
	ctx->frame_time += now - ctx->prev_tick;
	ctx->frames += 1;
#endif

	SDL_RenderPresent(ctx->renderer);

	// Update frame timer
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

		if (ctx->font.ttf) {
			TTF_CloseFont(ctx->font.ttf);
		}

		TTF_Quit();
		SDL_free(ctx);
	}
}
