#pragma once

#include <stdio.h>

#define ENABLE_LOG_CPU 0
#define ENABLE_LOG_DEBUG 0

#if ENABLE_LOG_CPU
#define LOG_CPU(...) printf(__VA_ARGS__)
#else
#define LOG_CPU(...)
#endif

#if ENABLE_LOG_DEBUG
#define LOG_DEBUG(...) printf(__VA_ARGS__)
#else
#define LOG_DEBUG(...)
#endif