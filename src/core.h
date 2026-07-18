#ifndef MSIC_CORE_H
#define MSIC_CORE_H

#include <stdint.h>
#include <stdbool.h>
#include "mosaic.h"

bool msic_check_layer(VkLayerProperties *layer_props, uint32_t layer_count, const char * layer_name); 

bool msic_check_layer_support(void); 

uint32_t msic_write_layers(uint32_t layer_count, const char** pp_layer_names); 

uint32_t msic_create_instance(MsicVulkanCore *core); 

void msic_default_log_callback(const char *file, uint32_t line, const char *message, uint32_t severity);

#endif // MSIC_CORE_H
