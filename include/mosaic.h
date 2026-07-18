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

#define MSIC_SEVERITY_ERR 3
#define MSIC_SEVERITY_WARN 2
#define MSIC_SEVERITY_INFO 1
#define MSIC_SEVERITY_VERBOSE 0

/**
 * Array of string values of mosaic severity values. Can be indexed using the MSIC_SEVERITY_<severity> directly
 */ 
extern const char *msic_severity_strings[]; 

/**
 * Function pointer for callback for mosaic logs. 
 * Can be set by user to receive mosaic log output.
 *
 * @param: Filename provided by __FILE__ macro
 * @param: Line number provided by __LINE__
 * @param: Log message
 * @param: Message severity, can be compared to the MSIC_SEVERITY_<severity> macros with severities: VERBOSE, INFO, WARN and ERR. 
 *          MSIC_SEVERITY_ERR is the highest value.
 */
extern void (*msic_log_callback)(const char*, uint32_t, const char*, uint32_t);

/**
 * Specifies which log level will be written by the default log callback 
 * Any MSIC_SEVERITY_<severity> macro is a valid value
 */
extern uint32_t msic_log_level;

/**
 * @brief: osaic calls this function to generate log messages. 
 * Will resolve the variadic arguments and then call the log callback if it is set.
 * 
 * If a user wishes to call this function it should be done using the corresponding macro MSIC_<severity>
 *
 * @param[file]   : Filename provided by __FILE__ macro
 * @param[line]   : Line number provided by __LINE__
 * @param[level]  : Message severity, can be compared to the MSIC_SEVERITY_<severity> macros with severities: VERBOSE, INFO, WARN and ERR. 
 *          MSIC_SEVERITY_ERR is the highest value.
 * @param[message]: Log message template with printf style format specifiers
 * @param[...]    : Variadic arguments passed to format the template.
 */
void msic_output_log(const char *file, uint32_t line, uint32_t level, const char* message, ...);

#define MSIC_VERBOSE(s, ...) msic_output_log(__FILE__, __LINE__, MSIC_SEVERITY_VERBOSE, s, ##__VA_ARGS__);
#define MSIC_INFO(s, ...) msic_output_log(__FILE__, __LINE__, MSIC_SEVERITY_INFO, s, ##__VA_ARGS__);
#define MSIC_WARN(s, ...) msic_output_log(__FILE__, __LINE__, MSIC_SEVERITY_WARN, s, ##__VA_ARGS__);
#define MSIC_ERR(s, ...) msic_output_log(__FILE__, __LINE__, MSIC_SEVERITY_ERR, s, ##__VA_ARGS__);

/**
 * Wrapper macro for vulkan API calls which logs errors generated and returns the error code
 *
 * @param [f]: Vulkan API call
 */
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

/**
 * Control parameter specifying if mosaic is initialized. Mosaic will not create any ressources if this is set to false
 */
extern bool msic_initialized;

/**
 * Control parameter specifying whether vulkan validation layers should be enabled
 */
extern bool msic_enable_validation;

/** 
 * Control parameter specifying number of additional layers in `msic_layers` 
 */
extern uint32_t msic_layer_count;

/** 
 * Control parameter specifying additional vulkan layers to enable.
 * Number of layers must be specified in `msic_layer_count`
 */
extern char **msic_layers;

/**
 * Control parameter specifying the application name, see https://docs.vulkan.org/refpages/latest/refpages/source/VkInstanceCreateInfo.html
 */
extern const char* msic_application_name;

/**
 * Control parameter specifying the application version, see https://docs.vulkan.org/refpages/latest/refpages/source/VkInstanceCreateInfo.html

 */
extern uint32_t msic_application_version;

/**
 * Control parameter specifying the engine name, see https://docs.vulkan.org/refpages/latest/refpages/source/VkInstanceCreateInfo.html

 */
extern const char* msic_engine_name;

/**
 * Control parameter specifying the engine version, see https://docs.vulkan.org/refpages/latest/refpages/source/VkInstanceCreateInfo.html

 */
extern uint32_t msic_engine_version;

/**
 * Control parameters specifing the vulkan api version, see https://docs.vulkan.org/refpages/latest/refpages/source/VkInstanceCreateInfo.html

 */
extern uint32_t msic_vk_api_version;

// -------------------
// State and functions
// -------------------

/**
 * Structure holding the core vulkan instances
 */
typedef struct MsicVulkanCore {
  /**
   * Vulkan instance handle, see https://docs.vulkan.org/refpages/latest/refpages/source/VkInstance.html
   */
  VkInstance instance;
} MsicVulkanCore;

/**
 * Dummy initializer function used to assist early development 
 * TODO will be removed later
 */
void msic_dummy_init(void);

/**
 * Builds the vulkan core.
 * Makes decisions based on how the control parameters are set.
 *
 * @param[core]: Pointer to a `MsicVulkanCore` which will be filled with the vulkan handles
 */
uint32_t msic_build_vulkan_core(MsicVulkanCore *core);
    
#ifdef __cplusplus
}
#endif

#endif // MOSAIC_H
