#include "device.h"
#include "core/core.h"

#include <stdlib.h>
#include <string.h>

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

  uint32_t  arr_present_indices[device_count];
  uint32_t  arr_graphics_indices[device_count];
  uint32_t *arr_queue_indices[device_count];
  bool      arr_present_graphics_match[device_count];
  bool      suitable[device_count];

  for (uint32_t i = 0; i < device_count; i++) {
    arr_queue_indices[i] = malloc(msic_num_extra_queue_families * sizeof(uint32_t));
  } 

  uint32_t num_suitable = 0;

  for (uint32_t i = 0; i < device_count; i++) {
    suitable[i] = (*msic_phys_device_suitable)(devices[i], 
                                               &(arr_present_indices[i]), 
                                               &(arr_graphics_indices[i]), 
                                               &(arr_present_graphics_match[i]), 
                                               &(arr_queue_indices[i]), 
                                               core);

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
  
  core->physical_device          = devices[chosen_device_index];
  core->num_device_queue_indices = msic_num_extra_queue_families;
  if (msic_present && msic_graphics) {
    core->present_graphics_match = arr_present_graphics_match[chosen_device_index];
  } else {
    core->present_graphics_match = false;
  }
  core->device_queue_indices     = arr_queue_indices[chosen_device_index];

  return 0;
}

