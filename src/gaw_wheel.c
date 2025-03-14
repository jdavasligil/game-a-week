/*
 * Game A Week (Project GAW)
 * Wheel Of Fortune
 */
#define SDL_MAIN_USE_CALLBACKS 1
#define DEBUG // Uncomment this line to enable runtime debugging.

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3/SDL_error.h>
#include <SDL3/SDL_iostream.h>
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

#include <EGL/EGL_strings.h>


#define PATH_MAX 4096
#define WHEEL_MAX 12
#define WORD_MAX 16

#define TEXT_PT_SIZE 32.0f

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 800

#define WHEEL_DIAMETER 600
#define WHEEL_RADIUS   300
#define WHEEL_SPEED    0.72f // deg / ms

#define DELTA_T 32 // milliseconds per simulation tick (16 ~ 60 FPS, 32 ~ 30 FPS)
#define IMPULSE 0.00025f // d𝛚 = (I / T) dt where IMPULSE = (I / T) [I = Moment of Inertia, T = Avg Torque]

#define EGL_ClearStr(s) ( s[0] = '\0' )

typedef struct {
	char words[WHEEL_MAX][WORD_MAX];

	float angle;
	float angle_prev;
	float angular_speed;

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
		SDL_Log("Failure to load bitmap: %s\n", SDL_GetError());
		return SDL_APP_FAILURE;
	}

	ctx->wheel.texture = SDL_CreateTextureFromSurface(ctx->renderer, surface);
	if (!ctx->wheel.texture) {
		SDL_Log("Failure to create static texture: %s\n", SDL_GetError());
		return SDL_APP_FAILURE;
	}

	SDL_DestroySurface(surface);

	/* Initialize Wheel */
	ctx->wheel.angular_speed = WHEEL_SPEED;

	/* Load Words */
	EGL_ClearStr(path);
	err = SDL_snprintf(path, PATH_MAX, "%sgames.dat", SDL_GetBasePath());
	if (err < 0) {
		SDL_Log("Failure to write path to buffer.\n");
		return SDL_APP_FAILURE;
	}

	Reader word_reader;
	word_reader.data = (char *) SDL_LoadFile(path, &(word_reader.size));
	word_reader.offset = 0;

	if (!word_reader.data) {
		SDL_Log("Failure to load file: %s\n", SDL_GetError());
		return SDL_APP_FAILURE;
	}

	for (int i = 0; i < WHEEL_MAX; i++) {
		err = EGL_ReadLine(&word_reader, ctx->wheel.words[i], WORD_MAX);
		if (err < 0) {
		SDL_Log("Failure to read line %d with error code: %d\n", i, err);
		return SDL_APP_FAILURE;
		}
	}
	SDL_free(word_reader.data);

	/* Load Font */
	if (!TTF_Init()) {
		SDL_Log("Failure to initialize SDL_ttf: %s\n", SDL_GetError());
		return SDL_APP_FAILURE;
	}

	EGL_ClearStr(path);
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

	SDL_free(path);

	/* Create texture for words on wheel */
	SDL_Color word_color = { 255, 255, 255, SDL_ALPHA_OPAQUE };
	ctx->font.texture = SDL_CreateTexture(ctx->renderer, SDL_PIXELFORMAT_ARGB32, SDL_TEXTUREACCESS_TARGET, WHEEL_DIAMETER, WHEEL_DIAMETER);
	SDL_SetRenderTarget(ctx->renderer, ctx->font.texture);

	float render_angle = 15.0f;
	SDL_FPoint center = { (float) WHEEL_RADIUS, (float) WHEEL_RADIUS };
	SDL_Surface *word_surface = SDL_CreateSurface(WHEEL_DIAMETER, WHEEL_DIAMETER, SDL_PIXELFORMAT_ARGB32);

	for (int i = 0; i < WHEEL_MAX; i++) {
		SDL_Surface *text = TTF_RenderText_Blended(ctx->font.ttf, ctx->wheel.words[i], 0, word_color);
		const SDL_Rect word_rect = {
			.x = (int)(WHEEL_RADIUS * 1.15 + (WHEEL_RADIUS * 0.85 - (float)text->w) / 2.0f),
			.y = WHEEL_RADIUS - (int) (TEXT_PT_SIZE * 2.0f / 3.0f),
			.w = WHEEL_RADIUS,
			.h = (int) (((float) WHEEL_RADIUS) * 0.10),
		};
		SDL_BlitSurface(text, NULL, word_surface, &word_rect);
		SDL_DestroySurface(text);

		SDL_Texture *word_texture = SDL_CreateTextureFromSurface(ctx->renderer, word_surface);
		SDL_ClearSurface(word_surface, 0.0f, 0.0f, 0.0f, 0.0f);

		SDL_RenderTextureRotated(ctx->renderer, word_texture, NULL, NULL, render_angle, &center, SDL_FLIP_NONE);
		render_angle += 30.0f;
	}
	SDL_DestroySurface(word_surface);
	SDL_SetRenderTarget(ctx->renderer, NULL);

	if (!ctx->font.texture) {
		SDL_Log("Failure to create text texture: %s\n", SDL_GetError());
		return SDL_APP_FAILURE;
	}

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
	wheel_AABB.x = (float) (WINDOW_WIDTH - WHEEL_DIAMETER) / 2.0f;
	wheel_AABB.y = (float) (WINDOW_HEIGHT - WHEEL_DIAMETER) / 2.0f;
	wheel_AABB.w = (float) WHEEL_DIAMETER;
	wheel_AABB.h = (float) WHEEL_DIAMETER;

	center.x = (float) WHEEL_RADIUS;
	center.y = (float) WHEEL_RADIUS;

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
