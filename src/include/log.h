#pragma once

#include <stdio.h>

#define ENABLE_LOG_CPU
#define ENABLE_LOG_DEBUG

#ifdef ENABLE_LOG_CPU
#define LOG_CPU(...) printf(__VA_ARGS__)
#else
#define LOG_CPU(...)
#endif

#ifdef ENABLE_LOG_DEBUG
#define LOG_DEBUG(...) printf(__VA_ARGS__)
#else
#define LOG_DEBUG(...)
#endif

//#define DEBUG_ON
#ifdef DEBUG_ON
#define LOG_EMU(...)                                                           \
  printf("[%s][%d][%s] ", __FILE__, __LINE__, __func__), printf(__VA_ARGS__)
#else
#define LOG_EMU(...)
#endif