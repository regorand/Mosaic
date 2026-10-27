#ifndef MSIC_PIPELINE_H
#define MSIC_PIPELINE_H

#include "mosaic.h"

uint32_t msic_load_shader_SPV(const char *file_name, uint32_t **source_out, size_t *byte_size_out);

uint32_t msic_build_shader_module_from_file(const MsicVulkanCore *core, const char *file_name, VkShaderModule *shader);

uint32_t msic_build_shader_module(const MsicVulkanCore *core, uint32_t *source, size_t code_size, VkShaderModule *shader);

#endif // MSIC_PIPELINE_H
