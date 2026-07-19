#include "mosaic.h"

int main(void) {
  MsicVulkanCore core;
  msic_dummy_init();
  msic_log_level = MSIC_SEVERITY_VERBOSE;

  msic_physical_device_type = VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU;

  msic_num_queue_families = 1;

  uint32_t flags_arr[msic_num_queue_families];
  flags_arr[0] = VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT;
  msic_queue_families_flags = flags_arr;
 
  // Not supported on my current development machine, lets me test physical device unsuitability
  // msic_required_physical_device_features.textureCompressionASTC_LDR = VK_TRUE;

  uint32_t res = msic_build_vulkan_core(&core);
  if (res) return (int) res;

  res = msic_destroy_vulkan_core(&core);
  return (int) res;
}
