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
   * Will match \link msic_num_queue_families \endlink control parameter if it is greater than zero. 
   */
  uint32_t num_device_queue_indices;

  /**
   * Array holding the indices to queue families as required by \link msic_queue_families_flags \endlink.
   * If \link msic_queue_families_flags \endlink was empty contains a single index to the first queue found.
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
   * Will be matched by index to requested queues in \link msic_queue_families_flags \endlink.
   *
   * Note that this array might contain repeats of queues, i.e. distinct indices into this array might not correspond to distinct VkQueue handles.
   * This is because queues can only be created once for each queue family. If multiple requested queues correspond to the same family, only one handle will be created and distributed to all indices in this array, for which the requested flags match.
   */
  VkQueue *queues;

  /**
   * Index of the queue guaranteed to support presentation.
   * Will only be set to a valid value if \link msic_headless \endlink is false.
   *
   * Indexes into the \link MsicVulkanCore.queues \endlink array.
   */
  uint32_t present_queue_index;

  /**
   * Indicates number of created surfaces and specifies the length of the \link MsicVulkanCore.surfaces \endlink array.
   *
   * Value is equal to \link msic_num_windows \endlink control parameter at the time of calling. 
   *
   * Will always be zero if \link msic_headless \endlink is true.
   */
  uint32_t num_windows;

  /**
   * Array holding the requested surface handles. Length of the array is specified in \link MsicVulkanCore.num_windows \endlink.
   *
   * Will always be zero if \link msic_headless \endlink is true.
   *
   * Surfaces and windows are created by glfw.
   */
  VkSurfaceKHR *surfaces;
  

  /**
   * Array holding pointers to the requested glfw windows. Length of array is specified in \link MsicVulkanCore.num_windows \endlink.
   *
   * Will always be zero if \link msic_headless \endlink is true.
   */
  GLFWwindow **windows;
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
 * Control parameter specifying how many different queue families are required
 * Must be the length of the array in \link msic_queue_families_flags \endlink
 */
extern uint32_t msic_num_queue_families;

/**
 * Control parameter array of uint32_t, each specifying required queue family flags
 * Length of the array must be written in \link msic_num_queue_families \endlink
 *
 * If no required flags are provided, i.e. the length of this array is 0, mosaic will choose the first queue it finds.
 */
extern uint32_t *msic_queue_families_flags;

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
 * Control parameter specifying whether vulkan is being run headless, i.e. without a window
 */
extern bool msic_headless;

/** 
 * Control parameter specifying the number of windows to create
 */
extern uint32_t msic_num_windows;

// TODO need more window control parameters: title, sizes, monitors etc.


/**
 * Control parameter specifying the glfw window hint counts for each window.
 * Assumed to have length \link msic_num_windows \endlink.
 *
 * If this is NULL but \link msic_num_windows \endlink is greather than 0, mosaic assumes no window hints for all windows.
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




// -------------------
// Control functions
// -------------------

/**
 * Control function pointer for callback for mosaic logs. 
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
 * Control function pointer for physical device suitability check.
 * Mosaic will set a default function for this which can be controlled using various control parameters which each with `msic_physical_device`. 
 *
 * For most applications, the default and its control parameters should be enough. 
 * This control function exists in case more fine grained control over device suitability is needed. 
 *
 * @param: Device handle which is to be tested for suitability, see https://docs.vulkan.org/refpages/latest/refpages/source/VkPhysicalDevice.html
 * @returns: bool value indicating whether the device is suitable
 */
extern bool (*msic_phys_device_suitable)(VkPhysicalDevice, uint32_t *, uint32_t **, MsicVulkanCore *);

/** 
 * Control function pointer which chooses the physical device
 * Mosaic provides a default implementation which always chooses the first valid physical device. 
 *
 * For better control over which physical device to choose provide the pointer to a different function here
 *
 * @param: total count of devices, gives array lenghts of other two parameters
 * @param: array of device handles, see https://docs.vulkan.org/refpages/latest/refpages/source/VkPhysicalDevice.html
 * @param: array of bools, specifying whether the index matched device handle is suitable
 *
 * @returns: Index of chosen device or UINT32_MAX on failure
 */
extern uint32_t(*msic_choose_physical_device)(uint32_t, VkPhysicalDevice *, bool *); 


#ifdef __cplusplus
}
#endif

#endif // MOSAIC_H
