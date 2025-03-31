/*
 * Florbles: Alien Invasion!
 * A planetary tower defense game.
 */

#define DEBUG true

#define SDL_MAIN_USE_CALLBACKS 1

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3/SDL_gpu.h>
#include <SDL3/SDL_error.h>
#include <SDL3/SDL_stdinc.h>
#include <SDL3/SDL_surface.h>

#include <stddef.h>
#include <stdint.h>

#include <EGL/EGL_3d.h>

#include <cglm/cglm.h>

#include "world.h"


#define PATH_MAX 4096

#define WINDOW_WIDTH  800
#define WINDOW_HEIGHT 800

#define FOVY 1.2217304763960306f // 70 deg in radians
#define OMEGA 0.0031415926535897933f // rad / ms
#define DELTA_T 16 // milliseconds per simulation tick (16 ~ 60 FPS, 32 ~ 30 FPS)

#define STRIDE 32


typedef struct {
	mat4 mvp;
} UBO;

typedef struct {
	SDL_Window *window;
	SDL_Renderer *renderer;
	SDL_GPUDevice *gpu_dev;
	SDL_GPUGraphicsPipeline *pipeline;
	SDL_GPUBuffer *vertex_buffer;
	SDL_GPUBuffer *index_buffer;
	SDL_GPUTexture *brick_texture;
	SDL_GPUSampler *sampler;

	World world;

	UBO ubo;

	mat4 projection;

	Uint64 total_time;
	Uint64 physics_time;
	Uint64 prev_tick;
} AppState;

