#ifndef MSIC_CORE_H
#define MSIC_CORE_H

#include <stdint.h>
#include <stdbool.h>
#include "mosaic.h"

#define UNUSED(x) (void)(x)

#define MSIC_VK_VALIDATION_LAYER_NAME "VK_LAYER_KHRONOS_validation"
#define MSIC_VK_VALIDATION_LAYER_NAME "VK_LAYER_KHRONOS_validation"

#define VK_SEVERITY_TO_MSIC_SEVERITY(s) \
  (s & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT   * MSIC_SEVERITY_ERR)     + \
  (s & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT * MSIC_SEVERITY_WARN)    + \
  (s & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT    * MSIC_SEVERITY_INFO)    + \
  (s & VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT * MSIC_SEVERITY_VERBOSE); \

void msic_default_log_callback(const char *file, uint32_t line, const char *message, uint32_t severity);

VKAPI_ATTR VkBool32 VKAPI_CALL msic_vk_message_callback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData);

bool msic_check_layer(VkLayerProperties *layer_props, uint32_t layer_count, const char * layer_name); 

bool msic_check_layer_support(void); 

uint32_t msic_write_extensions(uint32_t extension_count, const char** pp_extension_names);

uint32_t msic_write_layers(uint32_t layer_count, const char** pp_layer_names); 

uint32_t msic_create_instance(MsicVulkanCore *core); 

uint32_t msic_setup_debug_messenger(MsicVulkanCore *core);

uint32_t msic_create_physical_device(MsicVulkanCore *core);

bool msic_default_phys_device_suitable(VkPhysicalDevice device, uint32_t *num_queue_indices, uint32_t **queue_indices);

uint32_t msic_default_choose_physical_device(uint32_t device_count, VkPhysicalDevice *devices, bool *suitable);

void msic_list_phys_device_features(VkPhysicalDeviceFeatures features);

#endif // MSIC_CORE_H
