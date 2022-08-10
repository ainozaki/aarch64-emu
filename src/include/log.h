#pragma once

#include <stdio.h>

#define ENABLE_LOG_CPU
#define ENABLE_LOG_DEBUG

#ifdef ENABLE_LOG_CPU
#define LOG_CPU(...) printf("\t\t"), printf(__VA_ARGS__)
#else
#define LOG_CPU(...)
#endif

#ifdef ENABLE_LOG_DEBUG
#define LOG_DEBUG(...) printf(__VA_ARGS__)
#else
#define LOG_DEBUG(...)
#endif
