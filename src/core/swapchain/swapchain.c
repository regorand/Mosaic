#include "swapchain.h"
#include "core/core.h"

#include <stdlib.h>

uint32_t msic_query_surface_support(MsicVulkanCore *core, MsicWindowContext *w_context) {
  MSIC_VK_CALL(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(core->physical_device, w_context->surface, &w_context->surface_capabilities));

  // Get number supported supported formats
  MSIC_VK_CALL(vkGetPhysicalDeviceSurfaceFormatsKHR(
        core->physical_device, 
        w_context->surface,
        &w_context->num_supported_formats,
        NULL));


  if (w_context->num_supported_formats != 0) {
    w_context->formats = malloc(w_context->num_supported_formats * sizeof(VkSurfaceFormatKHR *));
    
    // get supported surface formats
    MSIC_VK_CALL(vkGetPhysicalDeviceSurfaceFormatsKHR(
          core->physical_device,
          w_context->surface,
          &w_context->num_supported_formats,
          w_context->formats));
  }


  // Get number of supported present modes
  MSIC_VK_CALL(vkGetPhysicalDeviceSurfacePresentModesKHR(
        core->physical_device, 
        w_context->surface, 
        &w_context->num_present_modes, 
        NULL));

  if (w_context->num_present_modes != 0) {
    w_context->present_modes = malloc(w_context->num_present_modes * sizeof(VkPresentModeKHR));

    // Get supported present modes
    MSIC_VK_CALL(vkGetPhysicalDeviceSurfacePresentModesKHR(
          core->physical_device,
          w_context->surface, 
          &w_context->num_present_modes, 
          w_context->present_modes));
  }

  return 0;
}

bool msic_check_swapchain_support(MsicVulkanCore *core) {
  for (uint32_t i = 0; i < core->num_windows; i++) {
    MsicWindowContext *w_context = &core->windows[i];

    MSIC_CALL(msic_query_surface_support(core, w_context));

    if (w_context->num_supported_formats == 0) {
      MSIC_ERR("Surface %d supports no surface formats", i);
      return true;
    }

    if (msic_swapchain_formats != NULL && msic_swapchain_formats[i].format != VK_FORMAT_UNDEFINED) {
      bool supported = false;

      for (uint32_t j = 0; j < w_context->num_supported_formats; j++) {
        if (msic_swapchain_formats[i].format == w_context->formats[j].format &&
            msic_swapchain_formats[i].colorSpace == w_context->formats[j].colorSpace) {
          supported = true;
          break;
        }  
      }

      if (!supported) {
        MSIC_ERR("Requested surface format { %s, %s } for swapchain %d not supported", 
            string_VkFormat(        msic_swapchain_formats[i].format), 
            string_VkColorSpaceKHR( msic_swapchain_formats[i].colorSpace), 
            i); 
        return false;
      }
    }


    if (w_context->num_present_modes == 0) {
      MSIC_ERR("Surface %d supports no present modes", i);
      return true;
    }

    if (msic_present_modes != NULL) {
      bool supported = false;

      for (uint32_t j = 0; j < w_context->num_present_modes; j++) {
        if (msic_present_modes[i] == w_context->present_modes[j]) {
          supported = true;
          break;
        }  
      }

      if (!supported) {
        MSIC_ERR("Requested present mode %s for swapchain %d not supported", string_VkPresentModeKHR(msic_present_modes[i]), i); 
        return false;
      }
    }

    /*
    MSIC_VERBOSE("support for surface %d", i);

    MSIC_VERBOSE("\tVkSurfaceCapabilitiesKHR:");

    VkSurfaceCapabilitiesKHR *caps = &w_context->surface_capabilities;

    MSIC_VERBOSE("\t\tminImageCount: %u", caps->minImageCount);
    MSIC_VERBOSE("\t\tmaxImageCount: %u", caps->maxImageCount);

    MSIC_VERBOSE("\t\tcurrentExtent: %ux%u",
                 caps->currentExtent.width,
                 caps->currentExtent.height);
    MSIC_VERBOSE("\t\tminImageExtent: %ux%u",
                 caps->minImageExtent.width,
                 caps->minImageExtent.height);
    MSIC_VERBOSE("\t\tmaxImageExtent: %ux%u",
                 caps->maxImageExtent.width,
                 caps->maxImageExtent.height);

    MSIC_VERBOSE("\t\tmaxImageArrayLayers: %u", caps->maxImageArrayLayers);

    MSIC_VERBOSE("\t\tsupportedTransforms: 0x%08x", caps->supportedTransforms);
    MSIC_VERBOSE("\t\tcurrentTransform: 0x%08x", caps->currentTransform);
    MSIC_VERBOSE("\t\tsupportedCompositeAlpha: 0x%08x", caps->supportedCompositeAlpha);
    MSIC_VERBOSE("\t\tsupportedUsageFlags: 0x%08x", caps->supportedUsageFlags);

    MSIC_VERBOSE("\tFormats");
    for (uint32_t j = 0; j < w_context->num_supported_formats; j++) {
      MSIC_VERBOSE("\t\tsurface: %s\t\tcolorspace: %s", 
          string_VkFormat(w_context->formats[j].format), 
          string_VkColorSpaceKHR(w_context->formats[j].colorSpace));
    } 

    MSIC_VERBOSE("\tPresent Modes");
    for (uint32_t j = 0; j < w_context->num_present_modes; j++) {
      MSIC_VERBOSE("\t\tpresent_mode: %s", string_VkPresentModeKHR(w_context->present_modes[j])); 
    } 
    */
  } 

  return true;
}