typedef struct {
	vec3 vertex;
	SDL_FColor color;
	vec2 uv;
} VertexData;


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
        SDL_Log("Couldn't create window: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }
	//SDL_SetRenderVSync(ctx->renderer, SDL_RENDERER_VSYNC_ADAPTIVE);

	/* Initialize GPU Device */
	ctx->gpu_dev = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV, DEBUG, NULL);
    if (!ctx->gpu_dev) {
        SDL_Log("Couldn't create gpu device: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

	SDL_ClaimWindowForGPUDevice(ctx->gpu_dev, ctx->window);

	/* Initialize Sphere */
	err = SDL_snprintf(path, PATH_MAX, "%ssphere.bin", SDL_GetBasePath());
	if (err < 0) {
		SDL_Log("Failure to write path to buffer.");
		return SDL_APP_FAILURE;
	}

	char *data = (char *) SDL_LoadFile(path, NULL);
	if (!data) {
		SDL_Log("Failure to load file at %s.", path);
		return SDL_APP_FAILURE;
	}

	World_Deserialize(&ctx->world, data);


	Transform *world_transform = &ctx->world.transform;
	EGL_TransformNew(world_transform);
	world_transform->z -= 3.0f;
	EGL_TransformUpdate(world_transform);
	EGL_TransformCopy(world_transform, &ctx->world.render_transform);
#ifdef DEBUG
	EGL_TransformPrint(world_transform);
#endif

	/* Load Texture */
	err = SDL_snprintf(path, PATH_MAX, "%sbricks.bmp", SDL_GetBasePath());
	if (err < 0) {
		SDL_Log("Failure to write path to buffer.");
		return SDL_APP_FAILURE;
	}

	SDL_Surface *brick_surface = SDL_LoadBMP(path);
	if (!brick_surface) {
		SDL_Log("Failure to load bitmap: %s", SDL_GetError());
		return SDL_APP_FAILURE;
	}

	const uint32_t brick_texture_size = brick_surface->w * brick_surface->h * 2;
	ctx->brick_texture = SDL_CreateGPUTexture(ctx->gpu_dev, (SDL_GPUTextureCreateInfo[]){(SDL_GPUTextureCreateInfo){
		.type = SDL_GPU_TEXTURETYPE_2D,	
		.format = SDL_GPU_TEXTUREFORMAT_B5G5R5A1_UNORM,
		.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER,
		.width = brick_surface->w,
		.height = brick_surface->h,
		.layer_count_or_depth = 1,
		.num_levels = 1,
	}});
	if (!ctx->brick_texture) {
		SDL_Log("Failure to create brick texture: %s", SDL_GetError());
		return SDL_APP_FAILURE;
	}

	/* Initialize Shaders */
	// TODO: EGL_LoadShader to make this less annoying
	err = SDL_snprintf(path, PATH_MAX, "%striangle_frag.spv", SDL_GetBasePath());
	if (err < 0) {
		SDL_Log("Failure to write path to buffer.");
		return SDL_APP_FAILURE;
	}
	size_t frag_shader_size = 0;
	Uint8 *frag_shader_bin = (Uint8 *) SDL_LoadFile(path, &frag_shader_size);
	if (!frag_shader_bin) {
		SDL_Log("Failure to load shader at %s.", path);
		return SDL_APP_FAILURE;
	}
	SDL_GPUShader *frag_shader = SDL_CreateGPUShader(ctx->gpu_dev, (SDL_GPUShaderCreateInfo[]){{
		.code_size = frag_shader_size,
		.code = frag_shader_bin,
		.entrypoint = "main",
		.format = SDL_GPU_SHADERFORMAT_SPIRV,
		.stage = SDL_GPU_SHADERSTAGE_FRAGMENT,
		.num_samplers = 1,
	}});

	err = SDL_snprintf(path, PATH_MAX, "%striangle_vert.spv", SDL_GetBasePath());
	if (err < 0) {
		SDL_Log("Failure to write path to buffer.");
		return SDL_APP_FAILURE;
	}
	size_t vert_shader_size = 0;
	Uint8 *vert_shader_bin = (Uint8 *) SDL_LoadFile(path, &vert_shader_size);
	if (!vert_shader_bin) {
		SDL_Log("Failure to load shader at %s.", path);
		return SDL_APP_FAILURE;
	}
	SDL_GPUShader *vert_shader = SDL_CreateGPUShader(ctx->gpu_dev, (SDL_GPUShaderCreateInfo[]){{
		.code_size = vert_shader_size,
		.code = vert_shader_bin,
		.entrypoint = "main",
		.format = SDL_GPU_SHADERFORMAT_SPIRV,
		.stage = SDL_GPU_SHADERSTAGE_VERTEX,
		.num_uniform_buffers = 1,
	}});

	/* Initialize Graphics Pipeline */
	const uint32_t vertex_count = 4;
	VertexData *vertices = (VertexData[]){
		{ .vertex = {-0.5,  0.5,  0.0}, .color = {1.0, 0.0, 0.0, 1}, .uv = {0.0, 0.0} },
		{ .vertex = { 0.5,  0.5,  0.0}, .color = {0.0, 1.0, 0.0, 1}, .uv = {1.0, 0.0} },
		{ .vertex = {-0.5, -0.5,  0.0}, .color = {0.0, 0.0, 1.0, 1}, .uv = {0.0, 1.0} },
		{ .vertex = { 0.5, -0.5,  0.0}, .color = {1.0, 1.0, 0.0, 1}, .uv = {1.0, 1.0} },
	};
	const uint32_t vert_size = vertex_count * sizeof(VertexData);

	const uint32_t index_count = 6;
	uint16_t *indices = (uint16_t[]){
		0, 1, 2,
		2, 1, 3,
	};
	const uint32_t index_size = index_count * sizeof(*indices);

	ctx->vertex_buffer = SDL_CreateGPUBuffer(ctx->gpu_dev, (SDL_GPUBufferCreateInfo[]){{
		.usage = SDL_GPU_BUFFERUSAGE_VERTEX,
		.size = vert_size,
	}});
	if (!ctx->vertex_buffer) {
		SDL_Log("Failure to create vertex buffer: %s", SDL_GetError());
		return SDL_APP_FAILURE;
	}

	ctx->index_buffer = SDL_CreateGPUBuffer(ctx->gpu_dev, (SDL_GPUBufferCreateInfo[]){{
		.usage = SDL_GPU_BUFFERUSAGE_INDEX,
		.size = index_size,
	}});
	if (!ctx->index_buffer) {
		SDL_Log("Failure to create index buffer: %s", SDL_GetError());
		return SDL_APP_FAILURE;
	}

	SDL_GPUTransferBuffer *transfer_buffer = SDL_CreateGPUTransferBuffer(ctx->gpu_dev, (SDL_GPUTransferBufferCreateInfo[]){{
		.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
		.size = vert_size + index_size,
	}});
	if (!transfer_buffer) {
		SDL_Log("Failure to create transfer buffer: %s", SDL_GetError());
		return SDL_APP_FAILURE;
	}

	SDL_GPUTransferBuffer *tex_transfer_buffer = SDL_CreateGPUTransferBuffer(ctx->gpu_dev, (SDL_GPUTransferBufferCreateInfo[]){{
		.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
		.size = brick_texture_size,
	}});
	if (!tex_transfer_buffer) {
		SDL_Log("Failure to create transfer buffer: %s", SDL_GetError());
		return SDL_APP_FAILURE;
	}

	void *transfer_mem = SDL_MapGPUTransferBuffer(ctx->gpu_dev, transfer_buffer, false);
	if (!transfer_mem) {
		SDL_Log("Failure to obtain transfer memory: %s", SDL_GetError());
		return SDL_APP_FAILURE;
	}
	SDL_memcpy(transfer_mem, vertices, vert_size);
	SDL_memcpy((uint8_t *)transfer_mem + vert_size, indices, index_size);

	void *tex_transfer_mem = SDL_MapGPUTransferBuffer(ctx->gpu_dev, tex_transfer_buffer, false);
	if (!tex_transfer_mem) {
		SDL_Log("Failure to obtain transfer memory: %s", SDL_GetError());
		return SDL_APP_FAILURE;
	}
	SDL_memcpy(tex_transfer_mem, brick_surface->pixels, brick_texture_size);

	SDL_UnmapGPUTransferBuffer(ctx->gpu_dev, transfer_buffer);
	SDL_UnmapGPUTransferBuffer(ctx->gpu_dev, tex_transfer_buffer);

	SDL_GPUCommandBuffer *copy_cmd_buf = SDL_AcquireGPUCommandBuffer(ctx->gpu_dev);
	if (!copy_cmd_buf) {
		SDL_Log("Failure to create copy cmd buffer: %s", SDL_GetError());
		return SDL_APP_FAILURE;
	}

	SDL_GPUCopyPass *copy_pass = SDL_BeginGPUCopyPass(copy_cmd_buf);
	if (!copy_pass) {
		SDL_Log("Failure to begin copy pass: %s", SDL_GetError());
		return SDL_APP_FAILURE;
	}

	SDL_UploadToGPUBuffer(copy_pass,
		(SDL_GPUTransferBufferLocation[]){{
			.transfer_buffer = transfer_buffer,
		}},
		(SDL_GPUBufferRegion[]){{
			.buffer = ctx->vertex_buffer,
			.size = vert_size,
		}},
		false);

	SDL_UploadToGPUBuffer(copy_pass,
		(SDL_GPUTransferBufferLocation[]){{
			.transfer_buffer = transfer_buffer,
			.offset = vert_size,
		}},
		(SDL_GPUBufferRegion[]){{
			.buffer = ctx->index_buffer,
			.size = index_size,
		}},
		false);

	SDL_UploadToGPUTexture(copy_pass,
		(SDL_GPUTextureTransferInfo[]){(SDL_GPUTextureTransferInfo){
			.transfer_buffer = tex_transfer_buffer,
		}},
		(SDL_GPUTextureRegion[]){(SDL_GPUTextureRegion){
			.texture = ctx->brick_texture,
			.w = brick_surface->w,
			.h = brick_surface->h,
			.d = 1,
		}},
		false);

	SDL_EndGPUCopyPass(copy_pass);

	bool ok = SDL_SubmitGPUCommandBuffer(copy_cmd_buf); SDL_assert(ok);

	SDL_ReleaseGPUTransferBuffer(ctx->gpu_dev, transfer_buffer);

	ctx->sampler = SDL_CreateGPUSampler(ctx->gpu_dev, (SDL_GPUSamplerCreateInfo[]){(SDL_GPUSamplerCreateInfo){
		.max_anisotropy = 16.0,
		.enable_anisotropy = true,
	}});
	if (!ctx->sampler) {
		SDL_Log("Failure to create texture sampler: %s", SDL_GetError());
		return SDL_APP_FAILURE;
	}

	ctx->pipeline = SDL_CreateGPUGraphicsPipeline(ctx->gpu_dev, (SDL_GPUGraphicsPipelineCreateInfo[]){{
		.vertex_shader = vert_shader,
		.fragment_shader = frag_shader,
		.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLESTRIP,
		.vertex_input_state = {
			.num_vertex_buffers = 1,
			.vertex_buffer_descriptions = (SDL_GPUVertexBufferDescription[]){{
				.slot = 0,
				.pitch = sizeof(VertexData),
			}},
			.num_vertex_attributes = 3,
			.vertex_attributes = (SDL_GPUVertexAttribute[]){
				{
					.location = 0,
					.format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3,
					.offset = (uint32_t)offsetof(VertexData, vertex),
				},
				{
					.location = 1,
					.format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4,
					.offset = (uint32_t)offsetof(VertexData, color),
				},
				{
					.location = 2,
					.format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2,
					.offset = (uint32_t)offsetof(VertexData, uv),
				},
			},
		},
		.target_info = {
			.num_color_targets = 1,
			.color_target_descriptions = (SDL_GPUColorTargetDescription[]){{
				.format = SDL_GetGPUSwapchainTextureFormat(ctx->gpu_dev, ctx->window),
			}},
		}
	}});
	if (!ctx->pipeline) {
		SDL_Log("Failure to create pipeline: %s", SDL_GetError());
		return SDL_APP_FAILURE;
	}

	SDL_DestroySurface(brick_surface);
	SDL_ReleaseGPUShader(ctx->gpu_dev, frag_shader);
	SDL_ReleaseGPUShader(ctx->gpu_dev, vert_shader);
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
	// Push this responsibility to gpu?
	glm_mat4_mul(ctx->projection, world->transform.model, ctx->ubo.mvp);
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
		SDL_BindGPUGraphicsPipeline(render_pass, pipeline);
		SDL_BindGPUVertexBuffers(render_pass, 0, (SDL_GPUBufferBinding[]){{ .buffer = ctx->vertex_buffer }}, 1);
		SDL_BindGPUIndexBuffer(render_pass, (SDL_GPUBufferBinding[]){{ .buffer = ctx->index_buffer }}, sizeof(uint16_t));
		SDL_PushGPUVertexUniformData(cmd_buf, 0, &ctx->ubo, sizeof(ctx->ubo));
		SDL_BindGPUFragmentSamplers(render_pass, 0, (SDL_GPUTextureSamplerBinding[]){{ .texture = ctx->brick_texture, .sampler = ctx->sampler }}, 1);
		SDL_DrawGPUIndexedPrimitives(render_pass, 6, 1, 0, 0, 0);
		SDL_EndGPURenderPass(render_pass);
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
