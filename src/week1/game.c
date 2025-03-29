/*
 * Florbles: Alien Invasion!
 * A planetary tower defense game.
 */

#include "EGL/EGL_3d.h"
#define DEBUG true

#define SDL_MAIN_USE_CALLBACKS 1

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3/SDL_gpu.h>
#include <SDL3/SDL_error.h>
#include <SDL3/SDL_stdinc.h>

#include <cglm/cglm.h>


#include "world.h"


#define PATH_MAX 4096

#define WINDOW_WIDTH  800
#define WINDOW_HEIGHT 800

//#define FOVY 1.2217304763960306f
#define FOVY 70.0f
#define OMEGA 0.0031415926535897933f // rad / ms
#define DELTA_T 32 // milliseconds per simulation tick (16 ~ 60 FPS, 32 ~ 30 FPS)

#define STRIDE 32



typedef struct {
	mat4 mvp;
} UBO;

typedef struct {
	SDL_Window *window;
	SDL_Renderer *renderer;
	SDL_GPUDevice *gpu_dev;
	SDL_GPUGraphicsPipeline *pipeline;

	World world;

	UBO ubo;

	mat4 projection;

	Uint64 total_time;
	Uint64 physics_time;
	Uint64 prev_tick;
} AppState;



SDL_AppResult SDL_AppInit(void **appstate, int argc, char *argv[])
{
	char *path = (char *) SDL_malloc(PATH_MAX * sizeof(char));
	int err;

	AppState *ctx = (AppState *)SDL_calloc(1, sizeof(AppState));
	if (!ctx) {
		return SDL_APP_FAILURE;
	}
	*appstate = ctx;

	glm_perspective(FOVY, (float)WINDOW_WIDTH / (float)WINDOW_HEIGHT, 0.0001, 1000, ctx->projection);

	/* Initialize App */
    SDL_SetAppMetadata("Florbles: Alien Invasion!", "0.0.0a", "com.florbles");

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("Couldn't initialize SDL: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

	/* Initialize Window */
	ctx->window = SDL_CreateWindow("Florbles: Alien Invasion!", WINDOW_WIDTH, WINDOW_HEIGHT, 0);
    if (!ctx->window) {
        SDL_Log("Couldn't create window: %s\n", SDL_GetError());
        return SDL_APP_FAILURE;
    }
	//SDL_SetRenderVSync(ctx->renderer, SDL_RENDERER_VSYNC_ADAPTIVE);

	/* Initialize GPU Device */
	ctx->gpu_dev = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV, DEBUG, NULL);
    if (!ctx->gpu_dev) {
        SDL_Log("Couldn't create gpu device: %s\n", SDL_GetError());
        return SDL_APP_FAILURE;
    }

	SDL_ClaimWindowForGPUDevice(ctx->gpu_dev, ctx->window);

	/* Initialize Shaders */
	// TODO: EGL_LoadShader to make this less annoying
	err = SDL_snprintf(path, PATH_MAX, "%striangle_frag.spv", SDL_GetBasePath());
	if (err < 0) {
		SDL_Log("Failure to write path to buffer.\n");
		return SDL_APP_FAILURE;
	}

	size_t frag_shader_size = 0;
	Uint8 *frag_shader_bin = (Uint8 *) SDL_LoadFile(path, &frag_shader_size);
	if (!frag_shader_bin) {
		SDL_Log("Failure to load shader at %s.\n", path);
		return SDL_APP_FAILURE;
	}

	const SDL_GPUShaderCreateInfo frag_shader_info = {
		.code_size = frag_shader_size,
		.code = frag_shader_bin,
		.entrypoint = "main",
		.format = SDL_GPU_SHADERFORMAT_SPIRV,
		.stage = SDL_GPU_SHADERSTAGE_FRAGMENT
	};

	SDL_GPUShader *frag_shader = SDL_CreateGPUShader(ctx->gpu_dev, &frag_shader_info);

	err = SDL_snprintf(path, PATH_MAX, "%striangle_vert.spv", SDL_GetBasePath());
	if (err < 0) {
		SDL_Log("Failure to write path to buffer.\n");
		return SDL_APP_FAILURE;
	}

	size_t vert_shader_size = 0;
	Uint8 *vert_shader_bin = (Uint8 *) SDL_LoadFile(path, &vert_shader_size);
	if (!vert_shader_bin) {
		SDL_Log("Failure to load shader at %s.\n", path);
		return SDL_APP_FAILURE;
	}

	const SDL_GPUShaderCreateInfo vert_shader_info = {
		.code_size = vert_shader_size,
		.code = vert_shader_bin,
		.entrypoint = "main",
		.format = SDL_GPU_SHADERFORMAT_SPIRV,
		.stage = SDL_GPU_SHADERSTAGE_VERTEX,
		.num_uniform_buffers = 1,
	};

	SDL_GPUShader *vert_shader = SDL_CreateGPUShader(ctx->gpu_dev, &vert_shader_info);

	/* Initialize Graphics Pipeline */
	const SDL_GPUGraphicsPipelineCreateInfo pipeline_info = {
		.vertex_shader = vert_shader,
		.fragment_shader = frag_shader,
		.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST,
		.target_info = {
			.num_color_targets = 1,
			.color_target_descriptions = (SDL_GPUColorTargetDescription[]){{
				.format = SDL_GetGPUSwapchainTextureFormat(ctx->gpu_dev, ctx->window),
			}},
		}
	};

	ctx->pipeline = SDL_CreateGPUGraphicsPipeline(ctx->gpu_dev, &pipeline_info);

	SDL_ReleaseGPUShader(ctx->gpu_dev, frag_shader);
	SDL_ReleaseGPUShader(ctx->gpu_dev, vert_shader);

	/* Initialize Sphere */
	err = SDL_snprintf(path, PATH_MAX, "%ssphere.bin", SDL_GetBasePath());
	if (err < 0) {
		SDL_Log("Failure to write path to buffer.\n");
		return SDL_APP_FAILURE;
	}

	char *data = (char *) SDL_LoadFile(path, NULL);
	if (!data) {
		SDL_Log("Failure to load file at %s.\n", path);
		return SDL_APP_FAILURE;
	}

	World_Deserialize(&ctx->world, data);


	Transform *world_transform = &ctx->world.transform;
	EGL_TransformNew(world_transform);
	world_transform->tz -= 5.0f;
	EGL_TransformUpdate(world_transform);
	EGL_TransformCopy(world_transform, &ctx->world.render_transform);

	SDL_free(data);
	SDL_free(path);

	ctx->prev_tick = SDL_GetTicks();

	return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event)
{
	AppState *ctx = (AppState *)appstate;

    if (event->type == SDL_EVENT_QUIT) {
        return SDL_APP_SUCCESS;
    } else if (event->type == SDL_EVENT_WINDOW_RESIZED) {
		int w = 0;
		int h = 0;
		SDL_GetWindowSizeInPixels(ctx->window, &w, &h);
		glm_perspective(FOVY, (float)w / (float)h, 0.0001, 1000, ctx->projection);
	}

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void *appstate)
{
	AppState *ctx = (AppState *)appstate;
	
	World *world = &ctx->world;

	SDL_Window *window = ctx->window;
	SDL_GPUDevice *gpu_dev = ctx->gpu_dev;
	SDL_GPUGraphicsPipeline *pipeline = ctx->pipeline;

	bool ok;

	/* Physics */
	const Uint64 now = SDL_GetTicks();
	const Uint64 dt = now - ctx->prev_tick;

	// Physics
	ctx->physics_time += dt;

	while (ctx->physics_time >= DELTA_T) {
		EGL_TransformCopy(&world->transform, &world->render_transform);
		EGL_TransformRotate(&world->transform, OMEGA * DELTA_T, GLM_YUP);

		ctx->physics_time -= DELTA_T;
	}

	
	/* Game State */

	/* Rendering */
	glm_quat_slerp(world->render_transform.rotation, world->transform.rotation, (float)ctx->physics_time / (float)DELTA_T, world->render_transform.rotation);
	EGL_TransformUpdate(&world->render_transform);
	glm_mat4_mul(ctx->projection, world->render_transform.model, ctx->ubo.mvp);
	SDL_GPUCommandBuffer *cmd_buf = SDL_AcquireGPUCommandBuffer(gpu_dev); SDL_assert(NULL != cmd_buf);

	SDL_GPUTexture *swapchain_tex;
	ok = SDL_WaitAndAcquireGPUSwapchainTexture(cmd_buf, window, &swapchain_tex, NULL, NULL); SDL_assert(ok);

	/* Check for NULL swapchain texture which can occur if the window resizes */
	if (swapchain_tex) {
		SDL_GPUColorTargetInfo color_target = {
			.texture = swapchain_tex,
			.load_op = SDL_GPU_LOADOP_CLEAR,
			.clear_color = { .r=0.0, .g=0.2, .b=0.4, .a=1.0 },
			.store_op = SDL_GPU_STOREOP_STORE,
		};

		SDL_GPURenderPass *render_pass = SDL_BeginGPURenderPass(cmd_buf, &color_target, 1, NULL);
		// DRAW
		SDL_BindGPUGraphicsPipeline(render_pass, pipeline);
		SDL_PushGPUVertexUniformData(cmd_buf, 0, &ctx->ubo, sizeof(ctx->ubo));
		SDL_DrawGPUPrimitives(render_pass, 3, 1, 0, 0);
		SDL_EndGPURenderPass(render_pass);
		// additional render passes
	}

	ok = SDL_SubmitGPUCommandBuffer(cmd_buf); SDL_assert(ok);

	ctx->prev_tick = now;

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
		SDL_free(ctx->world.vertices);
		SDL_free(ctx->world.normals);
		SDL_free(ctx->world.uvs);
		SDL_free(ctx);
	}
}
