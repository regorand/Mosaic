#ifndef MOSAIC_H
#define MOSAIC_H

#include <stdio.h>
#include <stdbool.h>
#include <vulkan/vulkan.h>
#include <vulkan/vk_enum_string_helper.h>
#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MSIC_VK_VALIDATION_LAYER_NAME "VK_LAYER_KHRONOS_validation"

#define MSIC_SEVERITY_ERR 3
#define MSIC_SEVERITY_WARN 2
#define MSIC_SEVERITY_INFO 1
#define MSIC_SEVERITY_VERBOSE 0

extern const char *msic_severity_strings[]; 
extern void (*msic_log_callback)(const char*, uint32_t, const char*, uint32_t);

extern uint32_t msic_log_level;

void msic_output_log(const char *file, uint32_t line, uint32_t level, const char* message, ...);

#define MSIC_VERBOSE(s, ...) msic_output_log(__FILE__, __LINE__, MSIC_SEVERITY_VERBOSE, s, ##__VA_ARGS__);
#define MSIC_INFO(s, ...) msic_output_log(__FILE__, __LINE__, MSIC_SEVERITY_INFO, s, ##__VA_ARGS__);
#define MSIC_WARN(s, ...) msic_output_log(__FILE__, __LINE__, MSIC_SEVERITY_WARN, s, ##__VA_ARGS__);
#define MSIC_ERR(s, ...) msic_output_log(__FILE__, __LINE__, MSIC_SEVERITY_ERR, s, ##__VA_ARGS__);

#define MSIC_VK_CALL(f) { \
  VkResult res = (f); \
  if (res) { \
    MSIC_ERR("Got VkResult %d: %s\n", res, string_VkResult(res)); \
    return res; \
  } \
}

// -------------------
// control parameters
// -------------------

extern bool msic_initialized;
extern bool msic_enable_validation;

extern char **msic_layers;
extern uint32_t msic_layer_count;

extern const char* msic_application_name;
extern uint32_t msic_application_version;

extern const char* msic_engine_name;
extern uint32_t msic_engine_version;

extern uint32_t msic_vk_api_version;

// -------------------
// State and functions
// -------------------

typedef struct MsicVulkanCore {
  VkInstance instance;
} MsicVulkanCore;

void msic_dummy_init(void);

uint32_t msic_build_vulkan_core(MsicVulkanCore *core);
    
#ifdef __cplusplus
}
#endif

#endif // MOSAIC_H
