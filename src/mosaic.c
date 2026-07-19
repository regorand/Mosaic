#include "mosaic.h"
#include "core.h"
#include <vulkan/vulkan.h>
#include <stdint.h>
#include <stdarg.h>
#include <stdlib.h>

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

uint32_t msic_build_vulkan_core(MsicVulkanCore *core) {
  if (!msic_initialized) {
    MSIC_ERR("Mosaic not initialized");
    return 1;
  }

  core->instance =                  VK_NULL_HANDLE;
  core->debug_messenger =           VK_NULL_HANDLE;
  core->physical_device =           VK_NULL_HANDLE;
  core->num_device_queue_indices =  0;
  core->device_queue_indices =      NULL;

  MSIC_INFO("Loading vulkan");

  MSIC_INFO("Checking requested layer support");
  if (!msic_check_layer_support()) {
    MSIC_ERR("Layer support validation failed");
  }

  uint32_t res = msic_create_instance(core);
  if (res) {
    MSIC_ERR("Aborting");
    return res;
  }

  if (msic_enable_validation) {
    MSIC_INFO("Enabling vulkan debug messenger");
    res = msic_setup_debug_messenger(core);
    if (res) {
      MSIC_ERR("Aborting");
      return res;
    }
  }
  
  MSIC_INFO("Creating physical device");
  res = msic_create_physical_device(core);
  if (res) {
    MSIC_ERR("Aborting");
    return res;
  }

  MSIC_INFO("Finished loading vulkan core");
  return 0;
}


uint32_t msic_destroy_vulkan_core(MsicVulkanCore *core) {
  if (core->device_queue_indices != NULL) {
    free(core->device_queue_indices);
  }


  if (msic_enable_validation && core->debug_messenger != VK_NULL_HANDLE) {
    MSIC_INFO("Destroying debug messenger");

    PFN_vkDestroyDebugUtilsMessengerEXT func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(core->instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func == NULL) {
      MSIC_ERR("Failed to find proc address for vkDestroyDebugUtilsMessengerEXT");
      return 1;
    }

    func(core->instance, core->debug_messenger, NULL);
  }

  if (core->instance != VK_NULL_HANDLE) {
    MSIC_INFO("Destroying vulkan instance");
    vkDestroyInstance(core->instance, NULL);
  }

  return 0;
}
