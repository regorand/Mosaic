#include "mosaic.h"

int main(void) {
  MsicVulkanCore core;
  msic_dummy_init();
  msic_log_level = MSIC_SEVERITY_VERBOSE;

  return (int) msic_build_vulkan_core(&core);
}
