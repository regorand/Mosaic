#include "window.h"

#include <stdlib.h>

void msic_glfw_callback(int error_code, const char* message) {
  msic_output_log("glfw error", 0, MSIC_SEVERITY_ERR, "glfw error code: %d: %s", error_code, message); 
}

uint32_t msic_init_glfw() {
  if (!glfwInit()) {
    MSIC_ERR("Failed to initialize glfw");
    return 1;
  }
  
  glfwSetErrorCallback(&msic_glfw_callback);

  return 0;
}

void msic_populate_empty_window_context(MsicWindowContext *w_context) {
  w_context->open          = false;
  w_context->surface       = NULL;
  w_context->window        = NULL;
  w_context->formats       = NULL;
  w_context->present_modes = NULL;
  w_context->swapchain     = NULL;
}

uint32_t msic_create_windows(MsicVulkanCore *core) {
  int glfw_supported = glfwVulkanSupported();
  if (glfw_supported != GLFW_TRUE) {
    MSIC_ERR("GLFW support query returned %d", glfw_supported);
    return 1;
  }

  core->windows = malloc(msic_num_windows * sizeof(MsicWindowContext));

  for (uint32_t i = 0; i < msic_num_windows; i++) {
    msic_populate_empty_window_context(&core->windows[i]);
    glfwDefaultWindowHints();

    // No OpenGL context
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    // XXX temporary, resizing not implemented yet
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    if (msic_window_hint_counts != NULL) {
      for (uint32_t j = 0; j < msic_window_hint_counts[i]; j++) {
        glfwWindowHint(msic_window_hints[i][j], msic_window_hint_values[i][j]);
      }
    }

    GLFWwindow *w = glfwCreateWindow(800, 500, msic_application_name, NULL, NULL);
    if (w == NULL) {
      MSIC_ERR("Encountered fatal error creating glfw window, aborting");
      return 1;
    }

    core->windows[i].window = w;
    core->windows[i].open   = true;
  }

  core->num_windows = msic_num_windows;
  MSIC_VERBOSE("Created %d window(s)", core->num_windows);

  return 0;
}

uint32_t msic_create_surfaces(MsicVulkanCore *core) {
  for (uint32_t i = 0; i < msic_num_windows; i++) {
    MsicWindowContext *w_context = &core->windows[i];
    MSIC_VK_CALL(glfwCreateWindowSurface(core->instance, w_context->window, NULL, &w_context->surface)); 

  }

  MSIC_VERBOSE("Created %d surface(s)", core->num_windows);
  return 0;
}

