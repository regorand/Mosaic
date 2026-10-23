#include "control_params.h"
#include "core.h"
#include "mosaic.h"

#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <vulkan/vulkan.h>
#include <vulkan/vk_enum_string_helper.h>
#include <GLFW/glfw3.h>

#define clamp(val, min, max) ((val) < (min) ? (min) : ((val) > (max) ? (max) : (val)))

void msic_default_log_callback(const char *file, uint32_t line, const char *message, uint32_t severity) {
  if (severity >= msic_log_level) {
    printf("mosaic | %20s:%-5d | %7s | %s\n", file, line, msic_severity_strings[severity], message);
  }
}

VKAPI_ATTR VkBool32 VKAPI_CALL msic_vk_message_callback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData) {

  UNUSED(messageType);
  UNUSED(pUserData);

  uint32_t msic_severity = VK_SEVERITY_TO_MSIC_SEVERITY(messageSeverity);
  msic_output_log("vk validation", 0, msic_severity, pCallbackData->pMessage);

  return VK_FALSE;
}

void msic_glfw_callback(int error_code, const char* message) {
  msic_output_log("glfw error", 0, MSIC_SEVERITY_ERR, "glfw error code: %d: %s", error_code, message); 
}

void msic_populate_debug_messenger_create_info(VkDebugUtilsMessengerCreateInfoEXT *create_info) {
  create_info->sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT; 
  
  create_info->messageSeverity =    VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT 
                                | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT;
                              //| VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT 
                              //| VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT; // TODO possible control parameter

  create_info->messageType     =    VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
                                | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
                                | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT; // TODO possible control parameter
                                                                                   
  create_info->pfnUserCallback = &msic_vk_message_callback;
  create_info->pUserData = NULL; // TODO possible control parameter
} 

uint32_t msic_setup_debug_messenger(MsicVulkanCore *core) {
  VkDebugUtilsMessengerCreateInfoEXT create_info = {0};
  msic_populate_debug_messenger_create_info(&create_info);

  PFN_vkCreateDebugUtilsMessengerEXT func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(core->instance, "vkCreateDebugUtilsMessengerEXT");
  if (!func) {
    MSIC_ERR("Could not load vkCreateDebugUtilsMessengerEXT");
    return 1;
  }

  MSIC_VK_CALL(func(core->instance, &create_info, NULL, &core->debug_messenger));
  return 0;
}

bool msic_check_layer(VkLayerProperties *layer_props, uint32_t layer_count, const char * layer_name) {
  for (uint32_t i = 0; i < layer_count; i++) {
    if (strcmp(layer_props[i].layerName, layer_name)) {
      return true;
    }
  }

  return false;
}

bool msic_check_layer_support(void) {
  uint32_t layer_count;
  MSIC_VK_CALL(vkEnumerateInstanceLayerProperties(&layer_count, NULL));

  VkLayerProperties layer_props[layer_count];
  MSIC_VK_CALL(vkEnumerateInstanceLayerProperties(&layer_count, layer_props));
 
  if (msic_enable_validation) {
    if (!msic_check_layer(layer_props, layer_count, MSIC_VK_VALIDATION_LAYER_NAME)) {
      MSIC_ERR("Vk Validation layer not supported!");
      return false;
    }
  } 
    
  if (msic_layer_count > 0 && msic_layers == NULL) {
    MSIC_ERR("Requested %d layers but provided no names to msic_layers**", msic_layer_count);
    return false;
  }

  for (uint32_t i = 0; i < msic_layer_count; i++) {
    char *layer = msic_layers[i];
    if (!msic_check_layer(layer_props, layer_count, layer))  {
      MSIC_ERR("Requested layer %s is not available", layer);
      return false;
    }
  }

  return true;
}

bool msic_check_extension(VkExtensionProperties *extension_props, uint32_t extension_count, const char * extension_name) {
  for (uint32_t i = 0; i < extension_count; i++) {
    if (strcmp(extension_props[i].extensionName, extension_name)) {
      return true;
    }
  }

  return false;
}

