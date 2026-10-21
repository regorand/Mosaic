#ifndef  CONTROL_PARAMS_H
#define CONTROL_PARAMS_H

#include <stdbool.h>
#include <stdint.h>
#include <vulkan/vulkan.h>

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
 * Control parameter specifying number of additional extensions in \link msic_extensions \endlink 
 */
extern uint32_t msic_extension_count;

/** 
 * Control parameter specifying additional vulkan extensions to enable.
 * Number of extensions must be specified in \link msic_extension_count \endlink
 */
extern char **msic_extensions;

/** 
 * Control parameter specifying number of additional layers in \link msic_layers \endlink
 */
extern uint32_t msic_layer_count;

/** 
 * Control parameter specifying additional vulkan layers to enable.
 * Number of layers must be specified in \link msic_layer_count \endlink
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
 * Control parameter specifing the vulkan api version, see https://docs.vulkan.org/refpages/latest/refpages/source/VkInstanceCreateInfo.html

 */
extern uint32_t msic_vk_api_version;

/**
 *
 */
extern int32_t msic_physical_device_type;

/**
 * Control parameter specifying the required physical device feature set.
 *
 * Requires any feature set to `true` to be supported by chosen physical device
 */
extern VkPhysicalDeviceFeatures msic_required_physical_device_features;

/**
 * Control parameter specifying whether vulkan is being run headless, i.e. without a window
 * If false, mosaic will require a present queue to be present and set the corresponding queue index variable in \link MsicVulkanCore \endlink.
 * TODO Remove and replace with \link msic_present \endlink.

 */
extern bool msic_headless;

/**
 * Control parameter specifying whether application will present to the screen 
 * If true, mosaic will require a present queue to be present and set the corresponding queue index variable in \link MsicVulkanCore \endlink.

 */
extern bool msic_present;

/**
 * Control parameter specifying whether application will perform graphics operations.
 * If true, will require a graphics queue and fill the corresponding graphics queue index variable in \link MsicVulkanCore \endlink.
 */
extern bool msic_graphics;

/**
 * Control parameter specifying how many different queue families are required
 * Must be the length of the array in \link msic_extra_queue_families_flags \endlink
 */
extern uint32_t msic_num_extra_queue_families;

/**
 * Control parameter array of uint32_t, each specifying required queue family flags
 * Length of the array must be written in \link msic_num_extra_queue_families \endlink
 *
 * If no required flags are provided, i.e. the length of this array is 0, mosaic will choose the first queue it finds.
 */
extern uint32_t *msic_extra_queue_families_flags;

/** 
 * Control parameter specifying number of device extensions when creating \link MsicVulkanCore.device \endlink.
 * Must be the length of the array \link msic_device_extensions \endlink. 
 */
extern uint32_t msic_device_extension_count;

/** 
 * Control parameter specifying which device extensions to enable
 * Length of the array must be written in \link msic_device_extension_count \endlink.
 */
extern char **msic_device_extensions;


/** 
 * Control parameter specifying the number of windows to create
 */
extern uint32_t msic_num_windows;

// TODO need more window control parameters: title, sizes, monitors etc.


/**
 * Control parameter specifying the glfw window hint counts for each window.
 * Assumed to have length \link msic_num_windows \endlink or be NULL.
 *
 * If this is NULL but \link msic_num_windows \endlink is greater than 0, mosaic assumes no window hints for all windows.
 */
extern uint32_t *msic_window_hint_counts;

/**
 * Control parameter specifying the glfw window hints for each window.
 * First index specifies window and has length \link msic_num_windows \endlink.
 * Second index specifies the window hint and has a length specified by the corresponding window index in \link msic_window_hint_counts \endlink.
 *
 * Hint values are matched by index in \link msic_window_hint_values \endlink.
 */
extern int32_t **msic_window_hints;

/**
 * Control parameter specifying the glfw window hint values for each window.
 * First index specifies window and has length \link msic_num_windows \endlink.
 * Second index specifies the window hint value and has a length specified by the corresponding window index in \link msic_window_hint_counts \endlink.
 *
 * Hints are matched by index in \link msic_window_hints \endlink.
 */
extern int32_t **msic_window_hint_values;

/*
 * Control parameters specifying the requested swapchain surface formats.
 * Must be NULL or an array of length \link msic_num_windows \endlink
 * If NULL, a format will be chosen for every swapchain by the callback function \link msic_choose_swapchain_format \endlink
 * To request no special format for a surface set the VkFormat field to VK_FORMAT_UNDEFINED
 * If not NULL, fails if swapchains do not support the requested format 
 */
extern VkSurfaceFormatKHR *msic_swapchain_formats;
 
/*
 * Control parameter specifying the requested swapchain present modes.
 * Must NULL or an array of length \link msic_num_windows \endlink.
 * If NULL, a format will be chosen for every swapchain by the callback function \link msic_choose_present_mode \endlink
 * If not NULL, fails if swapchains do not support the requested present modes
 */
extern VkPresentModeKHR *msic_present_modes;

/*
 * Control parameter specifying the swapchain image array layers count.
 * Must NULL or an array of length \link msic_num_windows \endlink.
 * If NULL, all image layer counts will be 1.
 */
extern int *msic_swapchain_image_layers;

/*
 * Control parameter specifying the swapchain image usage flags.
 * Must NULL or an array of length \link msic_num_windows \endlink.
 * If NULL, all image usage bits will be set to VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT
 */
extern VkImageUsageFlags *msic_swapchain_image_usage_flags;

// TODO
// surfaces, windows, swapchains
//
// 1. surface creation
//    -> include glfw, use that to create surfaces, but provide overridable callback for surface creation
//
// 2. specify how many windows are wanted
//
// 3. create swapchains for each, figure out how to specify swapchain settings for each -> just array ? 
//
//
//

#endif // CONTROL_PARAMS_H
