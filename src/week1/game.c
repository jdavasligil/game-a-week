/*
 * Florbles: Alien Invasion!
 * A planetary tower defense game.
 */

#include <SDL3/SDL_error.h>
#include <SDL3/SDL_stdinc.h>
#define DEBUG true

#define SDL_MAIN_USE_CALLBACKS 1

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3/SDL_gpu.h>

#include "world.h"


#define PATH_MAX 4096

#define WINDOW_WIDTH  800
#define WINDOW_HEIGHT 800

#define STRIDE 32


typedef struct {
	SDL_Window *window;
	SDL_Renderer *renderer;
	SDL_GPUDevice *gpu_dev;
	SDL_GPUGraphicsPipeline *pipeline;

	World world;

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

	/* Initialize GPU Device */
	ctx->gpu_dev = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV, DEBUG, NULL);
    if (!ctx->gpu_dev) {
        SDL_Log("Couldn't create gpu device: %s\n", SDL_GetError());
        return SDL_APP_FAILURE;
    }

	SDL_ClaimWindowForGPUDevice(ctx->gpu_dev, ctx->window);

	/* Initialize Shaders */
	// TODO: EGL_LoadShader to make this less annoying
	err = SDL_snprintf(path, PATH_MAX, "%striangle.spv.frag", SDL_GetBasePath());
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

	err = SDL_snprintf(path, PATH_MAX, "%striangle.spv.vert", SDL_GetBasePath());
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
		.stage = SDL_GPU_SHADERSTAGE_VERTEX
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
	AppState *ctx = (AppState *)appstate;
	SDL_Window *window = ctx->window;
	SDL_GPUDevice *gpu_dev = ctx->gpu_dev;
	SDL_GPUGraphicsPipeline *pipeline = ctx->pipeline;

	bool ok;
	
	/* Game State */

	/* Rendering */
	SDL_GPUCommandBuffer *cmd_buf = SDL_AcquireGPUCommandBuffer(gpu_dev); SDL_assert(NULL != cmd_buf);

	SDL_GPUTexture *swapchain_tex;
	ok = SDL_WaitAndAcquireGPUSwapchainTexture(cmd_buf, window, &swapchain_tex, NULL, NULL); SDL_assert(ok);
	SDL_GPUColorTargetInfo color_target = {
		.texture = swapchain_tex,
		.load_op = SDL_GPU_LOADOP_CLEAR,
		.clear_color = { .r=0.0, .g=0.2, .b=0.4, .a=1.0 },
		.store_op = SDL_GPU_STOREOP_STORE,
	};

	SDL_GPURenderPass *render_pass = SDL_BeginGPURenderPass(cmd_buf, &color_target, 1, NULL);
	// DRAW
	SDL_BindGPUGraphicsPipeline(render_pass, pipeline);
	// - bind vertex data
	// - bind uniform data
	// - draw calls
	SDL_DrawGPUPrimitives(render_pass, 3, 1, 0, 0);
	SDL_EndGPURenderPass(render_pass);
	// additional render passes
	ok = SDL_SubmitGPUCommandBuffer(cmd_buf); SDL_assert(ok);

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
