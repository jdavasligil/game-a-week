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

typedef struct {
	float angle;
	float angle_prev;
	float angular_speed;
	float friction;      // 0 < f_k <= 1
	float radius;        // r > 0
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

	AppState *app_state_p = (AppState *)SDL_calloc(1, sizeof(AppState));
	if (!app_state_p) {
		return SDL_APP_FAILURE;
	}

	*appstate = app_state_p;

	app_state_p->wheel.angular_speed = 0.72f; // deg / ms
	app_state_p->wheel.friction = 0.0025f;
	app_state_p->wheel.radius = WINDOW_WIDTH / 2.0f; 

    SDL_SetAppMetadata("Raycast Engine", "0.0.0a", "com.raycast");

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("Couldn't initialize SDL: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    if (!SDL_CreateWindowAndRenderer("GEOMETRY EXAMPLE", WINDOW_WIDTH, WINDOW_HEIGHT, 0, &app_state_p->window, &app_state_p->renderer)) {
        SDL_Log("Couldn't create window/renderer: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

	SDL_SetRenderVSync(app_state_p->renderer, SDL_RENDERER_VSYNC_ADAPTIVE);

	SDL_asprintf(&bmp_path, "%swheel.bmp", SDL_GetBasePath());
	surface = SDL_LoadBMP(bmp_path);
	if (!surface) {
		SDL_Log("Failure to load bitmap: %s", SDL_GetError());
		return SDL_APP_FAILURE;
	}
	SDL_free(bmp_path);

	app_state_p->wheel.texture = SDL_CreateTextureFromSurface(app_state_p->renderer, surface);
	if (!app_state_p->wheel.texture) {
		SDL_Log("Failure to create static texture: %s", SDL_GetError());
		return SDL_APP_FAILURE;
	}

	SDL_DestroySurface(surface);

	app_state_p->prev_tick = SDL_GetTicks();

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
	AppState *app_state_p = (AppState *)appstate;

	SDL_FPoint center;
	SDL_FRect wheel_AABB;

	const Uint64 now = SDL_GetTicks();
	const Uint64 dt = now - app_state_p->prev_tick;

	// Physics
	app_state_p->physics_time += dt;

	while (app_state_p->physics_time >= DELTA_T) {

		app_state_p->wheel.angle_prev = app_state_p->wheel.angle;
		app_state_p->wheel.angle += app_state_p->wheel.angular_speed * DELTA_T;
		app_state_p->wheel.angular_speed -= app_state_p->wheel.friction;
		app_state_p->wheel.angular_speed = (app_state_p->wheel.angular_speed < 0) ? 0 : app_state_p->wheel.angular_speed;

		app_state_p->physics_time -= DELTA_T;
	}

	// Game Logic
	float diameter = 2.0f * app_state_p->wheel.radius;

	wheel_AABB.x = (WINDOW_WIDTH - diameter) / 2.0f;
	wheel_AABB.y = (WINDOW_HEIGHT - diameter) / 2.0f;
	wheel_AABB.w = diameter;
	wheel_AABB.h = diameter;

	center.x = diameter / 2.0f;
	center.y = diameter / 2.0f;

	// Draw frame
	SDL_SetRenderDrawColor(app_state_p->renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
	SDL_RenderClear(app_state_p->renderer);
	SDL_RenderTextureRotated(app_state_p->renderer, app_state_p->wheel.texture, NULL, &wheel_AABB, glm_lerp(app_state_p->wheel.angle_prev, app_state_p->wheel.angle, ((float) app_state_p->physics_time) / DELTA_T), &center, SDL_FLIP_NONE);
	SDL_RenderPresent(app_state_p->renderer);

	// Update frame timer
	app_state_p->total_time += now - app_state_p->prev_tick;
	app_state_p->frames += 1;
	app_state_p->prev_tick = now;

    return SDL_APP_CONTINUE;
}


void SDL_AppQuit(void *appstate, SDL_AppResult result)
{
	if (NULL != appstate) {
		AppState *app_state_p = (AppState *)appstate;
		//SDL_Log("Avg frames per second: %f\n", 1000.0f * (float)app_state_p->frames / (float)app_state_p->total_time);
		SDL_DestroyRenderer(app_state_p->renderer);
		SDL_DestroyWindow(app_state_p->window);
		SDL_free(app_state_p);
	}
}