uint32_t msic_create_swapchain(MsicVulkanCore *core) {
  MSIC_VERBOSE("Creating swapchain(s)");
  for (uint32_t i = 0; i < core->num_windows; i++) {
    MsicWindowContext *w_context = &core->windows[i];
    VkSurfaceCapabilitiesKHR caps = w_context->surface_capabilities;

    VkSurfaceFormatKHR format     = msic_choose_swapchain_format(w_context, i);
    VkPresentModeKHR present_mode = msic_choose_present_mode(w_context, i);
    VkExtent2D extent             = msic_choose_swap_extent(w_context, i);

    uint32_t image_count          = msic_choose_image_count(w_context, i); 

    VkSwapchainCreateInfoKHR create_info;

    create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    create_info.pNext = NULL;
    create_info.flags = 0;
    create_info.surface = w_context->surface;
    create_info.minImageCount = image_count;
    create_info.imageFormat = format.format;
    create_info.imageColorSpace = format.colorSpace;
    create_info.imageExtent = extent;

    if (msic_swapchain_image_layers != NULL) {
      create_info.imageArrayLayers = msic_swapchain_image_layers[i];
    } else {
      create_info.imageArrayLayers = 1;
    }

    if (msic_swapchain_image_usage_flags != NULL) {
      create_info.imageUsage = msic_swapchain_image_usage_flags[i];
    } else {
      create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    }

    if (core->present_graphics_match) {
      create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
      create_info.queueFamilyIndexCount = 0;
      create_info.pQueueFamilyIndices = NULL;
    } else {
      create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
      if (msic_graphics) {
        uint32_t queue_indices[2] = { core->present_queue_index, core->graphics_queue_index };
        uint32_t num_queues = 2;

        create_info.queueFamilyIndexCount = num_queues;
        create_info.pQueueFamilyIndices = queue_indices;
      } else {
        create_info.queueFamilyIndexCount = 1;
        create_info.pQueueFamilyIndices = &core->present_queue_index;
      }
    }

    // TODO Possible control parameter
    create_info.preTransform = caps.currentTransform;
    
    // TODO Possible control parameter
    create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

    create_info.presentMode = present_mode;

    // TODO Possible control parameter
    create_info.clipped = VK_TRUE;

    // TODO Can allow for hot recreation of swapchain through this field ?
    create_info.oldSwapchain = VK_NULL_HANDLE;

    MSIC_VK_CALL(vkCreateSwapchainKHR(core->device, &create_info, NULL, &w_context->swapchain));
    MSIC_VERBOSE("Successfully created swapchain for window %d", i);

    w_context->chosen_swapchain_format = format.format;
    w_context->current_swap_extent = extent; 
    
    MSIC_VK_CALL(vkGetSwapchainImagesKHR(core->device, w_context->swapchain, &w_context->num_swap_images, NULL));

    w_context->swap_images = malloc(w_context->num_swap_images * sizeof(VkImage));
    MSIC_VK_CALL(vkGetSwapchainImagesKHR(core->device, w_context->swapchain, &w_context->num_swap_images, w_context->swap_images));
    MSIC_VERBOSE("Retrived swapchain images for window %d", i);
  }

  MSIC_INFO("Created %d swapchain(s)", core->num_windows);

  return 0;
}