bool msic_default_phys_device_suitable(VkPhysicalDevice device, 
                                        uint32_t *present_queue_index, 
                                        uint32_t *graphics_queue_index,
                                        bool     *present_graphics_match,
                                        uint32_t **queue_indices, 
                                        MsicVulkanCore *core) {

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

  VkQueueFamilyProperties queue_family_props[queue_families_count];
  vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_families_count, queue_family_props);

  bool present_support[queue_families_count]; 
  bool graphics_support[queue_families_count];

  bool extra_queues_support[msic_num_extra_queue_families][queue_families_count];
  
  bool any_present_support = false;
  bool any_graphics_support = false;

  bool extra_any_support[msic_num_extra_queue_families];

  for (uint32_t i = 0; i < msic_num_extra_queue_families; i++) {
    extra_any_support[i] = false;
  }

  for (uint32_t i = 0; i < queue_families_count; i++) {
    // Check for presentation support
    if (msic_present) {
      bool supported = true;

      for (uint32_t j = 0; j < core->num_windows; j++) {
        MsicWindowContext *w_context = &core->windows[j];
        VkBool32 present_supported = false;

        MSIC_VK_CALL(vkGetPhysicalDeviceSurfaceSupportKHR(device, i, w_context->surface, &present_supported));

        any_present_support |= present_supported;
        supported &= present_supported;
      }

      present_support[i] = supported;
    }


    // Check for graphics support
    if (msic_graphics) {
      graphics_support[i] = (queue_family_props[i].queueFlags & VK_QUEUE_GRAPHICS_BIT);
      any_graphics_support |= graphics_support[i];
    }

    // Check other requested queues
    for (uint32_t j = 0; j < msic_num_extra_queue_families; j++) {
      extra_queues_support[j][i] = (queue_family_props[i].queueFlags & msic_extra_queue_families_flags[j]) == msic_extra_queue_families_flags[j];
      extra_any_support[j] = extra_queues_support[j][i];
    }
  }
  
  if (msic_present && !any_present_support) {
    MSIC_VERBOSE("No present support on device %s", props.deviceName);
    return false;
  } 

  if (msic_graphics && !any_graphics_support) {
    MSIC_VERBOSE("No graphics support on device %s", props.deviceName); 
    return false;
  }

  for (uint32_t i = 0; i < msic_num_extra_queue_families; i++) {
    if (!extra_any_support[i]) {
      MSIC_VERBOSE("No support for requested queue flags at index %d", i);
      return false;
    }
  }

  // Check if we have a queue that supports both presentation and graphics operations.
  // If so, choose that one. If multiple, use lower index

  uint32_t first_present_support = 0;
  uint32_t first_graphics_support = 0;

  if (msic_present && msic_graphics) {
    for (uint32_t i = 0; i < queue_families_count; i++) {
      if (present_support[i]) first_present_support = i;
      if (graphics_support[i]) first_graphics_support = i;

      if (present_support[i] && graphics_support[i]) {
        *present_queue_index = i;
        *graphics_queue_index = i;
        *present_graphics_match = true;
        break;
      } 
    } 
  }

  if (!present_graphics_match) {
    // Don't care to check msic_present and msic_graphics here, if those are false the values are 0 and supposed to be invalid anyways
    *present_queue_index = first_present_support;
    *graphics_queue_index = first_graphics_support; 
  }

  for (uint32_t i = 0; i < msic_num_extra_queue_families; i++) {
    int first_test_index = -1;
    if (msic_present)       first_test_index = (int) *present_queue_index;
    else if (msic_graphics) first_test_index = (int) *graphics_queue_index;

    // Choose a queue we use for presentation or graphics if possible
    if (first_test_index != -1) {
      if (extra_queues_support[i][first_test_index]) {
        (*queue_indices)[i] = (uint32_t) first_test_index;  
        continue;
      }
    }

    for (uint32_t j = 0; j < queue_families_count; j++) {
      // Bit confusing, in this double loop the extra requested queues are outside, the available queues inside loop
      // This is reverse from the loop above where we check support, so the indices i and j are the other way around here
      if (extra_queues_support[i][j]) {
        (*queue_indices)[i] = j;  
        continue;
      }
    }
  }

  // --------------
  // Device Extension support 
  // --------------
  MSIC_VERBOSE("\tChecking device extension support");
  if (!msic_check_device_extension_support(device)) {
    MSIC_VERBOSE("\tRequested device extensions not supported"); 
    suitable = false;
  }

  if (!suitable) {
    MSIC_INFO("Device %s unsuitable", props.deviceName);
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

void msic_list_print_device_features(MsicVulkanCore *core) {
  VkPhysicalDeviceFeatures features;
  vkGetPhysicalDeviceFeatures(core->physical_device, &features);

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

uint32_t msic_create_device(MsicVulkanCore *core) {
  VkDeviceCreateInfo create_info = {0};
  create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
  create_info.pNext = NULL;
  create_info.flags = 0;
  
  // The problem here is that the user can request different queues with different flags
  // However, if multiple of these point to the same queue, I may not request multiple of these
  // 
  // The following code reduces the queue indices down to distinct ones, creates only distinct queues
  // and then distributes the obtained VkQueue handles into core->queues so that if two requested 
  // queues get the same queue index, the corresponding handle will be written to *both* positions
  // in core->queues
  // 
  // A better option might be to increase the queueCount for duplicated queues

  uint32_t num_queues = core->num_device_queue_indices;
  
  uint32_t prefix_queues = 0;
  if (msic_present) {
    prefix_queues++;
  }
  if (msic_graphics) {
    prefix_queues++;
  }

  uint32_t queue_indices[prefix_queues + num_queues];
  if (msic_present) {
    queue_indices[0] = core->present_queue_index;
  }

  if (msic_graphics) {
    queue_indices[msic_present ? 1 : 0] = core->graphics_queue_index;
  }

  memcpy(queue_indices + prefix_queues, core->device_queue_indices, core->num_device_queue_indices * sizeof(uint32_t));
  num_queues += prefix_queues;

  uint32_t distinct_queues_first_indices[num_queues];
  uint32_t queue_indirection_index      [num_queues];
  uint32_t distinct_queues_count = 0;

  for (uint32_t i = 0; i < num_queues; i++) {
    // Check if index is already known  
    
    bool is_duplicate = false;
    for (uint32_t j = 0; j < distinct_queues_count; j++) {
      if (queue_indices[distinct_queues_first_indices[j]] == queue_indices[i]) {
        queue_indirection_index[i] = j;
        is_duplicate = true;
      }
    } 

    if (!is_duplicate) {
      distinct_queues_first_indices[distinct_queues_count] = i;

      queue_indirection_index[i] = distinct_queues_count;
      distinct_queues_count++;
    }
  }
  
  uint32_t distinct_queue_indices[distinct_queues_count];
  for (uint32_t i = 0; i < distinct_queues_count; i++) {
    distinct_queue_indices[i] = distinct_queues_first_indices[i];
  }

  MSIC_VERBOSE("Reducing queues from %d to %d distinct queues", num_queues, distinct_queues_count);

  VkDeviceQueueCreateInfo queue_create_infos[distinct_queues_count];

  for (uint32_t i = 0; i < distinct_queues_count; i++) {
    queue_create_infos[i].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queue_create_infos[i].pNext             = NULL;
    queue_create_infos[i].flags             = 0;
    queue_create_infos[i].queueFamilyIndex  = queue_indices[distinct_queue_indices[i]];
    queue_create_infos[i].queueCount        = 1;

    float queue_prio = 1.0f;
    queue_create_infos[i].pQueuePriorities  = &queue_prio;
  }
  
  create_info.queueCreateInfoCount = distinct_queues_count;
  create_info.pQueueCreateInfos = queue_create_infos;

  create_info.enabledLayerCount = 0;
  create_info.ppEnabledLayerNames = NULL;

  create_info.enabledExtensionCount = msic_device_extension_count;
  
  if (!msic_headless) create_info.enabledExtensionCount++;

  const char *extension_names[create_info.enabledExtensionCount];

  if (create_info.enabledExtensionCount > 0) {
    if (msic_write_device_extensions(create_info.enabledExtensionCount, extension_names)) {
      MSIC_ERR("Failed to write vulkan device extension names");
      return 1;
    }
    
    create_info.ppEnabledExtensionNames = (const char * const *) extension_names;

    for (uint32_t i = 0; i < create_info.enabledExtensionCount; i++) {
      MSIC_VERBOSE("\tEnabling device extension: %s", extension_names[i]);
    }

  } else {
    create_info.ppEnabledExtensionNames = NULL;
  }

  create_info.pEnabledFeatures = &msic_required_physical_device_features;

  MSIC_VK_CALL(vkCreateDevice(core->physical_device, &create_info, NULL, &core->device));

  VkQueue distinct_queues[distinct_queues_count];

  for (uint32_t i = 0; i < distinct_queues_count; i++) {
    vkGetDeviceQueue(core->device, queue_indices[distinct_queue_indices[i]], 0, &(distinct_queues[i]));
  }
 
  if (msic_num_extra_queue_families > 0) {
    core->queues = malloc(sizeof(VkQueue) * msic_num_extra_queue_families);

    for (uint32_t i = prefix_queues; i < num_queues; i++) {
      core->queues[i] = distinct_queues[queue_indirection_index[i]];
    }
  } 

  if (msic_present)  core->present_queue   = distinct_queues[queue_indirection_index[0]];
  if (msic_graphics) core->graphics_queue = distinct_queues[queue_indirection_index[msic_present ? 1 : 0]];

  return 0;
}
