#pragma once

#include <stdio.h>

extern int log_system_on;
extern int log_cpu_on;
extern int log_debug_on;

#define LOG_SYSTEM(...)                                                        \
  if (log_system_on) {                                                         \
    printf(__VA_ARGS__);                                                       \
  }
#define LOG_CPU(...)                                                           \
  if (log_cpu_on) {                                                            \
    printf(__VA_ARGS__);                                                       \
  }
#define LOG_DEBUG(...)                                                         \
  if (log_debug_on) {                                                          \
    printf(__VA_ARGS__);                                                       \
  }
