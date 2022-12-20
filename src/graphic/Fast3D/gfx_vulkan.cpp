//
//  gfx_vulkan.cpp
//  libultraship
//
//  Created by David Chavez on 20.12.22.
//

#ifdef ENABLE_VULKAN

static const char* gfx_vulkan_get_name() {
    return "Vulkan";
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