bool msic_check_extension_support(void) {
  uint32_t extension_count;
  MSIC_VK_CALL(vkEnumerateInstanceExtensionProperties(NULL, &extension_count, NULL));

  VkExtensionProperties extension_props[extension_count];
  MSIC_VK_CALL(vkEnumerateInstanceExtensionProperties(NULL, &extension_count, extension_props));
 
  if (msic_extension_count > 0 && msic_extensions == NULL) {
    MSIC_ERR("Requested %d extensions but provided no names to msic_extensions**", msic_extension_count);
    return false;
  }

  if (!msic_headless) {
    if (!msic_check_extension(extension_props, extension_count, VK_KHR_SURFACE_EXTENSION_NAME)) {
      MSIC_ERR("VK_KHR_SURFACE extensions not supported!");
      return false;
    }
  } 

  for (uint32_t i = 0; i < msic_extension_count; i++) {
    char *extension = msic_extensions[i];
    if (!msic_check_extension(extension_props, extension_count, extension))  {
      MSIC_ERR("Requested extension %s is not available", extension);
      return false;
    }
  }

  return true;
}

bool msic_check_device_extension_support(VkPhysicalDevice device) {
  uint32_t extension_count;
  MSIC_VK_CALL(vkEnumerateDeviceExtensionProperties(device, NULL, &extension_count, NULL));

  VkExtensionProperties extension_props[extension_count];
  MSIC_VK_CALL(vkEnumerateDeviceExtensionProperties(device, NULL, &extension_count, extension_props));
 
  if (msic_device_extension_count > 0 && msic_device_extensions == NULL) {
    MSIC_VERBOSE("\tRequested %d device extensions but provided no names to msic_device_extensions**", msic_device_extension_count);
    return false;
  }

  if (!msic_headless) {
    if (!msic_check_extension(extension_props, extension_count, VK_KHR_SWAPCHAIN_EXTENSION_NAME)) {
      MSIC_ERR("VK_KHR_Swapchain extension not supported!");
      return false;
    } 
  } 

  for (uint32_t i = 0; i < msic_device_extension_count; i++) {
    char *extension = msic_device_extensions[i];
    if (!msic_check_extension(extension_props, extension_count, extension))  {
      MSIC_VERBOSE("\tRequested device extension %s is not available", extension);
      return false;
    }
  }

  return true;
}

uint32_t msic_write_extensions(uint32_t extension_count, const char** pp_extension_names) {
  memcpy(pp_extension_names, msic_extensions, msic_extension_count * sizeof(char *));

  uint32_t extra_extensions = 0;

  if (msic_enable_validation) extra_extensions++;
  if (!msic_headless)         extra_extensions++;

  if (extension_count < msic_extension_count + extra_extensions) {
    // TODO could give better error message -> which extensions can be added automatically, which are, etc.
    MSIC_ERR("extension_count not large enough to account for msic_extension_count and automatically added extensions");
    return 1;
  }

  uint32_t current_extra_offset = 0;

  if (msic_enable_validation) {
    pp_extension_names[msic_extension_count + current_extra_offset] = VK_EXT_DEBUG_UTILS_EXTENSION_NAME; 
    current_extra_offset++;
  }

  if (!msic_headless) {
    pp_extension_names[msic_extension_count + current_extra_offset] = VK_KHR_SURFACE_EXTENSION_NAME; 
    current_extra_offset++;
  }


  return 0;
}

uint32_t msic_write_layers(uint32_t layer_count, const char** pp_layer_names) {
  memcpy(pp_layer_names, msic_layers, msic_layer_count * sizeof(char *));

  if (msic_enable_validation) {
    if (layer_count < msic_layer_count + 1) {
      MSIC_ERR("layer_count not large enough for msic_layer_count and the validation layer");
      return 1;
    }

    pp_layer_names[msic_layer_count] = MSIC_VK_VALIDATION_LAYER_NAME; 
  }

  return 0;
}

uint32_t msic_write_device_extensions(uint32_t extension_count, const char** pp_extension_names) {
  memcpy(pp_extension_names, msic_extensions, msic_device_extension_count * sizeof(char *));

  uint32_t extra_extensions = 0;
  if (!msic_headless)         extra_extensions++;

  if (extension_count < msic_device_extension_count + extra_extensions) {
    // TODO could give better error message -> which extensions can be added automatically, which are, etc.
    MSIC_ERR("extension_count not large enough to account for msic_device_extension_count and automatically added extensions");
    return 1;
  }

  uint32_t current_extra_offset = 0;

  if (!msic_headless) {
    pp_extension_names[msic_device_extension_count + current_extra_offset] = VK_KHR_SWAPCHAIN_EXTENSION_NAME; 
    current_extra_offset++;
  }

  return 0;
}

