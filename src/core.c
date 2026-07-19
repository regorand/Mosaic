#include "core.h"
#include "mosaic.h"
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <vulkan/vulkan.h>
#include <vulkan/vk_enum_string_helper.h>

void msic_default_log_callback(const char *file, uint32_t line, const char *message, uint32_t severity) {
  if (severity >= msic_log_level) {
    printf("mosaic | %20s:%-5d | %7s | %s\n", file, line, msic_severity_strings[severity], message);
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
  
  createInfo.messageSeverity =    VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT 
                                | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT;
                              //| VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT 
                              //| VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT; // TODO possible control parameter

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

bool msic_check_layer(VkLayerProperties *layer_props, uint32_t layer_count, const char * layer_name) {
  for (uint32_t i = 0; i < layer_count; i++) {
    if (strcmp(layer_props[i].layerName, layer_name)) {
      return true;
    }
  }

  return false;
}

bool msic_check_layer_support(void) {
  uint32_t layer_count;
  MSIC_VK_CALL(vkEnumerateInstanceLayerProperties(&layer_count, NULL));

  VkLayerProperties layer_props[layer_count];
  MSIC_VK_CALL(vkEnumerateInstanceLayerProperties(&layer_count, layer_props));
 
  if (msic_enable_validation) {
    if (!msic_check_layer(layer_props, layer_count, MSIC_VK_VALIDATION_LAYER_NAME)) {
      MSIC_ERR("Vk Validation layer not supported!");
      return false;
    }
  } 
    
  if (msic_layer_count > 0 && msic_layers == NULL) {
    MSIC_ERR("Requested %d layers but provided no names to msic_layers**", msic_layer_count);
    return false;
  }

  for (uint32_t i = 0; i < msic_layer_count; i++) {
    char *layer = msic_layers[i];
    if (!msic_check_layer(layer_props, layer_count, layer))  {
      MSIC_ERR("Requested layer %s is not available", layer);
      return false;
    }
  }

  return true;
}

uint32_t msic_write_extensions(uint32_t extension_count, const char** pp_extension_names) {
  memcpy(pp_extension_names, msic_extensions, msic_extension_count * sizeof(char *));

  if (msic_enable_validation) {
    if (extension_count < msic_extension_count + 1) {
      MSIC_ERR("extension_count not large enough for msic_extension_count and the debug utils extension");
      return 1;
    }

    pp_extension_names[msic_extension_count] = VK_EXT_DEBUG_UTILS_EXTENSION_NAME; 
  }

  return 0;
}

uint32_t msic_write_layers(uint32_t layer_count, const char** pp_layer_names) {
  memcpy(pp_layer_names, msic_layers, msic_layer_count * sizeof(char *));

  if (msic_enable_validation) {
    if (layer_count < msic_layer_count + 1) {
      MSIC_ERR("layer_count not large enough for msic_layer_count and the validation layer");
      return 1;
    }

    pp_layer_names[msic_layer_count] = MSIC_VK_VALIDATION_LAYER_NAME; 
  }

  return 0;
}


uint32_t msic_create_instance(MsicVulkanCore *core) {
  MSIC_INFO("Creating Vulkan Instance");

  VkApplicationInfo appInfo;
  appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  appInfo.pApplicationName = msic_application_name; 
  appInfo.applicationVersion = msic_application_version;
  appInfo.pEngineName = msic_engine_name;
  appInfo.engineVersion = msic_engine_version;
  appInfo.apiVersion = msic_vk_api_version; 

  VkInstanceCreateInfo createInfo;
  createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  createInfo.pApplicationInfo = &appInfo;


  createInfo.enabledExtensionCount = msic_extension_count + msic_enable_validation;
  const char *extension_names[createInfo.enabledExtensionCount];
  
  if (msic_write_extensions(createInfo.enabledExtensionCount, extension_names)) {
    MSIC_ERR("Failed to write vulkan extension names");
    return 1;
  }
  
  createInfo.ppEnabledExtensionNames = (const char * const *) extension_names;


  createInfo.enabledLayerCount = msic_layer_count + msic_enable_validation;
  const char *layer_names[createInfo.enabledLayerCount]; 

  if (msic_write_layers(createInfo.enabledLayerCount, layer_names)) {
    MSIC_ERR("Failed to write vulkan layer names");
    return 1;
  }
  createInfo.ppEnabledLayerNames = (const char * const *) layer_names;

  for (uint32_t i = 0; i < createInfo.enabledExtensionCount; i++) {
    MSIC_VERBOSE("\tEnabling extension: %s", extension_names[i]);
  }

  for (uint32_t i = 0; i < createInfo.enabledLayerCount; i++) {
    MSIC_VERBOSE("\tEnabling layer: %s", layer_names[i]);
  }
  
  MSIC_VK_CALL(vkCreateInstance(&createInfo, NULL, &core->instance));

  return 0;
}

uint32_t msic_create_physical_device(MsicVulkanCore *core) {
  UNUSED(core);
  uint32_t device_count = 0;
  MSIC_VK_CALL(vkEnumeratePhysicalDevices(core->instance, &device_count, NULL));

  if (device_count == 0) {
    MSIC_ERR("No devices with vulkan support found!");
    return 1;
  }
  
  MSIC_INFO("Found %d devices", device_count);

  VkPhysicalDevice devices[device_count];
  MSIC_VK_CALL(vkEnumeratePhysicalDevices(core->instance, &device_count, devices));

  uint32_t  arr_num_queue_indices[device_count];
  uint32_t *arr_queue_indices[device_count];
  bool      suitable[device_count];

  uint32_t num_suitable = 0;

  for (uint32_t i = 0; i < device_count; i++) {
    // uint32_t num_queue_indices = 0;
    // uint32_t *queue_indices = NULL;

    suitable[i] = (*msic_phys_device_suitable)(devices[i], &(arr_num_queue_indices[i]), &(arr_queue_indices[i]));
    if (suitable[i]) num_suitable++;
  }

  if (num_suitable == 0) {
    MSIC_ERR("No suitable device was found");
    return 1;
  }

  uint32_t chosen_device_index = (*msic_default_choose_physical_device)(device_count, devices, suitable);
  
  // Suitable devices that were not chosen still have their queue indices malloc'ed, free them here
  for (uint32_t i = 0; i < device_count; i++) {
    if (suitable[i] && i != chosen_device_index) {
      free(arr_queue_indices[i]);
    }
  }


  VkPhysicalDeviceProperties props;
  vkGetPhysicalDeviceProperties(devices[chosen_device_index], &props);

  MSIC_INFO("Chose device %s with index %d", props.deviceName, chosen_device_index);
  
  core->physical_device =           devices[chosen_device_index];
  core->num_device_queue_indices =  arr_num_queue_indices[chosen_device_index];
  core->device_queue_indices =      arr_queue_indices[chosen_device_index];

  return 0;
}

bool msic_default_phys_device_suitable(VkPhysicalDevice device, uint32_t *num_queue_indices, uint32_t **queue_indices) {
  VkPhysicalDeviceProperties props;
  vkGetPhysicalDeviceProperties(device, &props);

  bool suitable = true;
  
  MSIC_INFO("Device %s: Checking suitability", props.deviceName);

  // --------------
  // Device type
  // --------------
  if (msic_physical_device_type != -1) {
    if ((int32_t) props.deviceType != msic_physical_device_type) {
      MSIC_VERBOSE("\tType %s(%d) not equal to requested device type %s(%d)", string_VkPhysicalDeviceType(props.deviceType), props.deviceType, string_VkPhysicalDeviceType(msic_physical_device_type), msic_physical_device_type);
      suitable = false;
    }
  }  

  // --------------
  // Device features 
  // --------------
  //
  VkPhysicalDeviceFeatures features;
  vkGetPhysicalDeviceFeatures(device, &features);

  bool all_requested_supported = true;

  // Small hack by casting the structs to VkBool32 arrays and jointly iterating over requested features and given device features
  uint32_t num_features = sizeof(VkPhysicalDeviceFeatures) / sizeof(VkBool32);
  for (uint32_t i = 0; i < num_features; i++) {
    VkBool32 requested_support = ((VkBool32*) &msic_required_physical_device_features)[i];
    VkBool32 device_support = ((VkBool32*) &features)[i];
    if (requested_support && !device_support) {
      MSIC_VERBOSE("\tRequested feature support for feature #%2d, but device does not support that feature", i);
      all_requested_supported = false;
    }
  }

  if (all_requested_supported) {
    MSIC_VERBOSE("\tAll requested features supported");
  } else {
    suitable = false;
  }

  // --------------
  // Queue families 
  // --------------
  uint32_t queue_families_count = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_families_count, NULL);
  
  bool require_free_on_fail = false;
  if (queue_families_count == 0) {
    MSIC_INFO("Device %s has no queue families", props.deviceName);
    suitable = false;
  } else {
    VkQueueFamilyProperties queue_families[queue_families_count];
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_families_count, queue_families);

    if (msic_num_queue_families == 0) {
      *num_queue_indices = 1;

      *queue_indices = malloc(sizeof(uint32_t));
      require_free_on_fail = true;
      
      *queue_indices[0] = 0;
    } else {
      *num_queue_indices = msic_num_queue_families;
      *queue_indices = malloc(sizeof(uint32_t) * msic_num_queue_families);
      require_free_on_fail = true;

      for (uint32_t i = 0; i < msic_num_queue_families; i++) {
        bool found_queue_family = false;

        for (uint32_t j = 0; j < queue_families_count; j++) {
          if ((msic_queue_families_flags[i] & queue_families[j].queueFlags) == msic_queue_families_flags[i]) {
            (*queue_indices)[i] = j;   

            found_queue_family = true;
            // Don't look for other queue families, which could overwrite this
            break;
          }

        }

        if (!found_queue_family) {
          MSIC_VERBOSE("\tNo queue family for flag %d", msic_queue_families_flags[i]);
          suitable = false;
        }
      }
    }
  }

  if (!suitable) {
    MSIC_INFO("Device %s unsuitable", props.deviceName);
    if (require_free_on_fail) free(*queue_indices);
    return false;
  }

  MSIC_INFO("Device %s suitable", props.deviceName);
  return true;
}

