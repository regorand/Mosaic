#include "mosaic.h"
#include "core.h"
#include <vulkan/vulkan.h>
#include <stdint.h>
#include <stdarg.h>
#include <stdlib.h>

void msic_output_log(const char *file, uint32_t line, uint32_t level, const char* message, ...) {
  if (msic_log_callback == NULL) return;
  
  char buffer[512];
  va_list args;
  va_start (args, message);
  vsnprintf (buffer, 512, message, args);

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

  core->instance                  = VK_NULL_HANDLE;
  core->debug_messenger           = VK_NULL_HANDLE;
  core->physical_device           = VK_NULL_HANDLE;
  core->windows                   = NULL;
  core->surfaces                  = NULL;
  core->num_windows               = 0;
  core->num_device_queue_indices  = 0;
  core->device_queue_indices      = NULL;
  core->device                    = VK_NULL_HANDLE; 
  core->queues                    = NULL;
  core->present_queue_index       = 0;

  MSIC_INFO("Loading vulkan");

  MSIC_INFO("Checking requested layer support");
  if (!msic_check_layer_support()) {
    MSIC_ERR("Layer support validation failed");
    return 1;
  }

  MSIC_INFO("Checking requested extension support");
  if (!msic_check_extension_support()) {
    MSIC_ERR("Extension support validation failed");
    return 1;
  }

  MSIC_CALL(msic_create_instance(core));

  if (msic_enable_validation) {
    MSIC_INFO("Enabling vulkan debug messenger");
    MSIC_CALL(msic_setup_debug_messenger(core));
  }
  
  if (!msic_headless) {
    MSIC_INFO("Running mosaic with gui");
   
    MSIC_INFO("Initializing glfw");
    MSIC_CALL(msic_init_glfw());

    MSIC_INFO("Creating windows");
    MSIC_CALL(msic_create_windows(core));

    MSIC_INFO("Creating surfaces");
    MSIC_CALL(msic_create_surfaces(core));
  } else {
    MSIC_INFO("Running mosaic headless");
  }

  MSIC_INFO("Creating physical device");
  MSIC_CALL(msic_create_physical_device(core));

  MSIC_INFO("Creating device");
  MSIC_CALL(msic_create_device(core));

  MSIC_INFO("Finished loading vulkan core");
  return 0;
}


uint32_t msic_destroy_vulkan_core(MsicVulkanCore *core) {
  if (core->queues != NULL) {
    MSIC_VERBOSE("Freeing queue handle array");
    free(core->queues);
  }

  if (core->surfaces != NULL) {
    MSIC_VERBOSE("Destroying surfaces and freeing handle array");
    for (uint32_t i = 0; i < core->num_windows; i++) {
      vkDestroySurfaceKHR(core->instance, core->surfaces[i], NULL);
    }
    free(core->surfaces);
  }

  if (!msic_headless) {
    MSIC_VERBOSE("Terminating glfw");
    glfwTerminate();
  }

  if (core->windows != NULL) {
    MSIC_VERBOSE("Freeing window pointer array");
    free(core->windows);
  }

  if (core->device != VK_NULL_HANDLE) {
    MSIC_INFO("Destroying device");
    vkDestroyDevice(core->device, NULL);
  }

  if (core->device_queue_indices != NULL) {
    MSIC_VERBOSE("Freeing device queue indices array");
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