uint32_t msic_create_instance(MsicVulkanCore *core) {
  MSIC_INFO("Creating Vulkan Instance");

  VkApplicationInfo appInfo;
  appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  appInfo.pApplicationName = msic_application_name; 
  appInfo.applicationVersion = msic_application_version;
  appInfo.pEngineName = msic_engine_name;
  appInfo.engineVersion = msic_engine_version;
  appInfo.apiVersion = msic_vk_api_version; 

  VkInstanceCreateInfo createInfo;
  createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  createInfo.pApplicationInfo = &appInfo;


  
  createInfo.enabledExtensionCount = msic_extension_count;
  
  if (msic_enable_validation) createInfo.enabledExtensionCount++;
  if (!msic_headless)         createInfo.enabledExtensionCount++;

  const char *extension_names[createInfo.enabledExtensionCount];

  if (createInfo.enabledExtensionCount > 0) {
    
    if (msic_write_extensions(createInfo.enabledExtensionCount, extension_names)) {
      MSIC_ERR("Failed to write vulkan extension names");
      return 1;
    }
    
    createInfo.ppEnabledExtensionNames = (const char * const *) extension_names;

    for (uint32_t i = 0; i < createInfo.enabledExtensionCount; i++) {
      MSIC_VERBOSE("\tEnabling extension: %s", extension_names[i]);
    }

  } else {
    createInfo.ppEnabledExtensionNames = NULL;
  }



  createInfo.enabledLayerCount = msic_layer_count + msic_enable_validation;
  const char *layer_names[createInfo.enabledLayerCount]; 

  if (createInfo.enabledLayerCount > 0) {
    if (msic_write_layers(createInfo.enabledLayerCount, layer_names)) {
      MSIC_ERR("Failed to write vulkan layer names");
      return 1;
    }

    createInfo.ppEnabledLayerNames = (const char * const *) layer_names;

    for (uint32_t i = 0; i < createInfo.enabledLayerCount; i++) {
      MSIC_VERBOSE("\tEnabling layer: %s", layer_names[i]);
    }

  } else {
    createInfo.ppEnabledLayerNames = NULL;
  }


  if (msic_enable_validation) {
    VkDebugUtilsMessengerCreateInfoEXT debug_create_info = {0};
    msic_populate_debug_messenger_create_info(&debug_create_info);
    createInfo.pNext = &debug_create_info; 
  }

  
  MSIC_VK_CALL(vkCreateInstance(&createInfo, NULL, &core->instance));

  return 0;
}

uint32_t msic_init_glfw() {
  if (!glfwInit()) {
    MSIC_ERR("Failed to initialize glfw");
    return 1;
  }
  
  glfwSetErrorCallback(&msic_glfw_callback);

  return 0;
}

void msic_populate_empty_window_context(MsicWindowContext *w_context) {
  w_context->open          = false;
  w_context->surface       = NULL;
  w_context->window        = NULL;
  w_context->formats       = NULL;
  w_context->present_modes = NULL;
  w_context->swapchain     = NULL;
}

uint32_t msic_create_windows(MsicVulkanCore *core) {
  int glfw_supported = glfwVulkanSupported();
  if (glfw_supported != GLFW_TRUE) {
    MSIC_ERR("GLFW support query returned %d", glfw_supported);
    return 1;
  }

  core->windows = malloc(msic_num_windows * sizeof(MsicWindowContext));

  for (uint32_t i = 0; i < msic_num_windows; i++) {
    msic_populate_empty_window_context(&core->windows[i]);
    glfwDefaultWindowHints();

    // No OpenGL context
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    // XXX temporary, resizing not implemented yet
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    if (msic_window_hint_counts != NULL) {
      for (uint32_t j = 0; j < msic_window_hint_counts[i]; j++) {
        glfwWindowHint(msic_window_hints[i][j], msic_window_hint_values[i][j]);
      }
    }

    GLFWwindow *w = glfwCreateWindow(800, 500, msic_application_name, NULL, NULL);
    if (w == NULL) {
      MSIC_ERR("Encountered fatal error creating glfw window, aborting");
      return 1;
    }

    core->windows[i].window = w;
    core->windows[i].open   = true;
  }

  core->num_windows = msic_num_windows;
  MSIC_VERBOSE("Created %d window(s)", core->num_windows);

  return 0;
}

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

