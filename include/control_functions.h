#ifndef CONTROL_FUNCTION_H
#define CONTROL_FUNCTION_H

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
extern bool (*msic_phys_device_suitable)(VkPhysicalDevice, uint32_t *, uint32_t *, bool *, uint32_t **, MsicVulkanCore *);

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

extern VkSurfaceFormatKHR (*msic_choose_swapchain_format)(MsicWindowContext *, uint32_t);
extern VkPresentModeKHR   (*msic_choose_present_mode)(MsicWindowContext *, uint32_t);
extern VkExtent2D         (*msic_choose_swap_extent)(MsicWindowContext *, uint32_t);

#endif // CONTROL_FUNCTION_H
