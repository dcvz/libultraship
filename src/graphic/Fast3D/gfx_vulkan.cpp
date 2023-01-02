//
//  gfx_vulkan.cpp
//  libultraship
//
//  Created by David Chavez on 20.12.22.
//

#ifdef ENABLE_VULKAN

#include "gfx_vulkan.h"

#ifndef _LANGUAGE_C
#define _LANGUAGE_C
#endif
#include "graphic/Fast3D/U64/PR/ultra64/gbi.h"

#include <VkBootstrap.h>
#include <spdlog/spdlog.h>
#include <ImGui/backends/imgui_impl_vulkan.h>
#include <glslang/Public/ShaderLang.h>


#include "gfx_cc.h"
#include "gfx_rendering_api.h"

// MARK: - Structs

struct Init {
    SDL_Window* window;
    vkb::Instance instance;
    VkSurfaceKHR surface;
    vkb::Device device;
    vkb::Swapchain swapchain;
};

struct FramebufferVulkan {
    VkCommandPool       command_pool;
    VkCommandBuffer     command_buffer;
    VkFence             fence;
    VkImage             back_buffer;
    VkImageView         back_buffer_view;
    VkFramebuffer       framebuffer;
};

struct ShaderProgramVulkan {
    uint64_t shader_id0;
    uint32_t shader_id1;

    uint8_t num_inputs;
    uint8_t num_floats;
    bool used_textures[2];
};

struct ShaderModuleVulkan {
  std::vector<unsigned int> SPIRV;
  VkShaderModule shaderModule;
};

static struct {
    // Elements that are only setup once
    Init init;
    VkRenderPass render_pass;

    std::vector<FramebufferVulkan> framebuffers;

    // Current state
    struct ShaderProgramVulkan* shader_program;

    int current_framebuffer;
} vctx;

// MARK: - Helpers

int device_initialization() {
    // Select physical device
    vkb::PhysicalDeviceSelector phys_device_selector (vctx.init.instance);
    // TODO: Maybe we have to handle multiple GPU? And select the discrete vs integrated (see ImGui ex).
    auto phys_device_ret = phys_device_selector.set_surface(vctx.init.surface).select();
    if (!phys_device_ret) {
        SPDLOG_ERROR("Failed to select physical device: {}", phys_device_ret.error ().message ());
        return -1;
    }
    vkb::PhysicalDevice physical_device = phys_device_ret.value();

    // Select logical device
    vkb::DeviceBuilder device_builder{ physical_device };
    auto device_ret = device_builder.build();
    if (!device_ret) {
        SPDLOG_ERROR("Failed to create logical device: {}", device_ret.error ().message ());
        return -1;
    }

    vctx.init.device = device_ret.value();
}

int create_swapchain() {
    vkb::SwapchainBuilder swapchain_builder{ vctx.init.device };
    auto swap_ret = swapchain_builder
        .set_desired_format({VK_FORMAT_B8G8R8A8_UNORM, VK_COLORSPACE_SRGB_NONLINEAR_KHR})
        .set_desired_format({VK_FORMAT_R8G8B8A8_UNORM, VK_COLORSPACE_SRGB_NONLINEAR_KHR})
        .set_desired_format({VK_FORMAT_B8G8R8_UNORM, VK_COLORSPACE_SRGB_NONLINEAR_KHR})
        .set_desired_format({VK_FORMAT_R8G8B8_UNORM, VK_COLORSPACE_SRGB_NONLINEAR_KHR})
        .set_old_swapchain(vctx.init.swapchain)
        .build();

    if (!swap_ret) {
        SPDLOG_ERROR("Failed to create swapchain: {} - {}", swap_ret.error().message(), swap_ret.vk_result());
        return -1;
    }

    vkb::destroy_swapchain(vctx.init.swapchain);
    vctx.init.swapchain = swap_ret.value ();
    return 0;
}

int get_queues(ImGui_ImplVulkan_InitInfo& init_info) {
    auto gqi = vctx.init.device.get_queue_index (vkb::QueueType::graphics);
    if (!gqi.has_value()) {
        SPDLOG_ERROR("Failed to get graphics queue: {}", gqi.error().message());
        return -1;
    }

    auto gq = vctx.init.device.get_queue(vkb::QueueType::graphics);
    if (!gq.has_value()) {
        SPDLOG_ERROR("Failed to get queue: {}", gq.error().message());
        return -1;
    }

    init_info.QueueFamily = gqi.value();
    init_info.Queue = gq.value();
}