uint32_t msic_create_surfaces(MsicVulkanCore *core) {
  for (uint32_t i = 0; i < msic_num_windows; i++) {
    MsicWindowContext *w_context = &core->windows[i];
    MSIC_VK_CALL(glfwCreateWindowSurface(core->instance, w_context->window, NULL, &w_context->surface)); 

  }

  MSIC_VERBOSE("Created %d surface(s)", core->num_windows);
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

uint32_t msic_create_physical_device(MsicVulkanCore *core) {
  UNUSED(core);
  uint32_t device_count = 0;
  MSIC_VK_CALL(vkEnumeratePhysicalDevices(core->instance, &device_count, NULL));

  if (device_count == 0) {
    MSIC_ERR("No devices with vulkan support found!");
    return 1;
  }
  
  MSIC_INFO("Found %d devices", device_count);

  VkPhysicalDevice devices[device_count];
  MSIC_VK_CALL(vkEnumeratePhysicalDevices(core->instance, &device_count, devices));

  uint32_t  arr_present_indices[device_count];
  uint32_t  arr_graphics_indices[device_count];
  bool      arr_present_graphics_match[device_count];
  uint32_t *arr_queue_indices[device_count];
  bool      suitable[device_count];

  for (uint32_t i = 0; i < device_count; i++) {
    arr_queue_indices[i] = malloc(msic_num_extra_queue_families * sizeof(uint32_t));
  } 

  uint32_t num_suitable = 0;

  for (uint32_t i = 0; i < device_count; i++) {
    suitable[i] = (*msic_phys_device_suitable)(devices[i], 
                                               &(arr_present_indices[i]), 
                                               &(arr_graphics_indices[i]), 
                                               &(arr_present_graphics_match[i]), 
                                               &(arr_queue_indices[i]), 
                                               core);

    if (suitable[i]) num_suitable++;
  }

  if (num_suitable == 0) {
    MSIC_ERR("No suitable device was found");
    return 1;
  }

  uint32_t chosen_device_index = (*msic_default_choose_physical_device)(device_count, devices, suitable);
  
  // Suitable devices that were not chosen still have their queue indices malloc'ed, free them here
  for (uint32_t i = 0; i < device_count; i++) {
    if (suitable[i] && i != chosen_device_index) {
      free(arr_queue_indices[i]);
    }
  }


  VkPhysicalDeviceProperties props;
  vkGetPhysicalDeviceProperties(devices[chosen_device_index], &props);

  MSIC_INFO("Chose device %s with index %d", props.deviceName, chosen_device_index);
  
  core->physical_device          = devices[chosen_device_index];
  core->num_device_queue_indices = msic_num_extra_queue_families;
  if (msic_present && msic_graphics) {
    core->present_graphics_match = arr_present_graphics_match[chosen_device_index];
  } else {
    core->present_graphics_match = false;
  }
  core->device_queue_indices     = arr_queue_indices[chosen_device_index];

  return 0;
}

bool msic_default_phys_device_suitable(VkPhysicalDevice device, 
                                        uint32_t *present_queue_index, 
                                        uint32_t *graphics_queue_index,
                                        bool     *present_graphics_match,
                                        uint32_t **queue_indices, 
                                        MsicVulkanCore *core) {

  VkPhysicalDeviceProperties props;
  vkGetPhysicalDeviceProperties(device, &props);

  bool suitable = true;
  
  MSIC_INFO("Device %s: Checking suitability", props.deviceName);

  // --------------
  // Device type
  // --------------
  if (msic_physical_device_type != -1) {
    if ((int32_t) props.deviceType != msic_physical_device_type) {
      MSIC_VERBOSE("\tType %s(%d) not equal to requested device type %s(%d)", string_VkPhysicalDeviceType(props.deviceType), props.deviceType, string_VkPhysicalDeviceType(msic_physical_device_type), msic_physical_device_type);
      suitable = false;
    }
  }  

  // --------------
  // Device features 
  // --------------
  //
  VkPhysicalDeviceFeatures features;
  vkGetPhysicalDeviceFeatures(device, &features);

  bool all_requested_supported = true;

  // Small hack by casting the structs to VkBool32 arrays and jointly iterating over requested features and given device features
  uint32_t num_features = sizeof(VkPhysicalDeviceFeatures) / sizeof(VkBool32);
  for (uint32_t i = 0; i < num_features; i++) {
    VkBool32 requested_support = ((VkBool32*) &msic_required_physical_device_features)[i];
    VkBool32 device_support = ((VkBool32*) &features)[i];
    if (requested_support && !device_support) {
      MSIC_VERBOSE("\tRequested feature support for feature #%2d, but device does not support that feature", i);
      all_requested_supported = false;
    }
  }

  if (all_requested_supported) {
    MSIC_VERBOSE("\tAll requested features supported");
  } else {
    suitable = false;
  }

  // --------------
  // Queue families 
  // --------------
  
  // TODO: rewrite queue handling:
  // Idea:
  // - Bool flag: msic_headless tells if we need present queue
  // - Bool flag: extra one for msic_require_graphics
  // - If both true, specially test if a queue supports both
  // - Then take additional flags: Test if either of present or graphics queue supports these aswell
  // - If yes, point there, if no, point elsewhere.
  //
  
 
  uint32_t queue_families_count = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_families_count, NULL);

  VkQueueFamilyProperties queue_family_props[queue_families_count];
  vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_families_count, queue_family_props);

  bool present_support[queue_families_count]; 
  bool graphics_support[queue_families_count];

  bool extra_queues_support[msic_num_extra_queue_families][queue_families_count];
  
  bool any_present_support = false;
  bool any_graphics_support = false;

  bool extra_any_support[msic_num_extra_queue_families];

  for (uint32_t i = 0; i < msic_num_extra_queue_families; i++) {
    extra_any_support[i] = false;
  }

  for (uint32_t i = 0; i < queue_families_count; i++) {
    // Check for presentation support
    if (msic_present) {
      bool supported = true;

      for (uint32_t j = 0; j < core->num_windows; j++) {
        MsicWindowContext *w_context = &core->windows[j];
        VkBool32 present_supported = false;

        MSIC_VK_CALL(vkGetPhysicalDeviceSurfaceSupportKHR(device, i, w_context->surface, &present_supported));

        any_present_support |= present_supported;
        supported &= present_supported;
      }

      present_support[i] = supported;
    }


    // Check for graphics support
    if (msic_graphics) {
      graphics_support[i] = (queue_family_props[i].queueFlags & VK_QUEUE_GRAPHICS_BIT);
      any_graphics_support |= graphics_support[i];
    }

    // Check other requested queues
    for (uint32_t j = 0; j < msic_num_extra_queue_families; j++) {
      extra_queues_support[j][i] = (queue_family_props[i].queueFlags & msic_extra_queue_families_flags[j]) == msic_extra_queue_families_flags[j];
      extra_any_support[j] = extra_queues_support[j][i];
    }
  }
  
  if (msic_present && !any_present_support) {
    MSIC_VERBOSE("No present support on device %s", props.deviceName);
    return false;
  } 

  if (msic_graphics && !any_graphics_support) {
    MSIC_VERBOSE("No graphics support on device %s", props.deviceName); 
    return false;
  }

  for (uint32_t i = 0; i < msic_num_extra_queue_families; i++) {
    if (!extra_any_support[i]) {
      MSIC_VERBOSE("No support for requested queue flags at index %d", i);
      return false;
    }
  }

  // Check if we have a queue that supports both presentation and graphics operations.
  // If so, choose that one. If multiple, use lower index

  uint32_t first_present_support = 0;
  uint32_t first_graphics_support = 0;

  if (msic_present && msic_graphics) {
    for (uint32_t i = 0; i < queue_families_count; i++) {
      if (present_support[i]) first_present_support = i;
      if (graphics_support[i]) first_graphics_support = i;

      if (present_support[i] && graphics_support[i]) {
        *present_queue_index = i;
        *graphics_queue_index = i;
        *present_graphics_match = true;
        break;
      } 
    } 
  }

  if (!present_graphics_match) {
    // Don't care to check msic_present and msic_graphics here, if those are false the values are 0 and supposed to be invalid anyways
    *present_queue_index = first_present_support;
    *graphics_queue_index = first_graphics_support; 
  }

  for (uint32_t i = 0; i < msic_num_extra_queue_families; i++) {
    int first_test_index = -1;
    if (msic_present)       first_test_index = (int) *present_queue_index;
    else if (msic_graphics) first_test_index = (int) *graphics_queue_index;

    // Choose a queue we use for presentation or graphics if possible
    if (first_test_index != -1) {
      if (extra_queues_support[i][first_test_index]) {
        (*queue_indices)[i] = (uint32_t) first_test_index;  
        continue;
      }
    }

    for (uint32_t j = 0; j < queue_families_count; j++) {
      // Bit confusing, in this double loop the extra requested queues are outside, the available queues inside loop
      // This is reverse from the loop above where we check support, so the indices i and j are the other way around here
      if (extra_queues_support[i][j]) {
        (*queue_indices)[i] = j;  
        continue;
      }
    }
  }

  // --------------
  // Device Extension support 
  // --------------
  MSIC_VERBOSE("\tChecking device extension support");
  if (!msic_check_device_extension_support(device)) {
    MSIC_VERBOSE("\tRequested device extensions not supported"); 
    suitable = false;
  }


  // --------------
  // Swapchain requirements 
  // --------------
  if (!msic_present) {
    // TODO Implement
  } 



  if (!suitable) {
    MSIC_INFO("Device %s unsuitable", props.deviceName);
    return false;
  }

  MSIC_INFO("Device %s suitable", props.deviceName);
  return true;
}

