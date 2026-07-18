#include "mosaic.h"

int main(void) {
  MsicVulkanCore core;
  msic_dummy_init();

  return (int) msic_build_vulkan_core(&core);
}
