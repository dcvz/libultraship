//
//  gfx_vulkan.h
//  libultraship
//
//  Created by David Chavez on 20.12.22.
//

#ifdef ENABLE_VULKAN

#ifndef GFX_VULKAN_H
#define GFX_VULKAN_H

#include "gfx_rendering_api.h"
#include <ImGui/imgui.h>

extern struct GfxRenderingAPI gfx_vulkan_api;

bool Vulkan_Init(void* instance);
void Vulkan_RenderDrawData(ImDrawData* draw_data);

#endif

#endif
