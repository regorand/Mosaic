#ifndef MSIC_CORE_H
#define MSIC_CORE_H

#include <stdint.h>
#include <stdbool.h>
#include "mosaic.h"

#define UNUSED(x) (void)(x)

#define clamp(val, min, max) ((val) < (min) ? (min) : ((val) > (max) ? (max) : (val)))

#define MSIC_VK_VALIDATION_LAYER_NAME "VK_LAYER_KHRONOS_validation"
#define MSIC_VK_VALIDATION_LAYER_NAME "VK_LAYER_KHRONOS_validation"

#define VK_SEVERITY_TO_MSIC_SEVERITY(s) \
   (s == VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT ? MSIC_SEVERITY_ERR       :       \
   (s == VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT ? MSIC_SEVERITY_WARN    :       \
   (s == VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT    ? MSIC_SEVERITY_INFO    :       \
   (s == VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT ? MSIC_SEVERITY_VERBOSE : 0)))) \

void msic_default_log_callback(const char *file, uint32_t line, const char *message, uint32_t severity);

VKAPI_ATTR VkBool32 VKAPI_CALL msic_vk_message_callback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData);


bool msic_check_layer(VkLayerProperties *layer_props, uint32_t layer_count, const char * layer_name); 
bool msic_check_layer_support(void); 

bool msic_check_extension(VkExtensionProperties *extension_props, uint32_t extension_count, const char * extension_name); 
bool msic_check_extension_support(void); 

bool msic_check_device_extension_support(VkPhysicalDevice device);

uint32_t msic_write_extensions(uint32_t extension_count, const char** pp_extension_names);
uint32_t msic_write_device_extensions(uint32_t extension_count, const char** pp_extension_names);

uint32_t msic_write_layers(uint32_t layer_count, const char** pp_layer_names); 

uint32_t msic_create_instance(MsicVulkanCore *core); 

uint32_t msic_setup_debug_messenger(MsicVulkanCore *core);
void msic_populate_debug_messenger_create_info(VkDebugUtilsMessengerCreateInfoEXT *create_info);


#endif // MSIC_CORE_H
