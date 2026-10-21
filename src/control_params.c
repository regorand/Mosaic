// #include "mosaic.h"
#include "control_params.h"
#include "core.h"

FILE **msic_log_output_stream = &stdout; 
FILE **msic_err_output_stream = &stderr;

const char *msic_severity_strings[] = { "VERBOSE", "INFO", "WARN", "ERROR" };
void (*msic_log_callback)(const char*, uint32_t, const char*, uint32_t) = &msic_default_log_callback;
uint32_t msic_log_level = MSIC_SEVERITY_INFO;

bool msic_initialized = false;
bool msic_enable_validation = true;

char **msic_extensions = NULL;
uint32_t msic_extension_count = 0;

char **msic_layers = NULL;
uint32_t msic_layer_count = 0;

const char* msic_application_name = "Mosaic Application";
uint32_t msic_application_version = VK_MAKE_VERSION(1, 0, 0);

const char* msic_engine_name = "No Engine";
uint32_t msic_engine_version = VK_MAKE_VERSION(1, 0, 0);

uint32_t msic_vk_api_version = VK_API_VERSION_1_0;

int32_t msic_physical_device_type = -1;

VkPhysicalDeviceFeatures msic_required_physical_device_features = {0};

bool (*msic_phys_device_suitable)(VkPhysicalDevice, uint32_t *, uint32_t *, bool *, uint32_t **, MsicVulkanCore *) = &msic_default_phys_device_suitable;

uint32_t (*msic_choose_physical_device)(uint32_t, VkPhysicalDevice *, bool *) = &msic_default_choose_physical_device; 

uint32_t msic_num_extra_queue_families = 0;

uint32_t *msic_extra_queue_families_flags = NULL;

uint32_t msic_device_extension_count = 0;

char **msic_device_extensions = NULL;

bool msic_headless = false;

bool msic_present =  true;
bool msic_graphics = true;

uint32_t msic_num_windows = 1;

uint32_t *msic_window_hint_counts = NULL;

int32_t **msic_window_hints = NULL;

int32_t **msic_window_hint_values = NULL;

VkSurfaceFormatKHR *msic_swapchain_formats = NULL;

VkPresentModeKHR *msic_present_modes = NULL;

VkSurfaceFormatKHR (*msic_choose_swapchain_format)(MsicWindowContext *, uint32_t) = &msic_default_choose_swapchain_format;
VkPresentModeKHR   (*msic_choose_present_mode)(MsicWindowContext *, uint32_t)     = &msic_default_choose_present_mode;
VkExtent2D         (*msic_choose_swap_extent)(MsicWindowContext *, uint32_t)      = &msic_default_choose_swap_extent;

uint32_t *msic_swapchain_image_layers = NULL;

VkImageUsageFlags *msic_swapchain_image_usage_flags = NULL;
