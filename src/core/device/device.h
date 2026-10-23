#ifndef MSIC_DEVICE_H
#define MSIC_DEVICE_H

#include <stdint.h>
#include <stdbool.h>

#include "mosaic.h"

uint32_t msic_create_physical_device(MsicVulkanCore *core);

uint32_t msic_create_device(MsicVulkanCore *core);

bool msic_default_phys_device_suitable(VkPhysicalDevice device, 
                                        uint32_t *present_queue_index, 
                                        uint32_t *graphics_queue_index,
                                        bool     *present_graphics_match,
                                        uint32_t **queue_indices, 
                                        MsicVulkanCore *core); 

uint32_t msic_default_choose_physical_device(uint32_t device_count, VkPhysicalDevice *devices, bool *suitable);


#endif // MSIC_DEVICE_H
