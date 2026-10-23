#ifndef MSIC_SWAPCHAIN_H
#define MSIC_SWAPCHAIN_H

#include "mosaic.h"

#include <stdint.h>
#include <stdbool.h>

uint32_t msic_query_surface_support(MsicVulkanCore *core, MsicWindowContext *w_context);

bool msic_check_swapchain_support(MsicVulkanCore *core);
uint32_t msic_create_swapchain(MsicVulkanCore *core);
void destroy_swapchain(MsicVulkanCore *core, VkSwapchainKHR swapchain);

VkSurfaceFormatKHR msic_default_choose_swapchain_format(MsicWindowContext *w_context, uint32_t index);
VkPresentModeKHR msic_default_choose_present_mode(MsicWindowContext *w_context, uint32_t index);
VkExtent2D msic_default_choose_swap_extent(MsicWindowContext *w_context, uint32_t index);

uint32_t msic_choose_image_count(MsicWindowContext *w_context, uint32_t index);

#endif // MSIC_SWAPCHAIN_H
