//
//  gfx_vulkan.cpp
//  libultraship
//
//  Created by David Chavez on 20.12.22.
//

#ifdef ENABLE_VULKAN

#ifndef _LANGUAGE_C
#define _LANGUAGE_C
#endif
#include "graphic/Fast3D/U64/PR/ultra64/gbi.h"

#include "gfx_cc.h"
#include "gfx_rendering_api.h"

#include "VkBootstrap.h"

// MARK: - Structs

struct ShaderProgramVulkan {
    uint64_t shader_id0;
    uint32_t shader_id1;

    uint8_t num_inputs;
    uint8_t num_floats;
    bool used_textures[2];
};

static struct {
    struct ShaderProgramVulkan* shader_program;
} vctx;

// MARK: - Public Methods

bool Vulkan_Init(void* instance) {
    ImGui_ImplVulkan_InitInfo init_info = {};
    init_info.Instance = g_Instance;
    init_info.PhysicalDevice = g_PhysicalDevice;
    init_info.Device = g_Device;
    init_info.QueueFamily = g_QueueFamily;
    init_info.Queue = g_Queue;
    init_info.PipelineCache = g_PipelineCache;
    init_info.DescriptorPool = g_DescriptorPool;
    init_info.Subpass = 0;
    init_info.MinImageCount = g_MinImageCount;
    init_info.ImageCount = wd->ImageCount;
    init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
    init_info.Allocator = g_Allocator;
    init_info.CheckVkResultFn = check_vk_result;
    ImGui_ImplVulkan_Init(&init_info, wd->RenderPass);

}

void Vulkan_RenderDrawData(ImDrawData* draw_data) {
    
}

// MARK: - Gfx API

static const char* gfx_vulkan_get_name() {
    return "Vulkan";
}

static struct GfxClipParameters gfx_vulkan_get_clip_parameters(float* clip_near, float* clip_far) {
//    return { false, framebuffers[current_framebuffer].invert_y };
}

static void gfx_vulkan_unload_shader(struct ShaderProgram* shader) {}

static void gfx_vulkan_load_shader(struct ShaderProgram* shader) {
    vctx.shader_program = (struct ShaderProgramVulkan *)shader;
}

static void gfx_vulkan_create_and_load_new_shader(uint64_t shader_id0, uint32_t shader_id1) {
    CCFeatures cc_features;
    gfx_cc_get_features(shader_id0, shader_id1, &cc_features);

    
}

struct GfxRenderingAPI gfx_vulkan_api = { gfx_vulkan_get_name,
                                          gfx_vulkan_get_clip_parameters,
                                          gfx_vulkan_unload_shader,
                                          gfx_vulkan_load_shader,
                                          gfx_vulkan_create_and_load_new_shader,
                                          gfx_vulkan_lookup_shader,
                                          gfx_vulkan_shader_get_info,
                                          gfx_vulkan_new_texture,
                                          gfx_vulkan_select_texture,
                                          gfx_vulkan_upload_texture,
                                          gfx_vulkan_set_sampler_parameters,
                                          gfx_vulkan_set_depth_test_and_mask,
                                          gfx_vulkan_set_zmode_decal,
                                          gfx_vulkan_set_viewport,
                                          gfx_vulkan_set_scissor,
                                          gfx_vulkan_set_use_alpha,
                                          gfx_vulkan_draw_triangles,
                                          gfx_vulkan_init,
                                          gfx_vulkan_on_resize,
                                          gfx_vulkan_start_frame,
                                          gfx_vulkan_end_frame,
                                          gfx_vulkan_finish_render,
                                          gfx_vulkan_create_framebuffer,
                                          gfx_vulkan_update_framebuffer_parameters,
                                          gfx_vulkan_start_draw_to_framebuffer,
                                          gfx_vulkan_clear_framebuffer,
                                          gfx_vulkan_resolve_msaa_color_buffer,
                                          gfx_vulkan_get_pixel_depth,
                                          gfx_vulkan_get_framebuffer_texture_id,
                                          gfx_vulkan_select_texture_fb,
                                          gfx_vulkan_delete_texture,
                                          gfx_vulkan_set_texture_filter,
                                          gfx_vulkan_get_texture_filter };

#endif

