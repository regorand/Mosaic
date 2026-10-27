#include "pipeline.h"

#include <string.h>
#include <errno.h>
#include <stdlib.h>

uint32_t msic_load_shader_SPV(const char *file_name, uint32_t **source_out, size_t *byte_size_out) {
  if (file_name == NULL) {
    MSIC_ERR("File name for shader spv is NULL");
    return 1;
  }

  FILE *f= fopen(file_name, "rb");

  if (f == NULL) {
    MSIC_ERR("Error opening shader spv file %s with code %d: %s", file_name, errno, strerror(errno));
    return 1;
  }

  if (fseek(f, 0, SEEK_END)) {
    MSIC_ERR("Error getting size of file %s", file_name);
    fclose(f);
    return 1;
  }

  long file_size = ftell(f);
  if (file_size < 0) {
    MSIC_ERR("Error getting size of file %s", file_name);
    fclose(f);
    return 1;
  }
  
  if (file_size % 4 != 0) {
    MSIC_ERR("Byte size of Spir-V files must be multiple of 4, check fails for %s", file_name);
    fclose(f);
    return 1;
  }

  if (fseek(f, 0, SEEK_SET)) {
    MSIC_ERR("Error getting size of file %s", file_name);
    fclose(f);
    return 1;
  }

  *byte_size_out = (size_t) file_size; 
  *source_out = malloc(*byte_size_out);

  if (*source_out == NULL) {
    MSIC_ERR("Failed to allocate memory for shader %s", file_name); 
    fclose(f);
    return 1;
  }


  const size_t read_ret = fread(*source_out, sizeof(uint32_t), *byte_size_out / 4, f);
  
  if (read_ret != *byte_size_out / 4) {
    if (feof(f)) {
      MSIC_ERR("Unexpected end of file reading shader file %s", file_name);
    } else if (ferror(f)) {
      MSIC_ERR("Error reading shader source file %s", file_name);
    }

    free(*source_out);
    fclose(f);
    return 1;
  }

  fclose(f);
  return 0;
}

uint32_t msic_build_shader_module_from_file(const MsicVulkanCore *core, const char *file_name, VkShaderModule *shader) {
  uint32_t *shader_source;
  size_t    byte_size;

  MSIC_VERBOSE("Loading shader SPV %s", file_name);
  MSIC_CALL(msic_load_shader_SPV(file_name, &shader_source, &byte_size));

  uint32_t res = msic_build_shader_module(core, shader_source, byte_size, shader);  

  free(shader_source);

  if (res != 0) {
    MSIC_ERR("Failed to build shader module for shader %s", file_name);
    return 1;
  }

  return 0;
}

uint32_t msic_build_shader_module(const MsicVulkanCore *core, uint32_t *source, size_t code_size, VkShaderModule *shader) {
  if (code_size % 4 != 0) {
    MSIC_ERR("Code size for Spir-V shader creation must be a multiple of 4!");
    return 1;
  }

  VkShaderModuleCreateInfo create_info;

  create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  create_info.pNext = NULL;
  create_info.flags = 0;
  create_info.codeSize = code_size;
  create_info.pCode = source;  

  MSIC_VK_CALL(vkCreateShaderModule(core->device, &create_info, NULL, shader));

  return 0;
}

