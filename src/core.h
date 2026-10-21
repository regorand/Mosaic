#ifndef MSIC_CORE_H
#define MSIC_CORE_H

#include <stdint.h>
#include <stdbool.h>
#include "mosaic.h"

#define UNUSED(x) (void)(x)

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

void msic_glfw_callback(int error_code, const char* message);

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

uint32_t msic_init_glfw(void); 

void msic_populate_empty_window_context(MsicWindowContext *w_context);
uint32_t msic_create_windows(MsicVulkanCore *core);
uint32_t msic_create_surfaces(MsicVulkanCore *core);
uint32_t msic_query_surface_support(MsicVulkanCore *core, MsicWindowContext *w_context);

bool msic_check_swapchain_support(MsicVulkanCore *core);
uint32_t msic_create_swapchain(MsicVulkanCore *core);
void destroy_swapchain(MsicVulkanCore *core, VkSwapchainKHR swapchain);

uint32_t msic_create_physical_device(MsicVulkanCore *core);

uint32_t msic_create_device(MsicVulkanCore *core);

bool msic_default_phys_device_suitable(VkPhysicalDevice device, 
                                        uint32_t *present_queue_index, 
                                        uint32_t *graphics_queue_index,
                                        bool     *present_graphics_match,
                                        uint32_t **queue_indices, 
                                        MsicVulkanCore *core); 

uint32_t msic_default_choose_physical_device(uint32_t device_count, VkPhysicalDevice *devices, bool *suitable);

void msic_list_phys_device_features(VkPhysicalDeviceFeatures features);

VkSurfaceFormatKHR msic_default_choose_swapchain_format(MsicWindowContext *w_context, uint32_t index);
VkPresentModeKHR msic_default_choose_present_mode(MsicWindowContext *w_context, uint32_t index);
VkExtent2D msic_default_choose_swap_extent(MsicWindowContext *w_context, uint32_t index);

uint32_t msic_choose_image_count(MsicWindowContext *w_context, uint32_t index);

#endif // MSIC_CORE_H