VkSurfaceFormatKHR msic_default_choose_swapchain_format(MsicWindowContext *w_context, uint32_t index) {
  // If user specifed a format choose that one
  // If surface format field VkFormat field is set to VK_FORMAT_UNDEFINED, choose another
  if (msic_swapchain_formats != NULL && msic_swapchain_formats[index].format != VK_FORMAT_UNDEFINED) {
    return msic_swapchain_formats[index];
  }

  // Check for some probably reasonable default format
  for (uint32_t i = 0; i < w_context->num_supported_formats; i++) {
    VkSurfaceFormatKHR format = w_context->formats[i];

    if (format.format == VK_FORMAT_B8G8R8A8_SRGB && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
      return format;
    }
  }

  // As last resort use first supported format 
  // Guaranteed we have this because w_context->num_supported_formats > 0 is checked earlier
  return w_context->formats[0];
}

VkPresentModeKHR msic_default_choose_present_mode(MsicWindowContext *w_context, uint32_t index) {
  // If user specified a present mode choose that one
  if (msic_present_modes != NULL) {
    return msic_present_modes[index];
  } 

  // Check if Mailbox present mode is present as preferred default
  for (uint32_t i = 0; i < w_context->num_present_modes; i++) {
    if (w_context->present_modes[i] == VK_PRESENT_MODE_MAILBOX_KHR) {
      return w_context->present_modes[i];
    }
  }

  // Otherwise, this should always be supported
  return VK_PRESENT_MODE_FIFO_KHR;
}

// swap extent choice function adapted to C from https://vulkan-tutorial.com/Drawing_a_triangle/Presentation/Swap_chain
VkExtent2D msic_default_choose_swap_extent(MsicWindowContext *w_context, uint32_t index) {
  UNUSED(index);
  VkSurfaceCapabilitiesKHR caps = w_context->surface_capabilities;

  if (caps.currentExtent.width != UINT32_MAX) {
    return caps.currentExtent;
  } else {
    int width, height; 

    glfwGetFramebufferSize(w_context->window, &width, &height);

    VkExtent2D actualExtent = {
        (uint32_t)(width),
        (uint32_t)(height)
    };

    actualExtent.width  = clamp(actualExtent.width,  caps.minImageExtent.width,  caps.maxImageExtent.width);
    actualExtent.height = clamp(actualExtent.height, caps.minImageExtent.height, caps.maxImageExtent.height);

    return actualExtent;
  }
}

uint32_t msic_choose_image_count(MsicWindowContext *w_context, uint32_t index){ 
  UNUSED(index);
  VkSurfaceCapabilitiesKHR caps = w_context->surface_capabilities;

  // Target three images to support triple buffering 
  uint32_t images = 3;

  // clamp image count, consider maxImageCount exception for 0
  if (                           caps.minImageCount > images) images = caps.minImageCount;
  if (caps.maxImageCount != 0 && caps.maxImageCount < images) images = caps.maxImageCount;

  return images;
}

uint32_t msic_create_swapchain_imageviews(MsicVulkanCore *core) {
  MSIC_VERBOSE("Creating swapchain image views");
  for (uint32_t i = 0; i < core->num_windows; i++) {
    MsicWindowContext *w_context = &core->windows[i];

    w_context->swap_image_views = malloc(w_context->num_swap_images * sizeof(VkImageView));

    VkImageViewCreateInfo create_info;
    create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    create_info.pNext = NULL;
    create_info.flags = 0;
    create_info.image = w_context->swap_images[i];
    create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    create_info.format = w_context->chosen_swapchain_format;

    create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

    create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    create_info.subresourceRange.baseMipLevel = 0;
    create_info.subresourceRange.levelCount = 1;
    create_info.subresourceRange.baseArrayLayer = 0;
    create_info.subresourceRange.layerCount = 1;

    MSIC_VK_CALL(vkCreateImageView(core->device, &create_info, NULL, &w_context->swap_image_views[i]));
  }

  return 0;
}