uint32_t msic_default_choose_physical_device(uint32_t device_count, VkPhysicalDevice *devices, bool *suitable) {
  UNUSED(devices);
  for (uint32_t i = 0; i < device_count; i++) {
    if (suitable[i]) return i;
  }

  MSIC_ERR("Asked to choose a physical device when none suitable were present");
  return UINT32_MAX;
}

void msic_list_phys_device_features(VkPhysicalDeviceFeatures features) {
     MSIC_VERBOSE("\trobustBufferAccess: %s",features.robustBufferAccess ? "true" : "false");
     MSIC_VERBOSE("\tfullDrawIndexUint32: %s",features.fullDrawIndexUint32 ? "true" : "false");
     MSIC_VERBOSE("\timageCubeArray: %s",features.imageCubeArray ? "true" : "false");
     MSIC_VERBOSE("\tindependentBlend: %s",features.independentBlend ? "true" : "false");
     MSIC_VERBOSE("\tgeometryShader: %s",features.geometryShader ? "true" : "false");
     MSIC_VERBOSE("\ttessellationShader: %s",features.tessellationShader ? "true" : "false");
     MSIC_VERBOSE("\tsampleRateShading: %s",features.sampleRateShading ? "true" : "false");
     MSIC_VERBOSE("\tdualSrcBlend: %s",features.dualSrcBlend ? "true" : "false");
     MSIC_VERBOSE("\tlogicOp: %s",features.logicOp ? "true" : "false");
     MSIC_VERBOSE("\tmultiDrawIndirect: %s",features.multiDrawIndirect ? "true" : "false");
     MSIC_VERBOSE("\tdrawIndirectFirstInstance: %s",features.drawIndirectFirstInstance ? "true" : "false");
     MSIC_VERBOSE("\tdepthClamp: %s",features.depthClamp ? "true" : "false");
     MSIC_VERBOSE("\tdepthBiasClamp: %s",features.depthBiasClamp ? "true" : "false");
     MSIC_VERBOSE("\tfillModeNonSolid: %s",features.fillModeNonSolid ? "true" : "false");
     MSIC_VERBOSE("\tdepthBounds: %s",features.depthBounds ? "true" : "false");
     MSIC_VERBOSE("\twideLines: %s",features.wideLines ? "true" : "false");
     MSIC_VERBOSE("\tlargePoints: %s",features.largePoints ? "true" : "false");
     MSIC_VERBOSE("\talphaToOne: %s",features.alphaToOne ? "true" : "false");
     MSIC_VERBOSE("\tmultiViewport: %s",features.multiViewport ? "true" : "false");
     MSIC_VERBOSE("\tsamplerAnisotropy: %s",features.samplerAnisotropy ? "true" : "false");
     MSIC_VERBOSE("\ttextureCompressionETC2: %s",features.textureCompressionETC2 ? "true" : "false");
     MSIC_VERBOSE("\ttextureCompressionASTC_LDR: %s",features.textureCompressionASTC_LDR ? "true" : "false");
     MSIC_VERBOSE("\ttextureCompressionBC: %s",features.textureCompressionBC ? "true" : "false");
     MSIC_VERBOSE("\tocclusionQueryPrecise: %s",features.occlusionQueryPrecise ? "true" : "false");
     MSIC_VERBOSE("\tpipelineStatisticsQuery: %s",features.pipelineStatisticsQuery ? "true" : "false");
     MSIC_VERBOSE("\tvertexPipelineStoresAndAtomics: %s",features.vertexPipelineStoresAndAtomics ? "true" : "false");
     MSIC_VERBOSE("\tfragmentStoresAndAtomics: %s",features.fragmentStoresAndAtomics ? "true" : "false");
     MSIC_VERBOSE("\tshaderTessellationAndGeometryPointSize: %s",features.shaderTessellationAndGeometryPointSize ? "true" : "false");
     MSIC_VERBOSE("\tshaderImageGatherExtended: %s",features.shaderImageGatherExtended ? "true" : "false");
     MSIC_VERBOSE("\tshaderStorageImageExtendedFormats: %s",features.shaderStorageImageExtendedFormats ? "true" : "false");
     MSIC_VERBOSE("\tshaderStorageImageMultisample: %s",features.shaderStorageImageMultisample ? "true" : "false");
     MSIC_VERBOSE("\tshaderStorageImageReadWithoutFormat: %s",features.shaderStorageImageReadWithoutFormat ? "true" : "false");
     MSIC_VERBOSE("\tshaderStorageImageWriteWithoutFormat: %s",features.shaderStorageImageWriteWithoutFormat ? "true" : "false");
     MSIC_VERBOSE("\tshaderUniformBufferArrayDynamicIndexing: %s",features.shaderUniformBufferArrayDynamicIndexing ? "true" : "false");
     MSIC_VERBOSE("\tshaderSampledImageArrayDynamicIndexing: %s",features.shaderSampledImageArrayDynamicIndexing ? "true" : "false");
     MSIC_VERBOSE("\tshaderStorageBufferArrayDynamicIndexing: %s",features.shaderStorageBufferArrayDynamicIndexing ? "true" : "false");
     MSIC_VERBOSE("\tshaderStorageImageArrayDynamicIndexing: %s",features.shaderStorageImageArrayDynamicIndexing ? "true" : "false");
     MSIC_VERBOSE("\tshaderClipDistance: %s",features.shaderClipDistance ? "true" : "false");
     MSIC_VERBOSE("\tshaderCullDistance: %s",features.shaderCullDistance ? "true" : "false");
     MSIC_VERBOSE("\tshaderFloat64: %s",features.shaderFloat64 ? "true" : "false");
     MSIC_VERBOSE("\tshaderInt64: %s",features.shaderInt64 ? "true" : "false");
     MSIC_VERBOSE("\tshaderInt16: %s",features.shaderInt16 ? "true" : "false");
     MSIC_VERBOSE("\tshaderResourceResidency: %s",features.shaderResourceResidency ? "true" : "false");
     MSIC_VERBOSE("\tshaderResourceMinLod: %s",features.shaderResourceMinLod ? "true" : "false");
     MSIC_VERBOSE("\tsparseBinding: %s",features.sparseBinding ? "true" : "false");
     MSIC_VERBOSE("\tsparseResidencyBuffer: %s",features.sparseResidencyBuffer ? "true" : "false");
     MSIC_VERBOSE("\tsparseResidencyImage2D: %s",features.sparseResidencyImage2D ? "true" : "false");
     MSIC_VERBOSE("\tsparseResidencyImage3D: %s",features.sparseResidencyImage3D ? "true" : "false");
     MSIC_VERBOSE("\tsparseResidency2Samples: %s",features.sparseResidency2Samples ? "true" : "false");
     MSIC_VERBOSE("\tsparseResidency4Samples: %s",features.sparseResidency4Samples ? "true" : "false");
     MSIC_VERBOSE("\tsparseResidency8Samples: %s",features.sparseResidency8Samples ? "true" : "false");
     MSIC_VERBOSE("\tsparseResidency16Samples: %s",features.sparseResidency16Samples ? "true" : "false");
     MSIC_VERBOSE("\tsparseResidencyAliased: %s",features.sparseResidencyAliased ? "true" : "false");
     MSIC_VERBOSE("\tvariableMultisampleRate: %s",features.variableMultisampleRate ? "true" : "false");
     MSIC_VERBOSE("\tinheritedQueries: %s",features.inheritedQueries ? "true" : "false");
}

