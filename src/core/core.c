#include "control_params.h"
#include "mosaic.h"

#include "core.h"
#include "window/window.h"
#include "device/device.h"
#include "swapchain/swapchain.h"

#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <vulkan/vulkan.h>
#include <vulkan/vk_enum_string_helper.h>
#include <GLFW/glfw3.h>

void msic_default_log_callback(const char *file, uint32_t line, const char *message, uint32_t severity) {
  if (severity >= msic_log_level) {
    printf("mosaic | %30s:%-5d | %7s | %s\n", file, line, msic_severity_strings[severity], message);
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

