#include "mosaic.h"

#include "core/core.h"
#include "core/device/device.h"
#include "core/window/window.h"
#include "core/swapchain/swapchain.h"

#include "pipeline/pipeline.h"

#include <vulkan/vulkan.h>
#include <stdint.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

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
  
  if (msic_present) {
    MSIC_INFO("Running mosaic with present support");
   
    MSIC_INFO("Initializing glfw");
    MSIC_CALL(msic_init_glfw());

    MSIC_INFO("Creating windows");
    MSIC_CALL(msic_create_windows(core));

    MSIC_INFO("Creating surfaces");
    MSIC_CALL(msic_create_surfaces(core));
  } else {
    MSIC_INFO("Running mosaic without presenting");
  }

  MSIC_INFO("Creating physical device");
  MSIC_CALL(msic_create_physical_device(core));

  MSIC_INFO("Creating device");
  MSIC_CALL(msic_create_device(core));

  if (msic_present) {
    MSIC_INFO("Initializing Swapchain");

    if (!msic_check_swapchain_support(core)) {
      MSIC_ERR("Requested Swapchain features not supported");
      return 1;
    }

    MSIC_CALL(msic_create_swapchain(core));

    MSIC_VERBOSE("Creating Swapchain Views");
    MSIC_CALL(msic_create_swapchain_imageviews(core));
  }

  MSIC_INFO("Finished loading vulkan core");
  return 0;
}


uint32_t msic_destroy_vulkan_core(MsicVulkanCore *core) {
  if (core->queues != NULL) {
    MSIC_VERBOSE("Freeing queue handle array");
    free(core->queues);
  }

  if (msic_present) {
    MSIC_VERBOSE("Terminating glfw");
    glfwTerminate();
  }

  if (core->windows != NULL) {
    MSIC_VERBOSE("Destroying window contexts");
    
    for (uint32_t i = 0; i < core->num_windows; i++) {
      if (core->windows[i].open) {
        msic_destroy_window(core, i);
      }
    }

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


void msic_destroy_window(MsicVulkanCore *core, uint32_t window_index) {
  MSIC_VERBOSE("Destroying window with window_index %d", window_index);
  MsicWindowContext *w_context = &core->windows[window_index];

  w_context->open = false;

  if (w_context->swap_image_views != NULL) {
    for (uint32_t i = 0; i < w_context->num_swap_images; i++) {
      vkDestroyImageView(core->device, w_context->swap_image_views[i], NULL);
    }

    free(w_context->swap_image_views);
  }

  if (w_context->swapchain != NULL) {
    destroy_swapchain(core, w_context->swapchain);
  }

  if (w_context->swap_images != NULL) {
    free(w_context->swap_images);
  }

  if (w_context->formats != NULL) {
    free(w_context->formats);
  }
  
  if (w_context->present_modes != NULL) {
    free(w_context->present_modes);
  }

  if (w_context->surface != NULL) {
    vkDestroySurfaceKHR(core->instance, w_context->surface, NULL);
  }
  
  if (w_context->window != NULL) {
    glfwDestroyWindow(w_context->window);
  }

  w_context->open = false;
}

void destroy_swapchain(MsicVulkanCore *core, VkSwapchainKHR swapchain) {
  vkDestroySwapchainKHR(core->device, swapchain, NULL);  
}


uint32_t msic_compile_shader_runtime(const char *path, const char* options, const char *out_path) {
#define TMP_LOG_FILE "tmp_log.txt"
  if (path == NULL) {
    MSIC_ERR("Shader compilation: provided source code path is NULL");
    return 1;
  }
  
  if (out_path == NULL) {
    MSIC_ERR("Shader compilation: provided output path is NULL");
    return 1;
  }

  if (!system(NULL)) {
    MSIC_ERR("No env found to run system() with glslc command");
    return 1;
  }

  const char *used_opts = options != NULL ? options :  "";

  size_t path_length    = strlen(path); 
  size_t options_length = strlen(used_opts); 
  size_t out_length     = strlen(out_path); 

  size_t buffer_size = 100 + path_length + options_length + out_length;
  char buffer[buffer_size];
  const char *base_string = "glslc %s %s -o %s";
  int res = snprintf(buffer, buffer_size, base_string, used_opts, path, out_path);
  if (res < 0) {
    MSIC_ERR("Error preparing command string for shader compilation of %s", path);
    return 1;
  }

  MSIC_INFO("Calling glslc through system call");
  // TODO better error handling, redirect child process stdout into buffer for me ? 
  int sys_res = system(buffer);

  if (sys_res == -1) {
    MSIC_ERR("system call returned error with with errno %d: %s", errno, strerror(errno));
    return 1;
  }

  if (sys_res) {
    MSIC_ERR("System call returned non-zero return code");
    return 1;
  }

  return 0;
}

uint32_t msic_build_pipeline(MsicVulkanCore *core, MsicPipelineInfo *info) {
  if (info->num_shaders == 0) {
    MSIC_INFO("No Shaders specified, doing nothing"); 
  }

  VkShaderModule modules[info->num_shaders];
  bool           valid[info->num_shaders];
  bool           all_valid = true;

  uint32_t return_res = 0;
  
  for (uint32_t i = 0; i < info->num_shaders; i++) {
    uint32_t res = msic_build_shader_module_from_file(core, info->shader_paths[i], modules + i);
    if (res != 0) {
      MSIC_ERR("Failed to load shader %s", info->shader_paths[i]);
      valid[i] = false;
      all_valid = false;
    } else {
      MSIC_VERBOSE("Successfully loaded shader %s", info->shader_paths[i]);
      valid[i] = true;
    }
  }

  if (!all_valid) {
    MSIC_ERR("Could not load all shader spir-V files");
    return_res = 1;
    goto destroy_shaders;
  }


  // Do Stuff


destroy_shaders:
  for (uint32_t i = 0; i < info->num_shaders; i++) {
    if (valid[i]) {
      vkDestroyShaderModule(core->device, modules[i], NULL);
    }
  }
  
  return return_res;
}
