#pragma once

#define STRINGIFY(val) #val
#define STRINGIFY1(val) STRINGIFY(val)
#define EXPAND_FILE __FILE__ ":" STRINGIFY1(__LINE__)

//#define LOG_INFO(...)
#define LOG_INFO(...) getLogger().info(__VA_ARGS__) 
#define LOG_DEBUG(...) 
//#define LOG_DEBUG(...) getLogger().debug(__VA_ARGS__) 
//#define LOG_WARN(...)
#define LOG_WARN(...) getLogger().warning(__VA_ARGS__) 
//#define LOG_ERROR(...)
#define LOG_ERROR(...) getLogger().error(__VA_ARGS__) 

#include "beatsaber-hook/shared/utils/utils.h"
Logger& getLogger();