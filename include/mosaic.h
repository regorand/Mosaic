#ifndef MOSAIC_H
#define MOSAIC_H

#include <stdio.h>
#include <stdbool.h>
#include <vulkan/vulkan.h>
#include <vulkan/vk_enum_string_helper.h>
#include <stdarg.h>
#include <GLFW/glfw3.h>

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
 * Specifies which log level will be written by the default log callback 
 * Any MSIC_SEVERITY_<severity> macro is a valid value
 */
extern uint32_t msic_log_level;

/**
 * @brief: mosaic calls this function to generate log messages. 
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
 *
 */
#define MSIC_CALL(f) { \
  uint32_t res = (f); \
  if (res) { \
    MSIC_ERR("mosaic error: aborting!"); \
    return res; \
  } \
}

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
// State and functions
// -------------------

struct MsicWindowContext {
  /**
   * Boolean value indicating whether this window is still open
   *
   * If false, all handles are invalidated. This window can not be used anymore.
   */
  bool open;

  /**
   * Surface handle for this window. 
   * Surfaces and windows are created by glfw.
   */
  VkSurfaceKHR surface;
  
  /**
   * Vulkan object holding the surface capabilities of this windows surface.
   * See https://docs.vulkan.org/refpages/latest/refpages/source/VkSurfaceCapabilitiesKHR.html
   */
  VkSurfaceCapabilitiesKHR surface_capabilities;

  /**
   * Number of the supported Surface formats.
   * Length of array \link formats \endlink.
   */
  uint32_t num_supported_formats;

  /**
   * array of supported surface formats for this windows surface.
   * Length of the array is given in \link num_supported_formats \endlink.
   */
  VkSurfaceFormatKHR *formats;

  /**
   * Number of the supported present modes.
   * Length of array \link present_modes \endlink.
   */
  uint32_t num_present_modes;

  /**
   * array of supported present modes for this windows surface.
   * Length of the array is given in \link num_present_modes \endlink.
   */
  VkPresentModeKHR *present_modes;

  /**
   * Pointer handle to the Glfw window object.
   */
  GLFWwindow *window;

  /**
   * Vulkan handle to the swapchain for this image
   */
  VkSwapchainKHR swapchain;

  /**
   * Swapchain format chosen for this window's swapchain
   */
  VkFormat chosen_swapchain_format;

  /**
   * Current extent of this window's swapchain
   */
  VkExtent2D current_swap_extent;

  /**
   * Number of created swapchain images
   * This parameter specifies the length of \link swap_images \endlink array.
   */
  uint32_t num_swap_images;

  /**
   * Vulkan handles to the swapchain images for this window.
   * Length of this array is specifed by \link num_swap_images \endlink parameter.
   */
  VkImage *swap_images;
};

typedef struct MsicWindowContext MsicWindowContext;

/**
 * Structure holding the core vulkan instances
 */
struct MsicVulkanCore {
  /**
   * Vulkan instance handle, see https://docs.vulkan.org/refpages/latest/refpages/source/VkInstance.html
   */
  VkInstance instance;

  /**
   * Vulkan debug messenger handle, see https://docs.vulkan.org/refpages/latest/refpages/source/VkDebugUtilsMessengerEXT.html
   */
  VkDebugUtilsMessengerEXT debug_messenger;

  /**
   * Vulkan physical device handle, see https://docs.vulkan.org/refpages/latest/refpages/source/VkPhysicalDevice.html
   */
  VkPhysicalDevice physical_device;

  /**
   * Length of \link device_queue_indices \endlink array. 
   * Will match \link msic_num_extra_queue_families \endlink control parameter.
   */
  uint32_t num_device_queue_indices;

  /**
   * Array holding the indices to the additional requested queue families as required by \link msic_extra_queue_families_flags \endlink.
   * if \link msic_num_extra_queue_families \endlink is empty, this will not be set to a valid value.
   */
  uint32_t *device_queue_indices;
  
