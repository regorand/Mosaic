#include "mosaic.h"
#include "core.h"
#include <vulkan/vulkan.h>
#include <stdint.h>
#include <string.h>
#include <stdarg.h>

// FILE **msic_log_output_stream = &stdout; 
// FILE **msic_err_output_stream = &stderr;
// 
// bool msic_log_disable_newline = false;
// 
// bool msic_initialized = false;
// bool msic_enable_validation = true;
// 
// char **msic_layers = NULL;
// uint32_t msic_layer_count = 0;

void msic_output_log(const char *file, uint32_t line, uint32_t level, const char* message, ...) {
  if (msic_log_callback == NULL) return;
  
  char buffer[256];
  va_list args;
  va_start (args, message);
  vsnprintf (buffer, 256, message, args);

  (*msic_log_callback)(file, line, buffer, level);
}

void msic_dummy_init() {
  msic_initialized = true;
  msic_log_callback = &msic_default_log_callback;
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

  // TODO this is not gonna work!
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

  createInfo.enabledExtensionCount = 0;
  createInfo.ppEnabledExtensionNames = NULL;
  

  createInfo.enabledLayerCount = msic_layer_count + msic_enable_validation;
  const char *layer_names[createInfo.enabledLayerCount]; 
  if (msic_write_layers(createInfo.enabledLayerCount, layer_names)) {
    MSIC_ERR("Failed to write vulkan layer names");
    return 1;
  }

  for (uint32_t i = 0; i < createInfo.enabledLayerCount; i++) {
    MSIC_INFO("\tEnabling layer: %s", layer_names[i]);
  }

  createInfo.ppEnabledLayerNames = (const char * const *) layer_names;
  
  MSIC_VK_CALL(vkCreateInstance(&createInfo, NULL, &core->instance));

  return 0;
}

uint32_t msic_build_vulkan_core(MsicVulkanCore *core) {
  if (!msic_initialized) {
    MSIC_ERR("Mosaic not initialized");
    return 1;
  }

  MSIC_INFO("Loading vulkan");

  MSIC_INFO("Checking requested layer support");
  if (!msic_check_layer_support()) {
    MSIC_ERR("Layer support validation failed");
  }

  msic_create_instance(core);

  MSIC_INFO("Finished loading vulkan core");
  return 0;
}

