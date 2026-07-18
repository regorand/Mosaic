#include "core.h"
#include <stdint.h>

void msic_default_log_callback(const char *file, uint32_t line, const char *message, uint32_t severity) {
  if (severity >= msic_log_level) {
    printf("mosaic | %s:%-5d | %7s | %s\n", file, line, msic_severity_strings[severity], message);
  }
}
