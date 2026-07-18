#include "core.h"
#include "mosaic.h"
#include <stdint.h>
#include <vulkan/vulkan.h>

void msic_default_log_callback(const char *file, uint32_t line, const char *message, uint32_t severity) {
  if (severity >= msic_log_level) {
    printf("mosaic | %s:%-5d | %7s | %s\n", file, line, msic_severity_strings[severity], message);
  }
}

VKAPI_ATTR VkBool32 VKAPI_CALL msic_vk_message_callback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData) {

  UNUSED(messageType);
  UNUSED(pUserData);

  uint32_t msic_severity = VK_SEVERITY_TO_MSIC_SEVERITY(messageSeverity);
  msic_output_log("vk validation", 0, msic_severity, pCallbackData->pMessage);

  return VK_FALSE;
}


uint32_t msic_setup_debug_messenger(MsicVulkanCore *core) {
  VkDebugUtilsMessengerCreateInfoEXT createInfo = {0};

  createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT; 
  
  createInfo.messageSeverity =    VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT 
                                | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT 
                             // | VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT 
                                | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT; // TODO possible control parameter

  createInfo.messageType     =    VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
                                | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
                                | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT; // TODO possible control parameter
                                                                                   
  createInfo.pfnUserCallback = &msic_vk_message_callback;
  createInfo.pUserData = NULL; // TODO possible control parameter

  PFN_vkCreateDebugUtilsMessengerEXT func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(core->instance, "vkCreateDebugUtilsMessengerEXT");
  if (!func) {
    MSIC_ERR("Could not load vkCreateDebugUtilsMessengerEXT");
    return 1;
  }

  MSIC_VK_CALL(func(core->instance, &createInfo, NULL, &core->debug_messenger));
  return 0;
}
