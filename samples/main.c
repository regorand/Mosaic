#include "mosaic.h"
#include <vulkan/vulkan.h>


void which_should_close(MsicVulkanCore *core, bool *still_open, bool *should_close) {
  for (uint32_t i = 0; i < core->num_windows; i++) {
    should_close[i] = false;
    if (still_open[i] && glfwWindowShouldClose(core->windows[i])) {
      should_close[i] = true;
    }
  }
}

bool all_should_close(MsicVulkanCore *core, bool *still_open) {
  for (uint32_t i = 0; i < core->num_windows; i++) {
    if (still_open[i] && !glfwWindowShouldClose(core->windows[i])) {
      return false;
    }
  }

  return true;
}

int main(void) {
  MsicVulkanCore core;
  msic_dummy_init();
  msic_log_level = MSIC_SEVERITY_VERBOSE;

  msic_physical_device_type = VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU;

  msic_extension_count = 1;
  // char *xcb_extension = VK_KHR_XCB_SURFACE_EXTENSION_NAME;
  char *xcb_extension = "VK_KHR_xcb_surface";
  msic_extensions = &xcb_extension;

  msic_num_queue_families = 1;
  // msic_headless = true;
  
  msic_num_windows = 1;

  uint32_t flags_arr[msic_num_queue_families];
  flags_arr[0] = VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT;
  msic_queue_families_flags = flags_arr;
 
  // Not supported on my current development machine, lets me test physical device unsuitability
  // msic_required_physical_device_features.textureCompressionASTC_LDR = VK_TRUE;

  uint32_t res = msic_build_vulkan_core(&core);
  if (res) return (int) res;

  MSIC_INFO("Initialized Vulkan context without error!");

  bool still_open[core.num_windows];
  for (uint32_t i = 0; i < core.num_windows; i++) {
    still_open[i] = true;
  }

  while (!all_should_close(&core, still_open)) {
    glfwPollEvents();

    bool should_close[core.num_windows];
    which_should_close(&core, still_open, should_close);

    for (uint32_t i = 0; i < core.num_windows; i++) {
      if (should_close[i]) {
        glfwDestroyWindow(core.windows[i]);
        still_open[i] = false;
      }
    }

  }  

  MSIC_INFO("Closing Windows");

  res = msic_destroy_vulkan_core(&core);
  return (int) res;
}