  /**
   * Vulkan device handle, see https://docs.vulkan.org/refpages/latest/refpages/source/VkDevice.html
   */
  VkDevice device;
 

  /**
   * Array of vulkan queue handles, see https://docs.vulkan.org/refpages/latest/refpages/source/VkQueue.html
   * Length of array is equal to the value in \link num_device_queue_indices \endlink.
   *
   * Will be matched by index to requested queues in \link msic_extra_queue_families_flags \endlink.
   *
   * Note that this array might contain repeats of queues, i.e. distinct indices into this array might not correspond to distinct VkQueue handles.
   * This is because queues can only be created once for each queue family. If multiple requested queues correspond to the same family, only one handle will be created and distributed to all indices in this array, for which the requested flags match.
   */
  VkQueue *queues;

  /**
   * Index of the queue guaranteed to support presentation.
   * Will only be set to a valid value if \link msic_present \endlink is true.
   *
   * Indexes into the \link MsicVulkanCore.queues \endlink array.
   */
  uint32_t present_queue_index;

  /**
   * Index of the queue guaranteed to support graphics operations.
   * Will only be set to a valid value of \link msic_graphics \endlink is true.
   *
   * Indexes into the \link MsicVulkanCore.queues \endlink array.
   */
  uint32_t graphics_queue_index;

  /**
   * Convenience parameter indicating whether the present queue and the graphics queue are the same.
   * Will always be false if \link msic_present \endlink or \link msic_graphics \endlink are false.
   */ 
  bool present_graphics_match;

  /**
   * Vulkan queue handle for the present queue.
   * Is set to a valid value if \link msic_present \endlink is set to true, otherwise undefined 
   */
  VkQueue present_queue;

  /**
   * Vulkan queue handle for the graphics queue.
   * Is set to a valid value if \link msic_graphics \endlink is set to true, otherwise undefined 
   */
  VkQueue graphics_queue;

  /**
   * Indicates number of created surfaces and specifies the length of the \link MsicVulkanCore.windows \endlink array.
   *
   * Value is equal to \link msic_num_windows \endlink control parameter at the time of calling. 
   *
   * Will always be zero if \link msic_headless \endlink is true.
   */
  uint32_t num_windows;
  
  /**
   * Array holding the window context for each requested winow. Length of array is specified in \link MsicVulkanCore.num_windows \endlink.
   *
   * Will always have length zero if \link msic_headless \endlink is true.
   * 
   * Window creation is handled by GLFW.
   */
  MsicWindowContext *windows;
};

typedef struct MsicVulkanCore MsicVulkanCore;

/**
 * Dummy initializer function used to assist early development 
 * TODO will be removed later
 */
void msic_dummy_init(void);

/**
 * Builds the vulkan core.
 * Makes decisions based on how the control parameters are set.
 *
 * @param[core]: Pointer to a \link MsicVulkanCore \endlink which will be filled with the vulkan handles
 * @returns: Exit status, any non-zero value indicates an error
 */
uint32_t msic_build_vulkan_core(MsicVulkanCore *core);
    
/**
 * Destroys the given vulkan core.
 *
 * @param[core]: Pointer to a \link MsicVulkanCore \endlink instance with valid vulkan handles which will be destroyed
 * @returns: Exit status, any non-zero value indicates an error
 */
uint32_t msic_destroy_vulkan_core(MsicVulkanCore *core);

/**
 * Closes and destroys the window specified by the given index. 
 * Windows will be unusable from that point onward. 
 *
 * Is not required to properly clean up windows, non-closed windows will be cleaned up by calling \link msic_destroy_vulkan_core \endlink.
 */
void msic_destroy_window(MsicVulkanCore *core, uint32_t window_index);

/**
 * Prints a list of device features and whether they are supported.
 * It prints through \link msic_log_callback \endlink.
 */
void msic_list_print_device_features(MsicVulkanCore *core);

// Include the control parameters
#include "control_params.h"

// Include the control functions
#include "control_functions.h"

#ifdef __cplusplus
}
#endif

#endif // MOSAIC_H