int create_descriptor_pool(ImGui_ImplVulkan_InitInfo& init_info) {
    VkDescriptorPoolSize pool_sizes[] = {
        { VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
        { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
        { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
    };
    VkDescriptorPoolCreateInfo pool_info = {};
    pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    pool_info.maxSets = 1000 * IM_ARRAYSIZE(pool_sizes);
    pool_info.poolSizeCount = (uint32_t)IM_ARRAYSIZE(pool_sizes);
    pool_info.pPoolSizes = pool_sizes;

    VkDescriptorPool imguiPool;
    VkResult err = vkCreateDescriptorPool(vctx.init.device.device, &pool_info, nullptr, &imguiPool);
    if (err != VK_SUCCESS) {
        SPDLOG_ERROR("Failed to create descriptor pool: {}", err);
        return -1;
    }

    init_info.DescriptorPool = imguiPool;
}

int create_render_pass (Init& init) {
    VkAttachmentDescription color_attachment = {};
    color_attachment.format = init.swapchain.image_format;
    color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
    color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    color_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference color_attachment_ref = {};
    color_attachment_ref.attachment = 0;
    color_attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &color_attachment_ref;

    VkSubpassDependency dependency = {};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo render_pass_info = {};
    render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    render_pass_info.attachmentCount = 1;
    render_pass_info.pAttachments = &color_attachment;
    render_pass_info.subpassCount = 1;
    render_pass_info.pSubpasses = &subpass;
    render_pass_info.dependencyCount = 1;
    render_pass_info.pDependencies = &dependency;

    VkResult err = vkCreateRenderPass(init.device.device, &render_pass_info, nullptr, &vctx.render_pass);
    if (err != VK_SUCCESS) {
        SPDLOG_ERROR("Failed to create render pass: {}", err);
        return -1;
    }

    return 0;
}

std::string generate_glsl(uint64_t shader_id0, uint32_t shader_id1, const CCFeatures& cc_features) {
    int vertex_index = 0;

    std::string cc_vertex = "#version 450\n";
    cc_vertex += fmt::format("layout(location = {}) in vec4 aPosition;\n", vertex_index++);

    for (int i = 0; i < 2; i++) {
        if (cc_features.used_textures[i]) {
            cc_vertex += fmt::format("layout(location = {}) in vec2 aTexCoord{};\n", vertex_index, i);
            cc_vertex += fmt::format("layout(location = {}) out vec2 vTexCoord{};\n", vertex_index++, i);

            for (int j = 0; j < 2; j++) {
                if (cc_features.clamp[i][j]) {
                    cc_vertex += fmt::format("layout(location = {}) in float aTexClamp{}{};\n", vertex_index, j == 0 ? "S" : "T", i);
                    cc_vertex += fmt::format("layout(location = {}) out float vTexClamp{}{};\n", vertex_index++, j == 0 ? "S" : "T", i);
                }
        }
    }

    if (cc_features.opt_fog) {
        cc_vertex += fmt::format("layout(location = {}) in float aFog;\n", vertex_index);
        cc_vertex += fmt::format("layout(location = {}) out float vFog;\n", vertex_index++);
    }

     if (cc_features.opt_grayscale) {
        cc_vertex += fmt::format("layout(location = {}) in float aGrayscale;\n", vertex_index);
        cc_vertex += fmt::format("layout(location = {}) out float vGrayscale;\n", vertex_index++);
    }

    for (int i = 0; i < cc_features.num_inputs; i++) {
        cc_vertex += fmt::format("layout(location = {}) in vec{} aInput{};\n", vertex_index, cc_features.opt_alpha ? 4 : 3, i);
        cc_vertex += fmt::format("layout(location = {}) out vec{} vInput{};\n", vertex_index++, cc_features.opt_alpha ? 4 : 3, i);
    }

    // start void main
    cc_vertex += "void main() {\n";
    for (int i = 0; i < 2; i++) {
        if (cc_features.used_textures[i]) {
            cc_vertex += fmt::format("    vTexCoord{} = aTexCoord{};\n", i, i);
            for (int j = 0; j < 2; j++) {
                if (cc_features.clamp[i][j]) {
                    cc_vertex += fmt::format("    vTexClamp{}{} = aTexClamp{}{};\n", j == 0 ? "S" : "T", i, j == 0 ? "S" : "T", i);
                }
            }
        }
    }

    if (cc_features.opt_fog) {
        cc_vertex += "    vFog = aFog;\n";
    }

    if (cc_features.opt_grayscale) {
        cc_vertex += "    vGrayscale = aGrayscale;\n";
    }

    for (int i = 0; i < cc_features.num_inputs; i++) {
        cc_vertex += fmt::format("    vInput{} = aInput{};\n", i + 1, i + 1);
    }

    cc_vertex += "    gl_Position = aPosition;\n";
    cc_vertex += "}\n";

    std::string cc_fragment = "#version 450\n";

    // layout(location = 0) in vec3 fragColor;
    // layout(location = 0) out vec4 outColor;
}

// MARK: - Public Methods

bool Vulkan_Init(SDL_Window* window, void* instance, void* surface) {
    vctx.init.window = window;
    vctx.init.instance = *static_cast<vkb::Instance *>(instance);
    vctx.init.surface = static_cast<VkSurfaceKHR>(surface);

    if (0 != device_initialization()) abort();
    if (0 != create_swapchain()) abort();

    // Create ImGui creation object
    ImGui_ImplVulkan_InitInfo init_info = {};
    init_info.Instance = vctx.init.instance.instance;
    init_info.PhysicalDevice = vctx.init.device.physical_device;
    init_info.Device = vctx.init.device.device;

    if (0 != get_queues(init_info)) abort();
    if (0 != create_descriptor_pool(init_info)) abort();

    init_info.MinImageCount = vctx.init.swapchain.requested_min_image_count;
    init_info.ImageCount = vctx.init.swapchain.image_count;
    init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
    ImGui_ImplVulkan_Init(&init_info, vctx.render_pass);
}

void Vulkan_RenderDrawData(ImDrawData* draw_data) {
    auto framebuffer = vctx.framebuffers[0];
    ImGui_ImplVulkan_RenderDrawData(draw_data, framebuffer.command_buffer);
}

// MARK: - Gfx API

static const char* gfx_vulkan_get_name() {
    return "Vulkan";
}

static struct GfxClipParameters gfx_vulkan_get_clip_parameters(float* clip_near, float* clip_far) {
    return { true, false };
}

static void gfx_vulkan_unload_shader(struct ShaderProgram* shader) {}

static void gfx_vulkan_load_shader(struct ShaderProgram* shader) {
    vctx.shader_program = (struct ShaderProgramVulkan *)shader;
}

static void gfx_vulkan_create_and_load_new_shader(uint64_t shader_id0, uint32_t shader_id1) {
    CCFeatures cc_features;
    gfx_cc_get_features(shader_id0, shader_id1, &cc_features);

    // generate shader with glslang
    std::string glsl = gfx_cc_generate_glsl(shader_id0, shader_id1, cc_features);
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