uint32_t msic_create_device(MsicVulkanCore *core) {
  VkDeviceCreateInfo create_info = {0};
  create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
  create_info.pNext = NULL;
  create_info.flags = 0;
  
  // The problem here is that the user can request different queues with different flags
  // However, if multiple of these point to the same queue, I may not request multiple of these
  // 
  // The following code reduces the queue indices down to distinct ones, creates only distinct queues
  // and then distributes the obtained VkQueue handles into core->queues so that if two requested 
  // queues get the same queue index, the corresponding handle will be written to *both* positions
  // in core->queues
  // 
  // A better option might be to increase the queueCount for duplicated queues

  uint32_t num_queues = core->num_device_queue_indices;
  
  uint32_t prefix_queues = 0;
  if (msic_present) {
    prefix_queues++;
  }
  if (msic_graphics) {
    prefix_queues++;
  }

  uint32_t queue_indices[prefix_queues + num_queues];
  if (msic_present) {
    queue_indices[0] = core->present_queue_index;
  }

  if (msic_graphics) {
    queue_indices[msic_present ? 1 : 0] = core->graphics_queue_index;
  }

  memcpy(queue_indices + prefix_queues, core->device_queue_indices, core->num_device_queue_indices * sizeof(uint32_t));
  num_queues += prefix_queues;

  uint32_t distinct_queues_first_indices[num_queues];
  uint32_t queue_indirection_index      [num_queues];
  uint32_t distinct_queues_count = 0;

  for (uint32_t i = 0; i < num_queues; i++) {
    // Check if index is already known  
    
    bool is_duplicate = false;
    for (uint32_t j = 0; j < distinct_queues_count; j++) {
      if (queue_indices[distinct_queues_first_indices[j]] == queue_indices[i]) {
        queue_indirection_index[i] = j;
        is_duplicate = true;
      }
    } 

    if (!is_duplicate) {
      distinct_queues_first_indices[distinct_queues_count] = i;

      queue_indirection_index[i] = distinct_queues_count;
      distinct_queues_count++;
    }
  }
  
  uint32_t distinct_queue_indices[distinct_queues_count];
  for (uint32_t i = 0; i < distinct_queues_count; i++) {
    distinct_queue_indices[i] = distinct_queues_first_indices[i];
  }

  MSIC_VERBOSE("Reducing queues from %d to %d distinct queues", num_queues, distinct_queues_count);

  VkDeviceQueueCreateInfo queue_create_infos[distinct_queues_count];

  for (uint32_t i = 0; i < distinct_queues_count; i++) {
    queue_create_infos[i].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queue_create_infos[i].pNext             = NULL;
    queue_create_infos[i].flags             = 0;
    queue_create_infos[i].queueFamilyIndex  = queue_indices[distinct_queue_indices[i]];
    queue_create_infos[i].queueCount        = 1;

    float queue_prio = 1.0f;
    queue_create_infos[i].pQueuePriorities  = &queue_prio;
  }
  
  create_info.queueCreateInfoCount = distinct_queues_count;
  create_info.pQueueCreateInfos = queue_create_infos;

  create_info.enabledLayerCount = 0;
  create_info.ppEnabledLayerNames = NULL;

  create_info.enabledExtensionCount = msic_device_extension_count;
  
  if (!msic_headless) create_info.enabledExtensionCount++;

  const char *extension_names[create_info.enabledExtensionCount];

  if (create_info.enabledExtensionCount > 0) {
    if (msic_write_device_extensions(create_info.enabledExtensionCount, extension_names)) {
      MSIC_ERR("Failed to write vulkan device extension names");
      return 1;
    }
    
    create_info.ppEnabledExtensionNames = (const char * const *) extension_names;

    for (uint32_t i = 0; i < create_info.enabledExtensionCount; i++) {
      MSIC_VERBOSE("\tEnabling device extension: %s", extension_names[i]);
    }

  } else {
    create_info.ppEnabledExtensionNames = NULL;
  }

  create_info.pEnabledFeatures = &msic_required_physical_device_features;

  MSIC_VK_CALL(vkCreateDevice(core->physical_device, &create_info, NULL, &core->device));

  VkQueue distinct_queues[distinct_queues_count];

  for (uint32_t i = 0; i < distinct_queues_count; i++) {
    vkGetDeviceQueue(core->device, queue_indices[distinct_queue_indices[i]], 0, &(distinct_queues[i]));
  }
 
  if (msic_num_extra_queue_families > 0) {
    core->queues = malloc(sizeof(VkQueue) * msic_num_extra_queue_families);

    for (uint32_t i = prefix_queues; i < num_queues; i++) {
      core->queues[i] = distinct_queues[queue_indirection_index[i]];
    }
  } 

  if (msic_present)  core->present_queue   = distinct_queues[queue_indirection_index[0]];
  if (msic_graphics) core->graphics_queue = distinct_queues[queue_indirection_index[msic_present ? 1 : 0]];

  return 0;
}