uint32_t msic_default_choose_physical_device(uint32_t device_count, VkPhysicalDevice *devices, bool *suitable) {
  UNUSED(devices);
  for (uint32_t i = 0; i < device_count; i++) {
    if (suitable[i]) return i;
  }

  MSIC_ERR("Asked to choose a physical device when none suitable were present");
  return UINT32_MAX;
}

void msic_list_phys_device_features(VkPhysicalDeviceFeatures features) {
     MSIC_VERBOSE("\trobustBufferAccess: %s",features.robustBufferAccess ? "true" : "false");
     MSIC_VERBOSE("\tfullDrawIndexUint32: %s",features.fullDrawIndexUint32 ? "true" : "false");
     MSIC_VERBOSE("\timageCubeArray: %s",features.imageCubeArray ? "true" : "false");
     MSIC_VERBOSE("\tindependentBlend: %s",features.independentBlend ? "true" : "false");
     MSIC_VERBOSE("\tgeometryShader: %s",features.geometryShader ? "true" : "false");
     MSIC_VERBOSE("\ttessellationShader: %s",features.tessellationShader ? "true" : "false");
     MSIC_VERBOSE("\tsampleRateShading: %s",features.sampleRateShading ? "true" : "false");
     MSIC_VERBOSE("\tdualSrcBlend: %s",features.dualSrcBlend ? "true" : "false");
     MSIC_VERBOSE("\tlogicOp: %s",features.logicOp ? "true" : "false");
     MSIC_VERBOSE("\tmultiDrawIndirect: %s",features.multiDrawIndirect ? "true" : "false");
     MSIC_VERBOSE("\tdrawIndirectFirstInstance: %s",features.drawIndirectFirstInstance ? "true" : "false");
     MSIC_VERBOSE("\tdepthClamp: %s",features.depthClamp ? "true" : "false");
     MSIC_VERBOSE("\tdepthBiasClamp: %s",features.depthBiasClamp ? "true" : "false");
     MSIC_VERBOSE("\tfillModeNonSolid: %s",features.fillModeNonSolid ? "true" : "false");
     MSIC_VERBOSE("\tdepthBounds: %s",features.depthBounds ? "true" : "false");
     MSIC_VERBOSE("\twideLines: %s",features.wideLines ? "true" : "false");
     MSIC_VERBOSE("\tlargePoints: %s",features.largePoints ? "true" : "false");
     MSIC_VERBOSE("\talphaToOne: %s",features.alphaToOne ? "true" : "false");
     MSIC_VERBOSE("\tmultiViewport: %s",features.multiViewport ? "true" : "false");
     MSIC_VERBOSE("\tsamplerAnisotropy: %s",features.samplerAnisotropy ? "true" : "false");
     MSIC_VERBOSE("\ttextureCompressionETC2: %s",features.textureCompressionETC2 ? "true" : "false");
     MSIC_VERBOSE("\ttextureCompressionASTC_LDR: %s",features.textureCompressionASTC_LDR ? "true" : "false");
     MSIC_VERBOSE("\ttextureCompressionBC: %s",features.textureCompressionBC ? "true" : "false");
     MSIC_VERBOSE("\tocclusionQueryPrecise: %s",features.occlusionQueryPrecise ? "true" : "false");
     MSIC_VERBOSE("\tpipelineStatisticsQuery: %s",features.pipelineStatisticsQuery ? "true" : "false");
     MSIC_VERBOSE("\tvertexPipelineStoresAndAtomics: %s",features.vertexPipelineStoresAndAtomics ? "true" : "false");
     MSIC_VERBOSE("\tfragmentStoresAndAtomics: %s",features.fragmentStoresAndAtomics ? "true" : "false");
     MSIC_VERBOSE("\tshaderTessellationAndGeometryPointSize: %s",features.shaderTessellationAndGeometryPointSize ? "true" : "false");
     MSIC_VERBOSE("\tshaderImageGatherExtended: %s",features.shaderImageGatherExtended ? "true" : "false");
     MSIC_VERBOSE("\tshaderStorageImageExtendedFormats: %s",features.shaderStorageImageExtendedFormats ? "true" : "false");
     MSIC_VERBOSE("\tshaderStorageImageMultisample: %s",features.shaderStorageImageMultisample ? "true" : "false");
     MSIC_VERBOSE("\tshaderStorageImageReadWithoutFormat: %s",features.shaderStorageImageReadWithoutFormat ? "true" : "false");
     MSIC_VERBOSE("\tshaderStorageImageWriteWithoutFormat: %s",features.shaderStorageImageWriteWithoutFormat ? "true" : "false");
     MSIC_VERBOSE("\tshaderUniformBufferArrayDynamicIndexing: %s",features.shaderUniformBufferArrayDynamicIndexing ? "true" : "false");
     MSIC_VERBOSE("\tshaderSampledImageArrayDynamicIndexing: %s",features.shaderSampledImageArrayDynamicIndexing ? "true" : "false");
     MSIC_VERBOSE("\tshaderStorageBufferArrayDynamicIndexing: %s",features.shaderStorageBufferArrayDynamicIndexing ? "true" : "false");
     MSIC_VERBOSE("\tshaderStorageImageArrayDynamicIndexing: %s",features.shaderStorageImageArrayDynamicIndexing ? "true" : "false");
     MSIC_VERBOSE("\tshaderClipDistance: %s",features.shaderClipDistance ? "true" : "false");
     MSIC_VERBOSE("\tshaderCullDistance: %s",features.shaderCullDistance ? "true" : "false");
     MSIC_VERBOSE("\tshaderFloat64: %s",features.shaderFloat64 ? "true" : "false");
     MSIC_VERBOSE("\tshaderInt64: %s",features.shaderInt64 ? "true" : "false");
     MSIC_VERBOSE("\tshaderInt16: %s",features.shaderInt16 ? "true" : "false");
     MSIC_VERBOSE("\tshaderResourceResidency: %s",features.shaderResourceResidency ? "true" : "false");
     MSIC_VERBOSE("\tshaderResourceMinLod: %s",features.shaderResourceMinLod ? "true" : "false");
     MSIC_VERBOSE("\tsparseBinding: %s",features.sparseBinding ? "true" : "false");
     MSIC_VERBOSE("\tsparseResidencyBuffer: %s",features.sparseResidencyBuffer ? "true" : "false");
     MSIC_VERBOSE("\tsparseResidencyImage2D: %s",features.sparseResidencyImage2D ? "true" : "false");
     MSIC_VERBOSE("\tsparseResidencyImage3D: %s",features.sparseResidencyImage3D ? "true" : "false");
     MSIC_VERBOSE("\tsparseResidency2Samples: %s",features.sparseResidency2Samples ? "true" : "false");
     MSIC_VERBOSE("\tsparseResidency4Samples: %s",features.sparseResidency4Samples ? "true" : "false");
     MSIC_VERBOSE("\tsparseResidency8Samples: %s",features.sparseResidency8Samples ? "true" : "false");
     MSIC_VERBOSE("\tsparseResidency16Samples: %s",features.sparseResidency16Samples ? "true" : "false");
     MSIC_VERBOSE("\tsparseResidencyAliased: %s",features.sparseResidencyAliased ? "true" : "false");
     MSIC_VERBOSE("\tvariableMultisampleRate: %s",features.variableMultisampleRate ? "true" : "false");
     MSIC_VERBOSE("\tinheritedQueries: %s",features.inheritedQueries ? "true" : "false");
}
