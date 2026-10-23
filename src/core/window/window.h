#ifndef MSIC_WINDOW_H_
#define MSIC_WINDOW_H_

#include "mosaic.h"

#include <stdint.h>

void msic_glfw_callback(int error_code, const char* message);

uint32_t msic_init_glfw(void); 

void msic_populate_empty_window_context(MsicWindowContext *w_context);

uint32_t msic_create_windows(MsicVulkanCore *core);
uint32_t msic_create_surfaces(MsicVulkanCore *core);

#endif // MSIC_WINDOW_H
